#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstgeofencefoot.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include "gstadmeta.h"
#include "utils.h"

//#define DEFAULT_ALERT_TYPE "BreakIn"


GST_DEBUG_CATEGORY_STATIC (gst_geofencefoot_debug_category);
#define GST_CAT_DEFAULT gst_geofencefoot_debug_category

static void gst_geofencefoot_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_geofencefoot_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_geofencefoot_dispose (GObject * object);
static void gst_geofencefoot_finalize (GObject * object);

static gboolean gst_geofencefoot_start (GstBaseTransform * trans);
static gboolean gst_geofencefoot_stop (GstBaseTransform * trans);
static gboolean gst_geofencefoot_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_geofencefoot_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame);

static void mapGstVideoFrame2OpenCVMat(Gstgeofencefoot *geofencefoot, GstVideoFrame *frame, GstMapInfo &info);
static void getDetectedPerson(Gstgeofencefoot *geofencefoot, GstBuffer* buffer);
static void doAlgorithm(Gstgeofencefoot *geofencefoot, GstBuffer* buffer);
static void drawArea(Gstgeofencefoot *geofencefoot);
static void drawAlertPerson(Gstgeofencefoot *geofencefoot);

struct _GstgeofencefootPrivate
{
    std::string alert_definition_path;
    std::vector<std::vector<double>> ratio_vec;
    std::vector<cv::Point> area_point_vec;
    std::vector<std::vector<cv::Point>> person_vec;
    std::vector<std::vector<cv::Point>> upper_person_vec;
    std::vector<std::vector<cv::Point>> foot_vec;
    bool area_display;
    bool person_display;
    bool alert;
    std::vector<int> map_vec;
    //std::string alertType;
	gchar* alertType;
    bool enterArea;
};

enum
{
    PROP_0,
    PROP_ALERT_AREA_DEFINITION,
    PROP_ALERT_AREA_DISPLAY,
    PROP_ALERT_PERSON_DISPLAY,
    PROP_ALERT_TYPE,    
};

#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gstgeofencefoot", 0, "debug category for gstgeofencefoot element");
G_DEFINE_TYPE_WITH_CODE(Gstgeofencefoot, gst_geofencefoot, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(Gstgeofencefoot) DEBUG_INIT)

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */
static void gst_geofencefoot_class_init (GstgeofencefootClass * klass)
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
      "Adlink Geo-Fence-Foot video filter", "Filter/Video", "An ADLINK Geo-Fence-Foot demo video filter", "Dr. Paul Lin <paul.lin@adlinktech.com> and Bernie JK Liu<berniejk.liu@adlinktech.com>");

  gobject_class->set_property = gst_geofencefoot_set_property;
  gobject_class->get_property = gst_geofencefoot_get_property;
  
  // Install the properties to GObjectClass
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DEFINITION,
                                   g_param_spec_string ("alert-area-def", "Alert-area-def", "The definition file location of the alert area respect the frame based on the specific resolution.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_TYPE,
                                   g_param_spec_string ("alert-type", "Alert-Type", "The alert type name when event occurred.", "BreakIn\0"/*DEFAULT_ALERT_TYPE*/, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DISPLAY,
                                   g_param_spec_boolean("area-display", "Area-display", "Show alert area in frame.", FALSE, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_PERSON_DISPLAY,
                                   g_param_spec_boolean("person-display", "Person-display", "Show inferenced person region in frame.", FALSE, G_PARAM_READWRITE));
  
  
  gobject_class->dispose = gst_geofencefoot_dispose;
  gobject_class->finalize = gst_geofencefoot_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_geofencefoot_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_geofencefoot_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_geofencefoot_set_info);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_geofencefoot_transform_frame_ip);

}

static void gst_geofencefoot_init (Gstgeofencefoot *geofencefoot)
{
    /*< private >*/
    geofencefoot->priv = (GstgeofencefootPrivate *)gst_geofencefoot_get_instance_private (geofencefoot);
    
    geofencefoot->priv->area_display = false;
    geofencefoot->priv->person_display = false;
    geofencefoot->priv->alert = false;
    geofencefoot->eventTick = 0;
    geofencefoot->lastEventTick = 0;
    geofencefoot->priv->alertType = "BreakIn\0";//DEFAULT_ALERT_TYPE;
    geofencefoot->priv->enterArea = false;
}

