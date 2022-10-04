#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstdrop.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
// #include "gstadmeta.h"
#include <adroi.h>
#include <gstadroi_frame.h>
#include "utils.h"



GST_DEBUG_CATEGORY_STATIC (gst_drop_debug_category);
#define GST_CAT_DEFAULT gst_drop_debug_category

static void gst_drop_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_drop_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_drop_dispose (GObject * object);
static void gst_drop_finalize (GObject * object);

static gboolean gst_drop_start (GstBaseTransform * trans);
static gboolean gst_drop_stop (GstBaseTransform * trans);
static gboolean gst_drop_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
//static GstFlowReturn gst_drop_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_drop_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame);

static void mapGstVideoFrame2OpenCVMat(GstDrop *drop, GstVideoFrame *frame, GstMapInfo &info);
static void getPlaten(GstDrop *drop, GstVideoFrame *frame);
// static void getDetectedPerson(GstDrop *drop, GstBuffer* buffer);
static void doAlgorithm(GstDrop *drop, GstBuffer* buffer);
static void calcObjectRectangle(GstDrop *drop);
static void drawAlertArea(GstDrop *drop);

struct graphiteRegion
{
    int x;
    int y;
    int width;
    int height;
    float confidence;
    std::string label;
    
    graphiteRegion(float x, float y, int width, int height, float confidence, std::string label)
    {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->confidence = confidence;
        this->label = label;
    }
    
    void show()
    {
        std::cout << "label = " << label << ", (x, y, w, h) = (" << x << ", " << y << ", " << width << ", " << height << "), confidence = " << confidence << std::endl;
    }
};

struct _GstDropPrivate
{
    std::string alert_definition_path;
     std::vector<std::vector<double>> ratio_vec;
     std::vector<cv::Point> area_point_vec;
    std::string query;
    std::vector<graphiteRegion> detectedROI_vec; 
    std::vector<cv::Rect> boundRect;
    bool area_display;
    
//     std::string alert_definition_path;
//     std::vector<std::vector<double>> ratio_vec;
//     std::vector<cv::Point> area_point_vec;
//     std::vector<std::vector<cv::Point>> person_vec;
//     bool area_display;
//     bool person_display;
    bool alert;
//     std::vector<int> map_vec;
//     //std::string alertType;
// 	gchar* alertType;
};

enum
{
    PROP_0,
    PROP_QUERY,
    PROP_ALERT_AREA_DEFINITION,
    PROP_ALERT_AREA_DISPLAY,
//     PROP_ALERT_PERSON_DISPLAY,
//     PROP_ALERT_TYPE,
};

#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gstdrop", 0, "debug category for gstdrop element");
G_DEFINE_TYPE_WITH_CODE(GstDrop, gst_drop, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(GstDrop) DEBUG_INIT)

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */
static void gst_drop_class_init (GstDropClass * klass)
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
      "Adlink Graphite anomaly drop video filter", "Filter/Video", "An ADLINK Graphite anomaly drop demo video filter", "Dr. Paul Lin <paul.lin@adlinktech.com>");

  gobject_class->set_property = gst_drop_set_property;
  gobject_class->get_property = gst_drop_get_property;
  
  // Install the properties to GObjectClass
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DEFINITION,
                                   g_param_spec_string ("alert-area-def", "Alert-area-def", "The definition file location of the alert area respect the frame based on the specific resolution.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_QUERY,
                                   g_param_spec_string ("query", "Query", "The query string to acquire the inference data from adlink metadata.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
//   g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DEFINITION,
//                                    g_param_spec_string ("alert-area-def", "Alert-area-def", "The definition file location of the alert area respect the frame based on the specific resolution.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
//   
//   g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_TYPE,
//                                    g_param_spec_string ("alert-type", "Alert-Type", "The alert type name when event occurred.", "BreakIn\0"/*DEFAULT_ALERT_TYPE*/, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
//   
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DISPLAY,
                                   g_param_spec_boolean("area-display", "Area-display", "Show alert area in frame.", FALSE, G_PARAM_READWRITE));
//   
//   g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_PERSON_DISPLAY,
//                                    g_param_spec_boolean("person-display", "Person-display", "Show inferenced person region in frame.", FALSE, G_PARAM_READWRITE));
  
  
  gobject_class->dispose = gst_drop_dispose;
  gobject_class->finalize = gst_drop_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_drop_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_drop_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_drop_set_info);
  //video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_drop_transform_frame);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_drop_transform_frame_ip);

}

