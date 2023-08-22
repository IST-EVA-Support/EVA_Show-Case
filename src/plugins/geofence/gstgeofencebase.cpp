#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstgeofencebase.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include "gstadmeta.h" // for metadata version 1
#include <gstadroi_frame.h> // for metadata version 2
#include <gstadroi_batch.h> // for metadata version 2
#include "utils.h"

////#define DEFAULT_ALERT_TYPE "BreakIn"


GST_DEBUG_CATEGORY_STATIC (gst_geofencebase_debug_category);
#define GST_CAT_DEFAULT gst_geofencebase_debug_category

static void gst_geofencebase_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_geofencebase_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_geofencebase_dispose (GObject * object);
static void gst_geofencebase_finalize (GObject * object);

static gboolean gst_geofencebase_start (GstBaseTransform * trans);
static gboolean gst_geofencebase_stop (GstBaseTransform * trans);
static gboolean gst_geofencebase_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
//static GstFlowReturn gst_geofencebase_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_geofencebase_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame);

static void mapGstVideoFrame2OpenCVMat(GstGeofencebase *geofencebase, GstVideoFrame *frame, GstMapInfo &info);
static void getDetectedPerson(GstGeofencebase *geofencebase, GstBuffer* buffer);
static void breakInDetection(GstGeofencebase *geofencebase, GstBuffer* buffer);
static void doAlgorithm(GstGeofencebase *geofencebase, GstBuffer* buffer, GstPad* pad);
static void drawAlertArea(GstGeofencebase *geofencebase);

struct _GstGeoFenceBasePrivate
{
    std::string alert_definition_path;
    std::vector<std::vector<double>> ratio_vec;
    std::vector<cv::Point> area_point_vec;
    std::vector<std::vector<cv::Point>> person_vec;
    bool area_display;
    bool person_display;
    bool alert;
    std::vector<int> map_vec;
    std::vector<int> map_breakin_person_vec;
    //std::string alertType;
    gchar* alertType;
};

enum
{
    PROP_0,
    PROP_ALERT_AREA_DEFINITION,
    PROP_ALERT_AREA_DISPLAY,
    PROP_ALERT_PERSON_DISPLAY,
    PROP_ALERT_TYPE,
    PROP_ENGINE_ID,
    PROP_QUERY,
};

