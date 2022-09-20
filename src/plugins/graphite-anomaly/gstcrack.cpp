#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstcrack.h"
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



GST_DEBUG_CATEGORY_STATIC (gst_crack_debug_category);
#define GST_CAT_DEFAULT gst_crack_debug_category

static void gst_crack_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_crack_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_crack_dispose (GObject * object);
static void gst_crack_finalize (GObject * object);

static gboolean gst_crack_start (GstBaseTransform * trans);
static gboolean gst_crack_stop (GstBaseTransform * trans);
static gboolean gst_crack_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
//static GstFlowReturn gst_crack_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_crack_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame);

static void mapGstVideoFrame2OpenCVMat(GstCrack *crack, GstVideoFrame *frame, GstMapInfo &info);
static void getGraphiteHoleRegion(GstCrack *crack, GstVideoFrame *frame);
// static void getDetectedPerson(GstCrack *crack, GstBuffer* buffer);
static void doAlgorithm(GstCrack *crack, GstBuffer* buffer);
// static void drawAlertArea(GstCrack *crack);

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

struct _GstCrackPrivate
{
    std::string query;
    std::vector<graphiteRegion> detectedROI_vec; 
//     std::string alert_definition_path;
//     std::vector<std::vector<double>> ratio_vec;
//     std::vector<cv::Point> area_point_vec;
//     std::vector<std::vector<cv::Point>> person_vec;
//     bool area_display;
//     bool person_display;
//     bool alert;
//     std::vector<int> map_vec;
//     //std::string alertType;
// 	gchar* alertType;
};

enum
{
    PROP_0,
    PROP_QUERY,
//     PROP_ALERT_AREA_DEFINITION,
//     PROP_ALERT_AREA_DISPLAY,
//     PROP_ALERT_PERSON_DISPLAY,
//     PROP_ALERT_TYPE,
};

#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gstcrack", 0, "debug category for gstcrack element");
G_DEFINE_TYPE_WITH_CODE(GstCrack, gst_crack, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(GstCrack) DEBUG_INIT)

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */
static void gst_crack_class_init (GstCrackClass * klass)
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
      "Adlink Graphite anomaly crack video filter", "Filter/Video", "An ADLINK Graphite anomaly crack demo video filter", "Dr. Paul Lin <paul.lin@adlinktech.com>");

  gobject_class->set_property = gst_crack_set_property;
  gobject_class->get_property = gst_crack_get_property;
  
  // Install the properties to GObjectClass
    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_QUERY,
                                   g_param_spec_string ("query", "Query", "The query string to acquire the inference data from adlink metadata.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
//   g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DEFINITION,
//                                    g_param_spec_string ("alert-area-def", "Alert-area-def", "The definition file location of the alert area respect the frame based on the specific resolution.", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
//   
//   g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_TYPE,
//                                    g_param_spec_string ("alert-type", "Alert-Type", "The alert type name when event occurred.", "BreakIn\0"/*DEFAULT_ALERT_TYPE*/, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
//   
//   g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_AREA_DISPLAY,
//                                    g_param_spec_boolean("area-display", "Area-display", "Show alert area in frame.", FALSE, G_PARAM_READWRITE));
//   
//   g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_PERSON_DISPLAY,
//                                    g_param_spec_boolean("person-display", "Person-display", "Show inferenced person region in frame.", FALSE, G_PARAM_READWRITE));
  
  
  gobject_class->dispose = gst_crack_dispose;
  gobject_class->finalize = gst_crack_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_crack_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_crack_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_crack_set_info);
  //video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_crack_transform_frame);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_crack_transform_frame_ip);

}

static void gst_crack_init (GstCrack *crack)
{
    /*< private >*/
    crack->priv = (GstCrackPrivate *)gst_crack_get_instance_private (crack);
    

    crack->priv->query = "//yolov4tiny[class in holeregion]";
//     crack->priv->area_display = false;
//     crack->priv->person_display = false;
//     crack->priv->alert = false;
    crack->eventTick = 0;
    crack->lastEventTick = 0;
//     crack->priv->alertType = "BreakIn\0";//DEFAULT_ALERT_TYPE;
}

void gst_crack_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstCrack *crack = GST_CRACK (object);

  GST_DEBUG_OBJECT (crack, "set_property");

  switch (property_id)
  {
      case PROP_QUERY:
        crack->priv->query = g_value_dup_string(value);
      break;
//     case PROP_ALERT_AREA_DEFINITION:
//     {
//         crack->priv->alert_definition_path = g_value_dup_string(value);
//         GST_MESSAGE(std::string("crack->priv->alert_definition_path = " + crack->priv->alert_definition_path).c_str());
//         
//         GST_MESSAGE("Start parsing definition file...");
//         std::ifstream infile(crack->priv->alert_definition_path);
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
//             crack->priv->ratio_vec.push_back(vec);
//         }
//         GST_MESSAGE("Parsing definition file done!");
//         
//         break;  
//     }
//     case PROP_ALERT_TYPE:
//     {
//         crack->priv->alertType = g_value_dup_string(value);
//         break;  
//     }
//     case PROP_ALERT_AREA_DISPLAY:
//     {
//         crack->priv->area_display = g_value_get_boolean(value);
//         if(crack->priv->area_display)
//             GST_MESSAGE("Display area is enabled!");
//         break;
//     }
//     case PROP_ALERT_PERSON_DISPLAY:
//     {
//         crack->priv->person_display = g_value_get_boolean(value);
//         if(crack->priv->person_display)
//             GST_MESSAGE("Display inferenced person is enabled!");
//         break;
//     }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
  }
}