void gst_geofencefoot_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT (object);

  GST_DEBUG_OBJECT (geofencefoot, "set_property");

  switch (property_id)
  {
    case PROP_ALERT_AREA_DEFINITION:
    {
        geofencefoot->priv->alert_definition_path = g_value_dup_string(value);
        GST_MESSAGE(std::string("geofencefoot->priv->alert_definition_path = " + geofencefoot->priv->alert_definition_path).c_str());
        
        GST_MESSAGE("Start parsing definition file...");
        std::ifstream infile(geofencefoot->priv->alert_definition_path);
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
            geofencefoot->priv->ratio_vec.push_back(vec);
        }
        GST_MESSAGE("Parsing definition file done!");
        
        break;  
    }
    case PROP_ALERT_TYPE:
    {
        geofencefoot->priv->alertType = g_value_dup_string(value);
        break;  
    }
    case PROP_ALERT_AREA_DISPLAY:
    {
        geofencefoot->priv->area_display = g_value_get_boolean(value);
        if(geofencefoot->priv->area_display)
            GST_MESSAGE("Display area is enabled!");
        break;
    }
    case PROP_ALERT_PERSON_DISPLAY:
    {
        geofencefoot->priv->person_display = g_value_get_boolean(value);
        if(geofencefoot->priv->person_display)
            GST_MESSAGE("Display inferenced person is enabled!");
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
  }
}

void gst_geofencefoot_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT (object);

  GST_DEBUG_OBJECT (geofencefoot, "get_property");

  switch (property_id)
  {
    case PROP_ALERT_AREA_DEFINITION:
       g_value_set_string (value, geofencefoot->priv->alert_definition_path.c_str());
       break;
    case PROP_ALERT_TYPE:
       //g_value_set_string (value, geofencefoot->priv->alertType.c_str());
	   g_value_set_string (value, geofencefoot->priv->alertType);
       break;
    case PROP_ALERT_AREA_DISPLAY:
       g_value_set_boolean(value, geofencefoot->priv->area_display);
       break;
    case PROP_ALERT_PERSON_DISPLAY:
       g_value_set_boolean(value, geofencefoot->priv->person_display);
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_geofencefoot_dispose (GObject * object)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT (object);

  GST_DEBUG_OBJECT (geofencefoot, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_geofencefoot_parent_class)->dispose (object);
}

void gst_geofencefoot_finalize (GObject * object)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT(object);

  GST_DEBUG_OBJECT (geofencefoot, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_geofencefoot_parent_class)->finalize (object);
}

static gboolean gst_geofencefoot_start (GstBaseTransform * trans)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT (trans);

  GST_DEBUG_OBJECT (geofencefoot, "start");
  
  return TRUE;
}

static gboolean gst_geofencefoot_stop (GstBaseTransform * trans)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT (trans);

  GST_DEBUG_OBJECT (geofencefoot, "stop");

  return TRUE;
}

static gboolean gst_geofencefoot_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT (filter);

  GST_DEBUG_OBJECT (geofencefoot, "set_info");
  
                
  // Start to parse the real area point
  int width = in_info->width;
  int height = in_info->height;
  for(unsigned int id = 0; id < geofencefoot->priv->ratio_vec.size(); ++id)
  {
      cv::Point2d p;
      p.x = (int)(width * geofencefoot->priv->ratio_vec[id][0]);
      p.y = (int)(height * geofencefoot->priv->ratio_vec[id][1]);
      
      geofencefoot->priv->area_point_vec.push_back(p);
  }
  return TRUE;
}

