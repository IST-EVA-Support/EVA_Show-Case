#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstweardetection.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include "gstadmeta.h"

//#define DEFAULT_ALERT_TYPE "Wear"
//#define DEFAULT_TARGET_TYPE "NONE"


GST_DEBUG_CATEGORY_STATIC (gst_weardetection_debug_category);
#define GST_CAT_DEFAULT gst_weardetection_debug_category

static void gst_weardetection_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_weardetection_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_weardetection_dispose (GObject * object);
static void gst_weardetection_finalize (GObject * object);

static gboolean gst_weardetection_start (GstBaseTransform * trans);
static gboolean gst_weardetection_stop (GstBaseTransform * trans);
static gboolean gst_weardetection_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
//static GstFlowReturn gst_weardetection_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_weardetection_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame);

static void mapGstVideoFrame2OpenCVMat(Gstweardetection *weardetection, GstVideoFrame *frame, GstMapInfo &info);
static void getDetectedPerson(Gstweardetection *weardetection, GstBuffer* buffer);
static void doAlgorithm(Gstweardetection *weardetection, GstBuffer* buffer);
static void drawAlertInfo(Gstweardetection *weardetection);

struct _GstweardetectionPrivate
{
    std::string alert_definition_path;
    std::vector<std::vector<double>> ratio_vec;    
    std::vector<std::vector<cv::Point>> person_vec;
    std::vector<std::vector<cv::Point>> upper_person_vec;
    std::vector<std::vector<cv::Point>> foot_vec;
    std::vector<std::vector<cv::Point>> coat_vec;
    bool wear_display;
    bool alert_display;
    bool alert;
    std::vector<int> map_vec;
    //std::string targetType;
    gchar* targetType;
	//std::string alertType;
	gchar* alertType;
    int coatNotDetectedCounter;
    int threshold_coatNotDetectedCounter;
};

enum
{
    PROP_0,
    PROP_TARGET_TYPE,
    PROP_ALERT_TYPE,
    PROP_WEAR_DISPLAY,
    PROP_ALERT_DISPLAY,
    PROP_NOT_WEAR_ACCUMULATE_COUNT,
};

std::vector<std::string> split(std::string inputString)
{
    std::regex delimiter(",");
    std::vector<std::string> list(std::sregex_token_iterator(inputString.begin(), inputString.end(), delimiter, -1), std::sregex_token_iterator());
    return list;
}

GstAdBatchMeta* gst_buffer_get_ad_batch_meta(GstBuffer* buffer)
{
    gpointer state = NULL;
    GstMeta* meta;
    const GstMetaInfo* info = GST_AD_BATCH_META_INFO;
    
    while ((meta = gst_buffer_iterate_meta (buffer, &state))) 
    {
        if (meta->info->api == info->api) 
        {
            GstAdMeta *admeta = (GstAdMeta *) meta;
            if (admeta->type == AdBatchMeta)
                return (GstAdBatchMeta*)meta;
        }
    }
    return NULL;
}