void gst_crack_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  GstCrack *crack = GST_CRACK (object);

  GST_DEBUG_OBJECT (crack, "get_property");

  switch (property_id)
  {
      case PROP_QUERY:
        g_value_set_string(value, crack->priv->query.c_str());
      break;
//     case PROP_ALERT_AREA_DEFINITION:
//        g_value_set_string (value, crack->priv->alert_definition_path.c_str());
//        break;
//     case PROP_ALERT_TYPE:
//        //g_value_set_string (value, crack->priv->alertType.c_str());
// 	   g_value_set_string (value, crack->priv->alertType);
//        break;
//     case PROP_ALERT_AREA_DISPLAY:
//        g_value_set_boolean(value, crack->priv->area_display);
//        break;
//     case PROP_ALERT_PERSON_DISPLAY:
//        g_value_set_boolean(value, crack->priv->person_display);
//        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_crack_dispose (GObject * object)
{
  GstCrack *crack = GST_CRACK (object);

  GST_DEBUG_OBJECT (crack, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_crack_parent_class)->dispose (object);
}

void gst_crack_finalize (GObject * object)
{
  GstCrack *crack = GST_CRACK(object);

  GST_DEBUG_OBJECT (crack, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_crack_parent_class)->finalize (object);
}

static gboolean gst_crack_start (GstBaseTransform * trans)
{
  GstCrack *crack = GST_CRACK (trans);

  GST_DEBUG_OBJECT (crack, "start");
  
  return TRUE;
}

static gboolean gst_crack_stop (GstBaseTransform * trans)
{
  GstCrack *crack = GST_CRACK (trans);

  GST_DEBUG_OBJECT (crack, "stop");

  return TRUE;
}

static gboolean gst_crack_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstCrack *crack = GST_CRACK (filter);

  GST_DEBUG_OBJECT (crack, "set_info");
  
                
//   // Start to parse the real area point
//   int width = in_info->width;
//   int height = in_info->height;
//   for(uint id = 0; id < crack->priv->ratio_vec.size(); ++id)
//   {
//       cv::Point2d p;
//       p.x = (int)(width * crack->priv->ratio_vec[id][0]);
//       p.y = (int)(height * crack->priv->ratio_vec[id][1]);
//       
//       crack->priv->area_point_vec.push_back(p);
//   }
  return TRUE;
}

/* transform */
// static GstFlowReturn gst_crack_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe)
// {
//   GstCrack *crack = GST_CRACK (filter);
// 
//   GST_DEBUG_OBJECT (crack, "transform_frame");
// 
//   return GST_FLOW_OK;
// }

static GstFlowReturn gst_crack_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  GstCrack *crack = GST_CRACK (filter);
  GstMapInfo info;

  GST_DEBUG_OBJECT (crack, "transform_frame_ip");
  gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
  // map frame data from stream to crack srcMat
  mapGstVideoFrame2OpenCVMat(crack, frame, info);
  
  
  // get detected region
  getGraphiteHoleRegion(crack, frame);
  // test saved detected region
  if(crack->priv->detectedROI_vec.size() > 0)
  {
    std::cout << "target region number = " << crack->priv->detectedROI_vec.size() << std::endl;
    for(int i = 0 ; i < crack->priv->detectedROI_vec.size() ; ++i)
    {
        graphiteRegion a = crack->priv->detectedROI_vec[i];
        a.show();
    }
  }
  
  
  
//   // get inference detected persons
//   getDetectedPerson(crack, frame->buffer);
//   
//   // do algorithm
  doAlgorithm(crack, frame->buffer);
//   
//   // draw alert area
//   drawAlertArea(crack);
  
  gst_buffer_unmap(frame->buffer, &info);
  return GST_FLOW_OK;
}