static void gst_drop_init (GstDrop *drop)
{
    /*< private >*/
    drop->priv = (GstDropPrivate *)gst_drop_get_instance_private (drop);
    

    drop->priv->query = "//yolov4tiny[class in platen]";
    //drop->pBackSub = cv::createBackgroundSubtractorMOG2();
    drop->pBackSub = cv::createBackgroundSubtractorKNN();
    drop->rng = cv::RNG(321650888278);
    drop->start_learn = 0;
    drop->required_time_to_learn = 5;
    drop->bDetect = false;
    drop->priv->boundRect = std::vector<cv::Rect>();
    drop->priv->area_display = false;
//     drop->priv->person_display = false;
    drop->priv->alert = false;
    drop->eventTick = 0;
    drop->lastEventTick = 0;
//     drop->priv->alertType = "BreakIn\0";//DEFAULT_ALERT_TYPE;
}

void gst_drop_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstDrop *drop = GST_DROP (object);

  GST_DEBUG_OBJECT (drop, "set_property");

  switch (property_id)
  {
      case PROP_ALERT_AREA_DEFINITION:
      {
        drop->priv->alert_definition_path = g_value_dup_string(value);
        GST_MESSAGE(std::string("drop->priv->alert_definition_path = " + drop->priv->alert_definition_path).c_str());
        
        GST_MESSAGE("Start parsing definition file...");
        std::ifstream infile(drop->priv->alert_definition_path);
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
            drop->priv->ratio_vec.push_back(vec);
        }
        GST_MESSAGE("Parsing definition file done!");
        
        break;
          
      }
      case PROP_ALERT_AREA_DISPLAY:
      {
        drop->priv->area_display = g_value_get_boolean(value);
        if(drop->priv->area_display)
            GST_MESSAGE("Display area is enabled!");
        break;
      }
      case PROP_QUERY:
        drop->priv->query = g_value_dup_string(value);
      break;
//     case PROP_ALERT_AREA_DEFINITION:
//     {
//         drop->priv->alert_definition_path = g_value_dup_string(value);
//         GST_MESSAGE(std::string("drop->priv->alert_definition_path = " + drop->priv->alert_definition_path).c_str());
//         
//         GST_MESSAGE("Start parsing definition file...");
//         std::ifstream infile(drop->priv->alert_definition_path);
//         if(!infile) 
//         {
//             std::cout << "Cannot open input file.\n";
//             break;
//         }
//         std::string lineString;
//         while (std::getline(infile, lineString))
//         {
//             std::vector<std::string> ratioVec = split(lineString);
//             double x = std::atof(ratioVec[0].c_str());
//             double y = std::atof(ratioVec[1].c_str());
//             
//             std::vector<double> vec;
//             vec.push_back(x);
//             vec.push_back(y);
//             drop->priv->ratio_vec.push_back(vec);
//         }
//         GST_MESSAGE("Parsing definition file done!");
//         
//         break;  
//     }
//     case PROP_ALERT_TYPE:
//     {
//         drop->priv->alertType = g_value_dup_string(value);
//         break;  
//     }
//     case PROP_ALERT_AREA_DISPLAY:
//     {
//         drop->priv->area_display = g_value_get_boolean(value);
//         if(drop->priv->area_display)
//             GST_MESSAGE("Display area is enabled!");
//         break;
//     }
//     case PROP_ALERT_PERSON_DISPLAY:
//     {
//         drop->priv->person_display = g_value_get_boolean(value);
//         if(drop->priv->person_display)
//             GST_MESSAGE("Display inferenced person is enabled!");
//         break;
//     }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
  }
}