#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gstgeofencebase", 0, "debug category for gstgeofencebase element");
G_DEFINE_TYPE_WITH_CODE(GstGeofencebase, gst_geofencebase, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(GstGeofencebase) DEBUG_INIT)

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */
static void gst_geofencebase_class_init (GstGeofencebaseClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
        gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Adlink Geo-Fence-Base video filter", "Filter/Video", "An ADLINK Geo-Fencing-Base demo video filter", "Dr. Paul Lin <paul.lin@adlinktech.com>");

  gobject_class->set_property = gst_geofencebase_set_property;
  gobject_class->get_property = gst_geofencebase_get_property;
  
  // Install the properties to GObjectClass
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DEFINITION,
                                   g_param_spec_string ("alert-area-def", "Alert-area-def", "The definition file location of the alert area respect the frame based on the specific resolution.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_TYPE,
                                   g_param_spec_string ("alert-type", "Alert-Type", "The alert type name when event occurred.", "BreakIn\0"/*DEFAULT_ALERT_TYPE*/, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DISPLAY,
                                   g_param_spec_boolean("area-display", "Area-display", "Show alert area in frame.", FALSE, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_PERSON_DISPLAY,
                                   g_param_spec_boolean("person-display", "Person-display", "Show inferenced person region in frame.", FALSE, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ENGINE_ID,
                                   g_param_spec_string ("engine-id", "engine-id", "The Inference engine ID, if empty will use plugin name as engine ID.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_QUERY,
                                   g_param_spec_string ("query", "query", "ROI query", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  
  gobject_class->dispose = gst_geofencebase_dispose;
  gobject_class->finalize = gst_geofencebase_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_geofencebase_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_geofencebase_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_geofencebase_set_info);
  //video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_geofencebase_transform_frame);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_geofencebase_transform_frame_ip);

}

static void gst_geofencebase_init (GstGeofencebase *geofencebase)
{
    /*< private >*/
    geofencebase->priv = (GstGeofencebasePrivate *)gst_geofencebase_get_instance_private (geofencebase);
    
    geofencebase->priv->area_display = false;
    geofencebase->priv->person_display = false;
    geofencebase->priv->alert = false;
    geofencebase->eventTick = 0;
    geofencebase->lastEventTick = 0;
    geofencebase->priv->alertType = "BreakIn\0";//DEFAULT_ALERT_TYPE;
    geofencebase->engineID = "";
    geofencebase->query = "";
}

void gst_geofencebase_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE (object);

  GST_DEBUG_OBJECT (geofencebase, "set_property");

  switch (property_id)
  {
    case PROP_ALERT_AREA_DEFINITION:
    {
        geofencebase->priv->alert_definition_path = g_value_dup_string(value);
        GST_MESSAGE(std::string("geofencebase->priv->alert_definition_path = " + geofencebase->priv->alert_definition_path).c_str());
        
        GST_MESSAGE("Start parsing definition file...");
        std::ifstream infile(geofencebase->priv->alert_definition_path);
        if(!infile) 
        {
            std::cout << "Cannot open input file.\n";
            break;
        }
        std::string lineString;
        while (std::getline(infile, lineString))
        {
            std::vector<std::string> ratioVec = split(lineString);
            double x = std::atof(ratioVec[0].c_str());
            double y = std::atof(ratioVec[1].c_str());
            
            std::vector<double> vec;
            vec.push_back(x);
            vec.push_back(y);
            geofencebase->priv->ratio_vec.push_back(vec);
        }
        GST_MESSAGE("Parsing definition file done!");
        
        break;  
    }
    case PROP_ALERT_TYPE:
    {
        geofencebase->priv->alertType = g_value_dup_string(value);
        break;  
    }
    case PROP_ALERT_AREA_DISPLAY:
    {
        geofencebase->priv->area_display = g_value_get_boolean(value);
        if(geofencebase->priv->area_display)
            GST_MESSAGE("Display area is enabled!");
        break;
    }
    case PROP_ALERT_PERSON_DISPLAY:
    {
        geofencebase->priv->person_display = g_value_get_boolean(value);
        if(geofencebase->priv->person_display)
            GST_MESSAGE("Display inferenced person is enabled!");
        break;
    }
    case PROP_ENGINE_ID:
    {
        geofencebase->engineID = g_value_dup_string(value);
        break;  
    }
    case PROP_QUERY:
    {
        geofencebase->query = g_value_dup_string(value);
        break;  
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
  }
}

void gst_geofencebase_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE (object);

  GST_DEBUG_OBJECT (geofencebase, "get_property");

  switch (property_id)
  {
    case PROP_ALERT_AREA_DEFINITION:
       g_value_set_string (value, geofencebase->priv->alert_definition_path.c_str());
       break;
    case PROP_ALERT_TYPE:
       g_value_set_string (value, geofencebase->priv->alertType);
       //g_value_set_string (value, geofencebase->priv->alertType.c_str());
       break;
    case PROP_ALERT_AREA_DISPLAY:
       g_value_set_boolean(value, geofencebase->priv->area_display);
       break;
    case PROP_ALERT_PERSON_DISPLAY:
       g_value_set_boolean(value, geofencebase->priv->person_display);
       break;
    case PROP_ENGINE_ID:
       g_value_set_string (value, geofencebase->engineID);
       //g_value_set_string (value, geofencebase->engineID.c_str());
       break;
    case PROP_QUERY:
       g_value_set_string (value, geofencebase->query);
       //g_value_set_string (value, geofencebase->query.c_str());
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_geofencebase_dispose (GObject * object)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE (object);

  GST_DEBUG_OBJECT (geofencebase, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_geofencebase_parent_class)->dispose (object);
}

void gst_geofencebase_finalize (GObject * object)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE(object);

  GST_DEBUG_OBJECT (geofencebase, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_geofencebase_parent_class)->finalize (object);
}

static gboolean gst_geofencebase_start (GstBaseTransform * trans)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE (trans);

  GST_DEBUG_OBJECT (geofencebase, "start");
  
  return TRUE;
}

static gboolean gst_geofencebase_stop (GstBaseTransform * trans)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE (trans);

  GST_DEBUG_OBJECT (geofencebase, "stop");

  return TRUE;
}

static gboolean gst_geofencebase_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE (filter);

  GST_DEBUG_OBJECT (geofencebase, "set_info");
  
                
  // Start to parse the real area point
  int width = in_info->width;
  int height = in_info->height;
  for(uint id = 0; id < geofencebase->priv->ratio_vec.size(); ++id)
  {
      cv::Point2d p;
      p.x = (int)(width * geofencebase->priv->ratio_vec[id][0]);
      p.y = (int)(height * geofencebase->priv->ratio_vec[id][1]);
      
      geofencebase->priv->area_point_vec.push_back(p);
  }
  return TRUE;
}

/* transform */
// static GstFlowReturn gst_geofencebase_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe)
// {
//   GstGeofencebase *geofencebase = GST_GEOFENCEBASE (filter);
// 
//   GST_DEBUG_OBJECT (geofencebase, "transform_frame");
// 
//   return GST_FLOW_OK;
// }

static GstFlowReturn gst_geofencebase_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  GstGeofencebase *geofencebase = GST_GEOFENCEBASE (filter);
  GstMapInfo info;

  GST_DEBUG_OBJECT (geofencebase, "transform_frame_ip");
  gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
  // map frame data from stream to geofencebase srcMat
  mapGstVideoFrame2OpenCVMat(geofencebase, frame, info);
  
  // get inference detected persons
  getDetectedPerson(geofencebase, frame->buffer);
  
  // do algorithm
  doAlgorithm(geofencebase, frame->buffer, filter->element.sinkpad);
  
  // draw alert area
  drawAlertArea(geofencebase);
  
  gst_buffer_unmap(frame->buffer, &info);
  return GST_FLOW_OK;
}