static GstFlowReturn gst_geofencefoot_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  Gstgeofencefoot *geofencefoot = GST_GEOFENCEFOOT (filter);
  GstMapInfo info;

  GST_DEBUG_OBJECT (geofencefoot, "transform_frame_ip");
  gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
  // map frame data from stream to geofencefoot srcMat
  mapGstVideoFrame2OpenCVMat(geofencefoot, frame, info);
  
  // get inference detected persons
  getDetectedPerson(geofencefoot, frame->buffer);
  
  // do algorithm
  doAlgorithm(geofencefoot, frame->buffer);  
  
  // draw alert area
  drawArea(geofencefoot);
  
  // draw alert person
  drawAlertPerson(geofencefoot);
  
  gst_buffer_unmap(frame->buffer, &info);
  return GST_FLOW_OK;
}

static void mapGstVideoFrame2OpenCVMat(Gstgeofencefoot *geofencefoot, GstVideoFrame *frame, GstMapInfo &info)
{
    if(geofencefoot->srcMat.cols == 0 || geofencefoot->srcMat.rows == 0)
        geofencefoot->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    else if((geofencefoot->srcMat.cols != frame->info.width) || (geofencefoot->srcMat.rows != frame->info.height))
    {
        geofencefoot->srcMat.release();
        geofencefoot->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else
        geofencefoot->srcMat.data = info.data;
}

static void getDetectedPerson(Gstgeofencefoot *geofencefoot, GstBuffer* buffer)
{
    // reset for each frame
    geofencefoot->priv->person_vec.clear();
    geofencefoot->priv->upper_person_vec.clear();
    geofencefoot->priv->foot_vec.clear();
    geofencefoot->priv->map_vec.clear();
    
    // image boundry 
    cv::Rect boundry(0, 0, geofencefoot->srcMat.cols, geofencefoot->srcMat.rows);
    
    
    GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
    if (meta == NULL)
        g_message("Adlink metadata is not exist!");
    else
    {
        AdBatch &batch = meta->batch;
        
        bool frame_exist = batch.frames.size() > 0 ? true : false;
        if(frame_exist)
        {
            VideoFrameData frame_info = batch.frames[0];
                
            int detectionBoxResultNumber = frame_info.detection_results.size();
            int width = geofencefoot->srcMat.cols;
            int height = geofencefoot->srcMat.rows;
            
            for(int i = 0 ; i < detectionBoxResultNumber ; ++i)
            {
                adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
                
                int x1 = (int)(width * detection_result.x1);
                int y1 = (int)(height * detection_result.y1);
                int x2 = (int)(width * detection_result.x2);
                int y2 = (int)(height * detection_result.y2);
                    
                if(detection_result.obj_label.compare("person") == 0)
                {    
                    std::vector<cv::Point> personPoint_vec;
                    personPoint_vec.push_back(cv::Point2d(x1, y1));
                    personPoint_vec.push_back(cv::Point2d(x2, y1));
                    personPoint_vec.push_back(cv::Point2d(x2, y2));
                    personPoint_vec.push_back(cv::Point2d(x1, y2));
                    
                    int head2footHeight = (y2 - y1 + 1) * 0.9;
                    std::vector<cv::Point> footPoint_vec;
                    footPoint_vec.push_back(cv::Point2d(x1, y1 + head2footHeight));
                    footPoint_vec.push_back(cv::Point2d(x2, y1 + head2footHeight));
                    footPoint_vec.push_back(cv::Point2d(x2, y2));
                    footPoint_vec.push_back(cv::Point2d(x1, y2));
                    
                    std::vector<cv::Point> upper_personPoint_vec;
                    upper_personPoint_vec.push_back(cv::Point2d(x1, y1));
                    upper_personPoint_vec.push_back(cv::Point2d(x2, y1));
                    upper_personPoint_vec.push_back(cv::Point2d(x2, y1 + (y2-y1+1)/4));
                    upper_personPoint_vec.push_back(cv::Point2d(x1, y1 + (y2-y1+1)/4));
                    
                    geofencefoot->priv->person_vec.push_back(personPoint_vec);
                    geofencefoot->priv->foot_vec.push_back(footPoint_vec);
                    geofencefoot->priv->upper_person_vec.push_back(upper_personPoint_vec);
                    
                    // record the mapping index
                    geofencefoot->priv->map_vec.push_back(i);
                    
                    int current_id = geofencefoot->priv->person_vec.size() - 1;
                    
                    if(geofencefoot->priv->person_display)
                        cv::rectangle(geofencefoot->srcMat, cv::Point(geofencefoot->priv->person_vec[current_id][0].x, geofencefoot->priv->person_vec[current_id][0].y), cv::Point(geofencefoot->priv->person_vec[current_id][2].x, geofencefoot->priv->person_vec[current_id][2].y), cv::Scalar(0,128,128), 3, cv::LINE_8);
                }                          
            }
        }
    }
}

static void doAlgorithm(Gstgeofencefoot *geofencefoot, GstBuffer* buffer)
{
    // If no region, return directly. Points must larger than 2.
    if(geofencefoot->priv->area_point_vec.size() <= 2)
        return;
    
    // If metadata does not exist, return directly.
    GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
    if (meta == NULL)
    {
        g_message("Adlink metadata is not exist!");
        return;
    }
    AdBatch &batch = meta->batch;
    
    // Reset region from alarm to normal after 2 seconds.
    if((cv::getTickCount() - geofencefoot->eventTick)/ cv::getTickFrequency() > 2)
        geofencefoot->priv->alert = false;
    
    std::vector<cv::Point> intersectionPolygon;
    int num_person_detected = geofencefoot->priv->person_vec.size();
    for(int id = 0; id < num_person_detected; ++id)
    {        
        // person logic size check
        float personWidth = geofencefoot->priv->person_vec[id][1].x - geofencefoot->priv->person_vec[id][0].x + 1;
        float personHeight = geofencefoot->priv->person_vec[id][2].y - geofencefoot->priv->person_vec[id][1].y + 1;
        if((personHeight / personWidth) < 1.4)
            continue;
        
        // foot intersection with area(overlap 70 %)
        int footWidth = geofencefoot->priv->foot_vec[id][1].x - geofencefoot->priv->foot_vec[id][0].x + 1;
        int footHeight = geofencefoot->priv->foot_vec[id][2].y - geofencefoot->priv->foot_vec[id][1].y + 1;
        float intersectArea = cv::intersectConvexConvex(geofencefoot->priv->area_point_vec, geofencefoot->priv->foot_vec[id], intersectionPolygon, true);
        float footOverlapRatio = intersectArea/(footWidth * footHeight);
        geofencefoot->priv->enterArea = footOverlapRatio > 0.7 ? true : false;

        //entering area detected
        if(geofencefoot->priv->enterArea == true)
        {
            // alert message format:",type<time>", must used append.
            std::string alertMessage = "," + std::string(geofencefoot->priv->alertType) + "<" + return_current_time_and_date() + ">";

            geofencefoot->eventTick = cv::getTickCount();
            geofencefoot->priv->alert = true;
            meta->batch.frames[0].detection_results[geofencefoot->priv->map_vec[id]].meta += alertMessage;
        }
    }
}

static void drawArea(Gstgeofencefoot *geofencefoot)
{
    if(geofencefoot->priv->area_display)
    {
        int lineType = cv::LINE_8;
        int pt_num = geofencefoot->priv->area_point_vec.size();
        cv::Scalar color = geofencefoot->priv->alert == true ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0);        
        int thickness = geofencefoot->priv->alert == true ? 6 : 2;
        for(int i = 0; i < pt_num; ++i)
            cv::line(geofencefoot->srcMat, geofencefoot->priv->area_point_vec[i%pt_num], geofencefoot->priv->area_point_vec[(i+1)%pt_num], color, thickness, lineType);
    }
}

static void drawAlertPerson(Gstgeofencefoot *geofencefoot)
{
    if(geofencefoot->priv->person_display)
    {
        cv::Scalar color = geofencefoot->priv->alert ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
        int thickness = 3;
        int num_person_detected = geofencefoot->priv->person_vec.size();
        for(int id = 0; id < num_person_detected; ++id)
        {
            cv::rectangle(geofencefoot->srcMat, cv::Point(geofencefoot->priv->foot_vec[id][0].x, geofencefoot->priv->foot_vec[id][0].y), cv::Point(geofencefoot->priv->foot_vec[id][2].x, geofencefoot->priv->foot_vec[id][2].y), color, thickness, cv::LINE_8);        
        }
    }
}