std::string return_current_time_and_date()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gstweardetection", 0, "debug category for gstweardetection element");
G_DEFINE_TYPE_WITH_CODE(Gstweardetection, gst_weardetection, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(Gstweardetection) DEBUG_INIT)

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */
static void gst_weardetection_class_init (GstweardetectionClass * klass)
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
      "Adlink Wear-Detection video filter", "Filter/Video", "An ADLINK Wear-Detection demo video filter", "Dr. Paul Lin <paul.lin@adlinktech.com> and Bernie JK Liu<berniejk.liu@adlinktech.com>");

  gobject_class->set_property = gst_weardetection_set_property;
  gobject_class->get_property = gst_weardetection_get_property;
  
  // Install the properties to GObjectClass
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TARGET_TYPE,
                                   g_param_spec_string ("target-type", "Target-Type", "The target type name used in this element for processing.", "NONE"/*DEFAULT_TARGET_TYPE*/, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_TYPE,
                                   g_param_spec_string ("alert-type", "Alert-Type", "The alert type name when event occurred.", "Wear\0"/*DEFAULT_ALERT_TYPE*/, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_NOT_WEAR_ACCUMULATE_COUNT,
                                   g_param_spec_int("accumulate-count", "Accumulate-count", "Not wear ", 0, 255, 20, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_WEAR_DISPLAY,
                                   g_param_spec_boolean("wear-display", "wear-display", "Show inferenced wearing in frame.", TRUE, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_DISPLAY,
                                   g_param_spec_boolean("alert-display", "alert-display", "Show alert box in frame.", TRUE, G_PARAM_READWRITE));
  
  gobject_class->dispose = gst_weardetection_dispose;
  gobject_class->finalize = gst_weardetection_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_weardetection_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_weardetection_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_weardetection_set_info);
  //video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_weardetection_transform_frame);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_weardetection_transform_frame_ip);

}

static void gst_weardetection_init (Gstweardetection *weardetection)
{
    /*< private >*/
    weardetection->priv = (GstweardetectionPrivate *)gst_weardetection_get_instance_private (weardetection);
    
    weardetection->priv->alert = false;
    weardetection->eventTick = 0;
    weardetection->lastEventTick = 0;
    weardetection->priv->targetType = "NONE\0";//DEFAULT_TARGET_TYPE;
    weardetection->priv->alertType = "Wear\0";//DEFAULT_ALERT_TYPE;
    weardetection->priv->coatNotDetectedCounter = 0;
    weardetection->priv->threshold_coatNotDetectedCounter = 20;
    weardetection->priv->wear_display = true;
    weardetection->priv->alert_display = true;
}

void gst_weardetection_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  Gstweardetection *weardetection = GST_weardetection (object);

  GST_DEBUG_OBJECT (weardetection, "set_property");

  switch (property_id)
  {  
    case PROP_WEAR_DISPLAY:
    {
        weardetection->priv->wear_display = g_value_get_boolean(value);
        GST_MESSAGE("Display wearing is enabled!");
        break;
    }
    case PROP_ALERT_DISPLAY:
    {
        weardetection->priv->alert_display = g_value_get_boolean(value);
        GST_MESSAGE("Display alert is enabled!");
        break;
    }
    case PROP_TARGET_TYPE:
    {
        weardetection->priv->targetType = g_value_dup_string(value);
        break;  
    }  
    case PROP_ALERT_TYPE:
    {
        weardetection->priv->alertType = g_value_dup_string(value);
        break;  
    }    
    case PROP_NOT_WEAR_ACCUMULATE_COUNT:
    {
        weardetection->priv->threshold_coatNotDetectedCounter = g_value_get_int(value);        
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
  }
}

void gst_weardetection_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  Gstweardetection *weardetection = GST_weardetection (object);

  GST_DEBUG_OBJECT (weardetection, "get_property");

  switch (property_id)
  {   
    case PROP_WEAR_DISPLAY:
       g_value_set_boolean(value, weardetection->priv->wear_display);
       break;
    case PROP_ALERT_DISPLAY:
       g_value_set_boolean(value, weardetection->priv->alert_display);
       break;
    case PROP_TARGET_TYPE:
       //g_value_set_string (value, weardetection->priv->targetType.c_str());
	   g_value_set_string (value, weardetection->priv->targetType);
       break;
    case PROP_ALERT_TYPE:
       //g_value_set_string (value, weardetection->priv->alertType.c_str());
	   g_value_set_string (value, weardetection->priv->alertType);
       break;    
    case PROP_NOT_WEAR_ACCUMULATE_COUNT:
       g_value_set_int(value, weardetection->priv->threshold_coatNotDetectedCounter);
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_weardetection_dispose (GObject * object)
{
  Gstweardetection *weardetection = GST_weardetection (object);

  GST_DEBUG_OBJECT (weardetection, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_weardetection_parent_class)->dispose (object);
}

void gst_weardetection_finalize (GObject * object)
{
  Gstweardetection *weardetection = GST_weardetection(object);

  GST_DEBUG_OBJECT (weardetection, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_weardetection_parent_class)->finalize (object);
}

static gboolean gst_weardetection_start (GstBaseTransform * trans)
{
  Gstweardetection *weardetection = GST_weardetection (trans);

  GST_DEBUG_OBJECT (weardetection, "start");
  
  return TRUE;
}

static gboolean gst_weardetection_stop (GstBaseTransform * trans)
{
  Gstweardetection *weardetection = GST_weardetection (trans);

  GST_DEBUG_OBJECT (weardetection, "stop");

  return TRUE;
}

static gboolean gst_weardetection_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  Gstweardetection *weardetection = GST_weardetection (filter);

  GST_DEBUG_OBJECT (weardetection, "set_info");
  
  return TRUE;
}

/* transform */
// static GstFlowReturn gst_weardetection_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe)
// {
//   Gstweardetection *weardetection = GST_weardetection (filter);
// 
//   GST_DEBUG_OBJECT (weardetection, "transform_frame");
// 
//   return GST_FLOW_OK;
// }

static GstFlowReturn gst_weardetection_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  Gstweardetection *weardetection = GST_weardetection (filter);
  GstMapInfo info;

  GST_DEBUG_OBJECT (weardetection, "transform_frame_ip");
  gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
  // map frame data from stream to weardetection srcMat
  mapGstVideoFrame2OpenCVMat(weardetection, frame, info);
  
  // get inference detected persons
  getDetectedPerson(weardetection, frame->buffer);
  
  // do algorithm
  doAlgorithm(weardetection, frame->buffer);  
  
  // draw alert person
  drawAlertInfo(weardetection);
  
  if(weardetection->priv->alert == true)
      weardetection->priv->coatNotDetectedCounter = 0;
  
  gst_buffer_unmap(frame->buffer, &info);
  return GST_FLOW_OK;
}