void gst_drop_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  GstDrop *drop = GST_DROP (object);

  GST_DEBUG_OBJECT (drop, "get_property");

  switch (property_id)
  {
      case PROP_ALERT_AREA_DEFINITION:
        g_value_set_string (value, drop->priv->alert_definition_path.c_str());
        break;
//       case PROP_ALERT_TYPE:
//         //g_value_set_string (value, geofencebase->priv->alertType.c_str());
//         g_value_set_string (value, geofencebase->priv->alertType);
//        break;
      case PROP_ALERT_AREA_DISPLAY:
        g_value_set_boolean(value, drop->priv->area_display);
        break;
      case PROP_QUERY:
        g_value_set_string(value, drop->priv->query.c_str());
      break;
//     case PROP_ALERT_AREA_DEFINITION:
//        g_value_set_string (value, drop->priv->alert_definition_path.c_str());
//        break;
//     case PROP_ALERT_TYPE:
//        //g_value_set_string (value, drop->priv->alertType.c_str());
// 	   g_value_set_string (value, drop->priv->alertType);
//        break;
//     case PROP_ALERT_AREA_DISPLAY:
//        g_value_set_boolean(value, drop->priv->area_display);
//        break;
//     case PROP_ALERT_PERSON_DISPLAY:
//        g_value_set_boolean(value, drop->priv->person_display);
//        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_drop_dispose (GObject * object)
{
  GstDrop *drop = GST_DROP (object);

  GST_DEBUG_OBJECT (drop, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_drop_parent_class)->dispose (object);
}

void gst_drop_finalize (GObject * object)
{
  GstDrop *drop = GST_DROP(object);

  GST_DEBUG_OBJECT (drop, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_drop_parent_class)->finalize (object);
}

static gboolean gst_drop_start (GstBaseTransform * trans)
{
  GstDrop *drop = GST_DROP (trans);

  GST_DEBUG_OBJECT (drop, "start");
  
  return TRUE;
}

static gboolean gst_drop_stop (GstBaseTransform * trans)
{
  GstDrop *drop = GST_DROP (trans);

  GST_DEBUG_OBJECT (drop, "stop");

  return TRUE;
}

static gboolean gst_drop_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstDrop *drop = GST_DROP (filter);

  GST_DEBUG_OBJECT (drop, "set_info");
  
                
  // Start to parse the real area point
  int width = in_info->width;
  int height = in_info->height;
  drop->priv->area_point_vec.clear();
  for(uint id = 0; id < drop->priv->ratio_vec.size(); ++id)
  {
      cv::Point2d p;
      p.x = (int)(width * drop->priv->ratio_vec[id][0]);
      p.y = (int)(height * drop->priv->ratio_vec[id][1]);
      
      drop->priv->area_point_vec.push_back(p);
  }
  
  // generate the area mask
  drop->areaMaskMat = cv::Mat::zeros(height, width, CV_8UC3);
  cv::fillPoly(drop->areaMaskMat, std::vector<std::vector<cv::Point>>{drop->priv->area_point_vec}, cv::Scalar(255, 255, 255));
  // convert the area mask from gray(CV_8UC1) to RGB space(CV_8UC3).
  //cvtColor(drop->areaMaskMat, drop->areaMaskMat, cv::COLOR_GRAY2RGB);
//   imwrite("foreground/area-mask.jpg", drop->areaMaskMat);
    
  return TRUE;
}

/* transform */
// static GstFlowReturn gst_drop_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe)
// {
//   GstDrop *drop = GST_DROP (filter);
// 
//   GST_DEBUG_OBJECT (drop, "transform_frame");
// 
//   return GST_FLOW_OK;
// }

static GstFlowReturn gst_drop_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  GstDrop *drop = GST_DROP (filter);
  GstMapInfo info;

  GST_DEBUG_OBJECT (drop, "transform_frame_ip");
  gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
  // map frame data from stream to drop srcMat
  mapGstVideoFrame2OpenCVMat(drop, frame, info);
  
  
  // get detected region
  getPlaten(drop, frame);
  // test saved detected region
//   if(drop->priv->detectedROI_vec.size() > 0)
//   {
// //     std::cout << "target region number = " << drop->priv->detectedROI_vec.size() << std::endl;
//     for(int i = 0 ; i < drop->priv->detectedROI_vec.size() ; ++i)
//     {
//         graphiteRegion a = drop->priv->detectedROI_vec[i];
// //         a.show();
//     }
//   }
  
  // do algorithm: background subtractor
  doAlgorithm(drop, frame->buffer);
     
  // draw alert area
  drawAlertArea(drop);
  
  gst_buffer_unmap(frame->buffer, &info);
  return GST_FLOW_OK;
}