static void mapGstVideoFrame2OpenCVMat(GstCrack *crack, GstVideoFrame *frame, GstMapInfo &info)
{
    if(crack->srcMat.cols == 0 || crack->srcMat.rows == 0)
        crack->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    else if((crack->srcMat.cols != frame->info.width) || (crack->srcMat.rows != frame->info.height))
    {
        crack->srcMat.release();
        crack->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else
        crack->srcMat.data = info.data;
}

static void getGraphiteHoleRegion(GstCrack *crack, GstVideoFrame *frame)
{
    int width = frame->info.width;
    int height = frame->info.height;
    
    crack->priv->detectedROI_vec.clear();
    
    if(width*height == 0)
        return;
    
    //std::cout << crack->priv->query << std::endl;
    std::vector<QueryResult> results = gst_buffer_adroi_query(frame->buffer, crack->priv->query);
    //std::cout << "\tresults size = " << results.size() << std::endl;
    
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
                std::cout << "\t\t(x1, y1, x2, y2) = (" << box->x1 << ", " << box->y1 << ", " << box->x2 << ", " << box->y2 << "), label = " << labelInfo->label << ", confidence = " << labelInfo->confidence << std::endl;
                
                // add to element private vector
                if(labelInfo->label.compare("holeregion") == 0)
                {
                    int x1 = (int)(width * box->x1);
                    int y1 = (int)(height * box->y1);
                    int x2 = (int)(width * box->x2);
                    int y2 = (int)(height * box->y2);
                    int w = abs(x2 - x1) + 1;
                    int h = abs(y2 - y1) + 1;
                    crack->priv->detectedROI_vec.push_back(graphiteRegion(x1, y1, w, h, labelInfo->confidence, labelInfo->label));
                }
            }
            
        }
    }
}

// static void getDetectedPerson(GstCrack *crack, GstBuffer* buffer)
// {
//     // reset for each frame
//     crack->priv->person_vec.clear();
//     crack->priv->map_vec.clear();
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
//             int width = crack->srcMat.cols;
//             int height = crack->srcMat.rows;
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
//                     crack->priv->person_vec.push_back(personPoint_vec);
//                     
//                     // record the mapping index
//                     crack->priv->map_vec.push_back(i);
//                     
//                     int current_id = crack->priv->person_vec.size() - 1;
//                     
//                     if(crack->priv->person_display)
//                         cv::rectangle(crack->srcMat, cv::Point(crack->priv->person_vec[current_id][0].x, crack->priv->person_vec[current_id][0].y), cv::Point(crack->priv->person_vec[current_id][2].x, crack->priv->person_vec[current_id][2].y), cv::Scalar(0,128,128), 3, cv::LINE_8);
//                         
//                 }
//             }
//         }
//     }
// }

int aa = 0;
static void doAlgorithm(GstCrack *crack, GstBuffer* buffer)
{
    // if there's no detected box in detectedROI_vec, directly returen.
    int n_detectedROI = crack->priv->detectedROI_vec.size();
    if(n_detectedROI  == 0)
        return;
    
    for(int i = 0; i < n_detectedROI; ++i)
    {
        graphiteRegion box = crack->priv->detectedROI_vec[i];
        cv::Mat dstMat = crack->srcMat(cv::Rect(box.x, box.y, box.width, box.height));
        cv::Mat gray_dstMat;
        cv::cvtColor(dstMat, gray_dstMat, cv::COLOR_BGR2GRAY);
        cv::Mat cannyMat;
        cv::Canny(gray_dstMat, cannyMat, 50, 200, 3);
        
//         cv::imwrite("crack-images/MVI_2783/" + std::to_string(aa) + ".png", dstMat);
        aa++;
    }
    
//     // If no region, return directly. Points must larger than 2.
//     if(crack->priv->area_point_vec.size() <= 2)
//         return;
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
//     if((cv::getTickCount() - crack->eventTick)/ cv::getTickFrequency() > 2)
//         crack->priv->alert = false;
//     
//     std::vector<cv::Point> intersectionPolygon;
//     int num_person_detected = crack->priv->person_vec.size();
//     for(int id = 0; id < num_person_detected; ++id)
//     {
//         float intersectArea = cv::intersectConvexConvex(crack->priv->area_point_vec, crack->priv->person_vec[id], intersectionPolygon, true);
//         
//         if(intersectArea > 0)
//         {
//             crack->eventTick = cv::getTickCount();
//             crack->priv->alert = true;
//             
//             // mark the alert data to metadata
//             if(frame_exist)
//             {
//                 // alert message format:",type<time>", must used append.
//                 std::string alertMessage = "," + std::string(crack->priv->alertType) + "<" + return_current_time_and_date() + ">";
//                 
//                 // write alert message to meta use pointer
//                 meta->batch.frames[0].detection_results[crack->priv->map_vec[id]].meta += alertMessage;
//             }
//         }
//         
//     }
}

// static void drawAlertArea(GstCrack *crack)
// {
//     if(crack->priv->area_display)
//     {
//         int lineType = cv::LINE_8;
//         int pt_num = crack->priv->area_point_vec.size();
//         cv::Scalar color = crack->priv->alert == true ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0);
//         int thickness = crack->priv->alert == true ? 6 : 2;
//         for(int i = 0; i < pt_num; ++i)
//             cv::line(crack->srcMat, crack->priv->area_point_vec[i%pt_num], crack->priv->area_point_vec[(i+1)%pt_num], color, thickness, lineType);
//     }
// }