static void mapGstVideoFrame2OpenCVMat(Gstweardetection *weardetection, GstVideoFrame *frame, GstMapInfo &info)
{
    if(weardetection->srcMat.cols == 0 || weardetection->srcMat.rows == 0)
        weardetection->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    else if((weardetection->srcMat.cols != frame->info.width) || (weardetection->srcMat.rows != frame->info.height))
    {
        weardetection->srcMat.release();
        weardetection->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else
        weardetection->srcMat.data = info.data;
}

static void getDetectedPerson(Gstweardetection *weardetection, GstBuffer* buffer)
{
    // reset for each frame
    weardetection->priv->person_vec.clear();
    weardetection->priv->upper_person_vec.clear();
    weardetection->priv->foot_vec.clear();
    weardetection->priv->map_vec.clear();
    weardetection->priv->coat_vec.clear();
    
    // image boundry 
    cv::Rect boundry(0, 0, weardetection->srcMat.cols, weardetection->srcMat.rows);
    
    
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
            int width = weardetection->srcMat.cols;
            int height = weardetection->srcMat.rows;
            
            for(int i = 0 ; i < detectionBoxResultNumber ; ++i)
            {
                adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
                
                int x1 = (int)(width * detection_result.x1);
                int y1 = (int)(height * detection_result.y1);
                int x2 = (int)(width * detection_result.x2);
                int y2 = (int)(height * detection_result.y2);
                
                if(detection_result.obj_label.compare("person") == 0)
                {
                    if(detection_result.meta.find(weardetection->priv->targetType) != std::string::npos || std::string(weardetection->priv->targetType).compare("NONE"/*DEFAULT_TARGET_TYPE*/) == 0)
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
                        
                        weardetection->priv->person_vec.push_back(personPoint_vec);
                        weardetection->priv->foot_vec.push_back(footPoint_vec);
                        weardetection->priv->upper_person_vec.push_back(upper_personPoint_vec);
                        
                        // record the mapping index
                        weardetection->priv->map_vec.push_back(i);
                    }
                }
                else if(detection_result.obj_label.compare("coat") == 0)
                {    
                    std::vector<cv::Point> coatPoint_vec;
                    cv::Rect coatRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
                    coatRect = coatRect & boundry;
                    coatPoint_vec.push_back(cv::Point2d(coatRect.x, coatRect.y));
                    coatPoint_vec.push_back(cv::Point2d(coatRect.x + coatRect.width - 1, coatRect.y));
                    coatPoint_vec.push_back(cv::Point2d(coatRect.x + coatRect.width - 1, coatRect.y + coatRect.height - 1));
                    coatPoint_vec.push_back(cv::Point2d(coatRect.x, coatRect.y + coatRect.height - 1));
                    
                    weardetection->priv->coat_vec.push_back(coatPoint_vec);

                    int current_id = weardetection->priv->coat_vec.size() - 1;
                    
                    if(weardetection->priv->wear_display)
                    {
                        int roiWidth = weardetection->priv->coat_vec[current_id][1].x - weardetection->priv->coat_vec[current_id][0].x + 1;
                        int roiHeight = weardetection->priv->coat_vec[current_id][2].y - weardetection->priv->coat_vec[current_id][0].y + 1;
                        cv::Mat roi = weardetection->srcMat(cv::Rect(weardetection->priv->coat_vec[current_id][0].x, weardetection->priv->coat_vec[current_id][0].y, roiWidth, roiHeight));
                        
                        double alpha = 0.3;
                        cv::Mat color(roi.size(), CV_8UC3, cv::Scalar(124, 252, 0));
                        cv::addWeighted(color, alpha, roi, 1.0 - alpha , 0.0, roi);
                    }                                                                           
                }                
            }
        }
    }
}