static void mapGstVideoFrame2OpenCVMat(GstDrop *drop, GstVideoFrame *frame, GstMapInfo &info)
{
    if(drop->srcMat.cols == 0 || drop->srcMat.rows == 0)
    {
        drop->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else if((drop->srcMat.cols != frame->info.width) || (drop->srcMat.rows != frame->info.height))
    {
        drop->srcMat.release();
        drop->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else
        drop->srcMat.data = info.data;
}

static void getPlaten(GstDrop *drop, GstVideoFrame *frame)
{
    int width = frame->info.width;
    int height = frame->info.height;
    
    drop->priv->detectedROI_vec.clear();
    
    if(width*height == 0)
        return;
    
    //std::cout << drop->priv->query << std::endl;
    std::vector<QueryResult> results = gst_buffer_adroi_query(frame->buffer, drop->priv->query);
//     std::cout << "\tresults size = " << results.size() << std::endl;
    
    // test for getting box coordination
    for(int i = 0; i < results.size(); ++i)
    {
        QueryResult queryResult = results[i];
        //std::cout << "\t=> rois size = " << queryResult.rois.size() << std::endl;
        for(auto roi: queryResult.rois)
        {
    //         std::string hash;
    //         const std::string engineID;
    //         const std::string ID;
    //         const std::string category;
    //         const float confidence;
    //         std::vector<std::string> events;
    //         std::vector<std::shared_ptr<ROI>> subROIs;
    //         std::vector<std::shared_ptr<InferenceData>> datas;
            //std::cout << "\t\thash = " << roi->hash << std::endl;
            //std::cout << "\t\tengineID = " << roi->engineID << std::endl;
            //std::cout << "\t\tID = " << roi->ID << std::endl;
            //std::cout << "\t\tcategory = " << roi->category << std::endl;
            //std::cout << "\t\tconfidence = " << roi->confidence << std::endl;
            
            if(roi->category == "box")
            {
                auto box = std::static_pointer_cast<Box>(roi);
                auto labelInfo = std::static_pointer_cast<Classification>(roi->datas.at(0));
//                 std::cout << "\t\t(x1, y1, x2, y2) = (" << box->x1 << ", " << box->y1 << ", " << box->x2 << ", " << box->y2 << "), label = " << labelInfo->label << ", confidence = " << labelInfo->confidence << std::endl;
                
                // add to element private vector
//                 if(labelInfo->label.compare("platen") == 0)
//                 {
                    int x1 = (int)(width * box->x1);
                    int y1 = (int)(height * box->y1);
                    int x2 = (int)(width * box->x2);
                    int y2 = (int)(height * box->y2);
                    int w = abs(x2 - x1) + 1;
                    int h = abs(y2 - y1) + 1;
                    drop->priv->detectedROI_vec.push_back(graphiteRegion(x1, y1, w, h, labelInfo->confidence, labelInfo->label));
//                 }
            }
            
        }
    }
}

// static void getDetectedPerson(GstDrop *drop, GstBuffer* buffer)
// {
//     // reset for each frame
//     drop->priv->person_vec.clear();
//     drop->priv->map_vec.clear();
//     
//     
//     GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
//     if (meta == NULL)
//         GST_MESSAGE("Adlink metadata is not exist!");
//     else
//     {
//         AdBatch &batch = meta->batch;
//         
//         bool frame_exist = batch.frames.size() > 0 ? true : false;
//         if(frame_exist)
//         {
//             VideoFrameData frame_info = batch.frames[0];
//                 
//             int detectionBoxResultNumber = frame_info.detection_results.size();
//             int width = drop->srcMat.cols;
//             int height = drop->srcMat.rows;
//             
//             for(int i = 0 ; i < detectionBoxResultNumber ; ++i)
//             {
//                 adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
//                 
//                 if(detection_result.obj_label.compare("person") == 0)
//                 {
//                     int x1 = (int)(width * detection_result.x1);
//                     int y1 = (int)(height * detection_result.y1);
//                     int x2 = (int)(width * detection_result.x2);
//                     int y2 = (int)(height * detection_result.y2);
//                     
//                     std::vector<cv::Point> personPoint_vec;
//                     personPoint_vec.push_back(cv::Point2d(x1, y1));
//                     personPoint_vec.push_back(cv::Point2d(x2, y1));
//                     personPoint_vec.push_back(cv::Point2d(x2, y2));
//                     personPoint_vec.push_back(cv::Point2d(x1, y2));
//                     
//                     drop->priv->person_vec.push_back(personPoint_vec);
//                     
//                     // record the mapping index
//                     drop->priv->map_vec.push_back(i);
//                     
//                     int current_id = drop->priv->person_vec.size() - 1;
//                     
//                     if(drop->priv->person_display)
//                         cv::rectangle(drop->srcMat, cv::Point(drop->priv->person_vec[current_id][0].x, drop->priv->person_vec[current_id][0].y), cv::Point(drop->priv->person_vec[current_id][2].x, drop->priv->person_vec[current_id][2].y), cv::Scalar(0,128,128), 3, cv::LINE_8);
//                         
//                 }
//             }
//         }
//     }
// }

static void calcObjectRectangle(GstDrop *drop)
{
    // erode then dilate to remove the small noise
    cv::Mat procMat;
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::erode(drop->foregroundMat, procMat, element, cv::Point(-1, -1), 1);
    cv::dilate(procMat, procMat, element, cv::Point(-1, -1), 1);
    
    
    cv::Mat canny_output;
    cv::Canny(procMat, canny_output, 50, 100);
    
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(canny_output, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    
    drop->priv->boundRect.clear();
    drop->priv->boundRect = std::vector<cv::Rect>(contours.size());
    std::vector<std::vector<cv::Point>> contours_poly(contours.size());
    for(size_t i = 0; i < contours.size(); ++i)
    {
        cv::approxPolyDP(contours[i], contours_poly[i], 3, true);
        drop->priv->boundRect[i] = cv::boundingRect(contours_poly[i]);
        //minEnclosingCircle( contours_poly[i], centers[i], radius[i] );
    }
    
    // eliminate the rectangle which is inside the other one
    if(drop->priv->boundRect.size() > 0)
    {
        int nobj = drop->priv->boundRect.size();
        for(int i = 0; i < nobj; ++i)
        {
            cv::Scalar color = cv::Scalar(255, 255, 255);
            cv::rectangle(procMat, drop->priv->boundRect[i].tl(), drop->priv->boundRect[i].br(), color, cv::FILLED);
        }
    }
    cv::Canny(procMat, canny_output, 50, 100);
    std::vector<std::vector<cv::Point> > contours2;
    cv::findContours(canny_output, contours2, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    
    drop->priv->boundRect.clear();
    drop->priv->boundRect = std::vector<cv::Rect>(contours2.size());
    std::vector<std::vector<cv::Point>> contours_poly2(contours2.size());
    for(size_t i = 0; i < contours2.size(); ++i)
    {
        cv::approxPolyDP(contours2[i], contours_poly2[i], 3, true);
        drop->priv->boundRect[i] = cv::boundingRect(contours_poly2[i]);
        //minEnclosingCircle( contours_poly[i], centers[i], radius[i] );
    }
}

int i = 0;
static void doAlgorithm(GstDrop *drop, GstBuffer* buffer)
{
    if(drop->start_learn == 0)
    {
        drop->start_learn = cv::getTickCount();
    }
    
    // if there's no detected box in detectedROI_vec, directly returen.
    int n_detectedROI = drop->priv->detectedROI_vec.size();
    n_detectedROI = 1;
    if(n_detectedROI  == 0)
        return;
    
    // If no region, return directly. Points must larger than 2.
    if(drop->priv->area_point_vec.size() <= 2)
        return;
    
    
    // only use the area to built the background model
    cv::bitwise_and(drop->srcMat, drop->areaMaskMat, drop->inspectMat);
//     imwrite("normal/" + std::to_string(i) + ".jpg", drop->inspectMat);
//     ++i;
//     return;
    
    // learning the background
    if(((cv::getTickCount() - drop->start_learn)/cv::getTickFrequency()) < drop->required_time_to_learn)
    {
        //std::cout << "begin to learn\n";
        drop->pBackSub->apply(drop->inspectMat, drop->foregroundMat);
        return;
    }
    else
    {
        // get foreground image
        //std::cout << "already learn\n";
        drop->pBackSub->apply(drop->inspectMat, drop->foregroundMat, 0.0005);
        drop->bDetect = true;
    }
    
    
//     cv::Mat bgMat;
//     drop->pBackSub->getBackgroundImage(bgMat);
//     imwrite("background/" + std::to_string(i) + "-bg.jpg", bgMat);
//     imwrite("background/" + std::to_string(i) + "-fg.jpg", drop->foregroundMat);
    
    
    // median filter to remove noise
    cv::medianBlur(drop->foregroundMat, drop->foregroundMat, 3);
    
//     // erode then dilate to remove the small noise
//     cv::Mat erodeMat;
//     cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
//     cv::erode(drop->foregroundMat, erodeMat, element, cv::Point(-1, -1), 5);
//     
//     // bilateral filter
//     cv::Mat bilateralMat;
//     cv::bilateralFilter(drop->foregroundMat, bilateralMat, 3, 3, 10);
    
    // extract the foreground object bounding boxes
    calcObjectRectangle(drop);
    
    // update the background model if no object is detected
    //std::cout << "i = " << i << ", drop->priv->boundRect.size = " << drop->priv->boundRect.size() << std::endl;
    if(drop->priv->boundRect.size() == 0)
    {
        drop->pBackSub->apply(drop->inspectMat, drop->foregroundMat);
    
        //std::cout << "\t\t\tlearn again\n";
    }
//     imwrite("foreground/" + std::to_string(i) + ".jpg", drop->foregroundMat);
    ++i;
    

//     
//     // If metadata does not exist, return directly.
//     GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
//     if (meta == NULL)
//     {
//         GST_MESSAGE("Adlink metadata is not exist!");
//         return;
//     }
//     AdBatch &batch = meta->batch;
//     bool frame_exist = batch.frames.size() > 0 ? true : false;
//     
//     // Reset region
//     if((cv::getTickCount() - drop->eventTick)/ cv::getTickFrequency() > 2)
//         drop->priv->alert = false;
//     
//     std::vector<cv::Point> intersectionPolygon;
//     int num_person_detected = drop->priv->person_vec.size();
//     for(int id = 0; id < num_person_detected; ++id)
//     {
//         float intersectArea = cv::intersectConvexConvex(drop->priv->area_point_vec, drop->priv->person_vec[id], intersectionPolygon, true);
//         
//         if(intersectArea > 0)
//         {
//             drop->eventTick = cv::getTickCount();
//             drop->priv->alert = true;
//             
//             // mark the alert data to metadata
//             if(frame_exist)
//             {
//                 // alert message format:",type<time>", must used append.
//                 std::string alertMessage = "," + std::string(drop->priv->alertType) + "<" + return_current_time_and_date() + ">";
//                 
//                 // write alert message to meta use pointer
//                 meta->batch.frames[0].detection_results[drop->priv->map_vec[id]].meta += alertMessage;
//             }
//         }
//         
//     }
}

static void drawAlertArea(GstDrop *drop)
{
    if(drop->priv->area_display)
    {
        int lineType = cv::LINE_8;
        int pt_num = drop->priv->area_point_vec.size();
        cv::Scalar color = drop->priv->alert == true ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0);
        int thickness = drop->priv->alert == true ? 6 : 2;
        for(int i = 0; i < pt_num; ++i)
            cv::line(drop->srcMat, drop->priv->area_point_vec[i%pt_num], drop->priv->area_point_vec[(i+1)%pt_num], color, thickness, lineType);
        
        // draw foreground object rectangles
        if(drop->priv->boundRect.size() > 0)
        {
            int nobj = drop->priv->boundRect.size();
            for(int i = 0; i < nobj; ++i)
            {
                //cv::Scalar color = cv::Scalar(drop->rng.uniform(0, 256), drop->rng.uniform(0, 256), drop->rng.uniform(0, 256));
                cv::Scalar color = cv::Scalar(0, 0, drop->rng.uniform(128, 256));
                cv::rectangle(drop->srcMat, drop->priv->boundRect[i].tl(), drop->priv->boundRect[i].br(), color, thickness);
            }
        }
    }
}