static void mapGstVideoFrame2OpenCVMat(GstGeofencebase *geofencebase, GstVideoFrame *frame, GstMapInfo &info)
{
    if(geofencebase->srcMat.cols == 0 || geofencebase->srcMat.rows == 0)
        geofencebase->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    else if((geofencebase->srcMat.cols != frame->info.width) || (geofencebase->srcMat.rows != frame->info.height))
    {
        geofencebase->srcMat.release();
        geofencebase->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else
        geofencebase->srcMat.data = info.data;
}

static void getDetectedPerson(GstGeofencebase *geofencebase, GstBuffer* buffer)
{
    // reset for each frame
    geofencebase->priv->person_vec.clear();
    geofencebase->priv->map_vec.clear();
    
    int width = geofencebase->srcMat.cols;
    int height = geofencebase->srcMat.rows;
    
    if(geofencebase->query != "") /*if query is not empty, use metadata version 2*/
    {
        // check engineID
        std::string engineID = std::string(geofencebase->engineID);
        if (engineID == "") 
        {
            gchar *name = gst_element_get_name(geofencebase);
            if (engineID == "")
                name = gst_element_get_name(geofencebase);
            
            if (name == NULL)
                engineID = std::string("geofencebase");
            else
            {
                engineID = std::string(name);
                g_free(name);
            }
        }
        
        std::vector<QueryResult> results = gst_buffer_adroi_query(buffer, geofencebase->query);
//         std::cout << "std::vector<QueryResult> results size = " << results.size() << std::endl;
        for(unsigned int result_index = 0; result_index < results.size(); ++result_index)
        {
            QueryResult queryResult = results[result_index];
//             std::cout << "rois size = " << queryResult.rois.size() << std::endl;
            //for(auto roi: queryResult.rois)
            for(unsigned int i = 0 ; i < queryResult.rois.size() ; ++i)
            {
                if(queryResult.rois[i]->category == "box")
                {
                    auto box = std::static_pointer_cast<Box>(queryResult.rois[i]);
                    
                    int x1 = (int)(width * box->x1);
                    int y1 = (int)(height * box->y1);
                    int x2 = (int)(width * box->x2);
                    int y2 = (int)(height * box->y2);
                    
                    std::vector<cv::Point> personPoint_vec;
                    personPoint_vec.push_back(cv::Point2d(x1, y1));
                    personPoint_vec.push_back(cv::Point2d(x2, y1));
                    personPoint_vec.push_back(cv::Point2d(x2, y2));
                    personPoint_vec.push_back(cv::Point2d(x1, y2));
                    
                    geofencebase->priv->person_vec.push_back(personPoint_vec);
                    
                    // record the mapping index
                    geofencebase->priv->map_vec.push_back(i);
                    
                    int current_id = geofencebase->priv->person_vec.size() - 1;
                    
                    if(geofencebase->priv->person_display)
                        cv::rectangle(geofencebase->srcMat, cv::Point(geofencebase->priv->person_vec[current_id][0].x, geofencebase->priv->person_vec[current_id][0].y), cv::Point(geofencebase->priv->person_vec[current_id][2].x, geofencebase->priv->person_vec[current_id][2].y), cv::Scalar(0,128,128), 3, cv::LINE_8);
                }    
            }
        } 
    }
    else /*if query is empty, use metadata version 1*/
    {
        GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
        if (meta == NULL)
            GST_MESSAGE("Adlink metadata is not exist!");
        else
        {
            AdBatch &batch = meta->batch;
            
            bool frame_exist = batch.frames.size() > 0 ? true : false;
            if(frame_exist)
            {
                VideoFrameData frame_info = batch.frames[0];
                    
                int detectionBoxResultNumber = frame_info.detection_results.size();
                
                for(int i = 0 ; i < detectionBoxResultNumber ; ++i)
                {
                    adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
                    
                    if(detection_result.obj_label.compare("person") == 0)
                    {
                        int x1 = (int)(width * detection_result.x1);
                        int y1 = (int)(height * detection_result.y1);
                        int x2 = (int)(width * detection_result.x2);
                        int y2 = (int)(height * detection_result.y2);
                        
                        std::vector<cv::Point> personPoint_vec;
                        personPoint_vec.push_back(cv::Point2d(x1, y1));
                        personPoint_vec.push_back(cv::Point2d(x2, y1));
                        personPoint_vec.push_back(cv::Point2d(x2, y2));
                        personPoint_vec.push_back(cv::Point2d(x1, y2));
                        
                        geofencebase->priv->person_vec.push_back(personPoint_vec);
                        
                        // record the mapping index
                        geofencebase->priv->map_vec.push_back(i);
                        
                        int current_id = geofencebase->priv->person_vec.size() - 1;
                        
                        if(geofencebase->priv->person_display)
                            cv::rectangle(geofencebase->srcMat, cv::Point(geofencebase->priv->person_vec[current_id][0].x, geofencebase->priv->person_vec[current_id][0].y), cv::Point(geofencebase->priv->person_vec[current_id][2].x, geofencebase->priv->person_vec[current_id][2].y), cv::Scalar(0,128,128), 3, cv::LINE_8);
                            
                    }
                }
            }
        }
    }
}

static void breakInDetection(GstGeofencebase *geofencebase)
{
    // Reset break in person index
    geofencebase->priv->map_breakin_person_vec.clear();
    
    // Reset region
    if((cv::getTickCount() - geofencebase->eventTick)/ cv::getTickFrequency() > 2)
        geofencebase->priv->alert = false;
    
    std::vector<cv::Point> intersectionPolygon;
    int num_person_detected = geofencebase->priv->person_vec.size();
    //std::cout << "num_person_detected = " << num_person_detected << std::endl;
    for(int id = 0; id < num_person_detected; ++id)
    {
        float intersectArea = cv::intersectConvexConvex(geofencebase->priv->area_point_vec, geofencebase->priv->person_vec[id], intersectionPolygon, true);
        
        if(intersectArea > 0)
        {
            geofencebase->eventTick = cv::getTickCount();
            geofencebase->priv->alert = true;
            geofencebase->priv->map_breakin_person_vec.push_back(id);
        }
    }
}

static void doAlgorithm(GstGeofencebase *geofencebase, GstBuffer* buffer, GstPad* pad)
{
    // If no region, return directly. Points must larger than 2.
    if(geofencebase->priv->area_point_vec.size() <= 2)
        return;
    
    // Break in detection
    breakInDetection(geofencebase);
    
    if(geofencebase->priv->alert == true)
    {
        if(geofencebase->query != "")
        {
            auto *f_meta = gst_buffer_acquire_adroi_frame_meta(buffer, pad);
            if (f_meta == nullptr) 
            {
                GST_ERROR("Can not get adlink ROI frame metadata");
                return;
            }
            
            auto *b_meta = gst_buffer_acquire_adroi_batch_meta(buffer);
            if (b_meta == nullptr)
            {
                GST_ERROR("Can not acquire adlink ROI batch metadata");
                return;
            }
            
            auto qrs = f_meta->frame->query(geofencebase->query);
            if(qrs.size() > 0)
            {
                for(int i = 0; i < geofencebase->priv->map_breakin_person_vec.size(); ++i)
                {
                    int id = geofencebase->priv->map_breakin_person_vec[i];
                    std::string alertMessage = std::string(geofencebase->priv->alertType) + "<" + return_current_time_and_date() + ">";
                    qrs[0].rois[geofencebase->priv->map_vec[id]]->events.push_back(alertMessage);
                }
            }
        }
        else
        {
            // If metadata does not exist, return directly.
            GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
            if (meta == NULL)
            {
                GST_MESSAGE("Adlink metadata is not exist!");
                return;
            }
            AdBatch &batch = meta->batch;
            bool frame_exist = batch.frames.size() > 0 ? true : false;
            if(frame_exist)
            {
                for(int i = 0; i < geofencebase->priv->map_breakin_person_vec.size(); ++i)
                {
                    int id = geofencebase->priv->map_breakin_person_vec[i];
                    // alert message format:",type<time>", must used append.
                    std::string alertMessage = "," + std::string(geofencebase->priv->alertType) + "<" + return_current_time_and_date() + ">";
                    
                    // write alert message to meta use pointer
                    meta->batch.frames[0].detection_results[geofencebase->priv->map_vec[id]].meta += alertMessage;
                }
            }
        }
    }
}

static void drawAlertArea(GstGeofencebase *geofencebase)
{
    if(geofencebase->priv->area_display)
    {
        int lineType = cv::LINE_8;
        int pt_num = geofencebase->priv->area_point_vec.size();
        cv::Scalar color = geofencebase->priv->alert == true ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0);
        int thickness = geofencebase->priv->alert == true ? 6 : 2;
        for(int i = 0; i < pt_num; ++i)
            cv::line(geofencebase->srcMat, geofencebase->priv->area_point_vec[i%pt_num], geofencebase->priv->area_point_vec[(i+1)%pt_num], color, thickness, lineType);
    }
}