static void doAlgorithm(Gstweardetection *weardetection, GstBuffer* buffer)
{    
    // If metadata does not exist, return directly.
    GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
    if (meta == NULL)
    {
        g_message("Adlink metadata is not exist!");
        return;
    }
    AdBatch &batch = meta->batch;
    bool frame_exist = batch.frames.size() > 0 ? true : false;
    
    // Reset region from alarm to normal after 2 seconds.
    if((cv::getTickCount() - weardetection->eventTick)/ cv::getTickFrequency() > 2)
        weardetection->priv->alert = false;
    
    std::vector<cv::Point> intersectionPolygon;

    VideoFrameData frame_info = batch.frames[0];
    int detectionBoxResultNumber = frame_info.detection_results.size();
    bool personWithCoat = false;
    int num_coat_detected = weardetection->priv->coat_vec.size();
    int num_person_detected = weardetection->priv->person_vec.size();
    
    for(int id = 0; id < num_person_detected; ++id)
    {    
        for(int coatId = 0; coatId < num_coat_detected; ++coatId)
        {
            float intersectArea = cv::intersectConvexConvex(weardetection->priv->coat_vec[coatId], weardetection->priv->upper_person_vec[id], intersectionPolygon, true);
         
            if(intersectArea > 0)
            {
                personWithCoat = true; 
                weardetection->priv->coatNotDetectedCounter = 0;
                weardetection->priv->alert = false;
                break;
            }
        }

        if(!personWithCoat)
            weardetection->priv->coatNotDetectedCounter++; 
        

        if(weardetection->priv->coatNotDetectedCounter > weardetection->priv->threshold_coatNotDetectedCounter)
        {
            weardetection->eventTick = cv::getTickCount();
            weardetection->priv->alert = true;

            // mark the alert data to metadata
            if(frame_exist)
            {
                // alert message format:",type<time>", must use append.
                std::string alertMessage = "," + std::string(weardetection->priv->alertType) + "<" + return_current_time_and_date() + ">";
                meta->batch.frames[0].detection_results[weardetection->priv->map_vec[id]].meta += alertMessage;
            }
        }
    }
}

static void drawAlertInfo(Gstweardetection *weardetection)
{
    int thickness = 3;
    int num_person_detected = weardetection->priv->person_vec.size();
    
    for(int id = 0; id < num_person_detected; ++id)
    {        
        if(weardetection->priv->alert == true && weardetection->priv->alert_display == true)
        {
            cv::Scalar color = cv::Scalar(0, 0, 255);

            int thickness = 3;
            int num_person_detected = weardetection->priv->person_vec.size();            
            cv::rectangle(weardetection->srcMat, cv::Point(weardetection->priv->person_vec[id][0].x, weardetection->priv->person_vec[id][0].y), cv::Point(weardetection->priv->person_vec[id][2].x, weardetection->priv->person_vec[id][2].y), color, thickness, cv::LINE_8);
        }

        cv::putText(weardetection->srcMat, "Not wear accumulation: "  + std::to_string(weardetection->priv->coatNotDetectedCounter), weardetection->priv->person_vec[id][0], cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar::all(255), 1);
    }
}

static gboolean plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "weardetection", GST_RANK_NONE, GST_TYPE_weardetection);
}

#ifndef VERSION
#define VERSION "1.1.0"
#endif
#ifndef PACKAGE
#define PACKAGE "An ADLINK Wear-Detection plugin"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "libweardetection.so"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://www.adlinktech.com/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    weardetection,
    PACKAGE,
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

