#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstpartassembly.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include "gstadmeta.h"
#include "utils.h"
#define NANO_SECOND 1000000000.0

GST_DEBUG_CATEGORY_STATIC (gst_partassembly_debug_category);
#define GST_CAT_DEFAULT gst_partassembly_debug_category

static void gst_partassembly_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_partassembly_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_partassembly_dispose (GObject * object);
static void gst_partassembly_finalize (GObject * object);

static void gst_partassembly_before_transform (GstBaseTransform * trans, GstBuffer * buffer);
static gboolean gst_partassembly_start (GstBaseTransform * trans);
static gboolean gst_partassembly_stop (GstBaseTransform * trans);
static gboolean gst_partassembly_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_partassembly_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame);

static void mapGstVideoFrame2OpenCVMat(Gstpartassembly *partassembly, GstVideoFrame *frame, GstMapInfo &info);
static void getDetectedBox(Gstpartassembly *partassembly, GstBuffer* buffer);
static int getPartIndexInBomList(Gstpartassembly *partassembly, const std::string& partName);
static bool twoSidesCheck(Gstpartassembly *partassembly, int partIndex, int eachSideExistNumber);
static void doAlgorithm(Gstpartassembly *partassembly, GstBuffer* buffer);
static void drawObjects(Gstpartassembly *partassembly);
static void drawStatus(Gstpartassembly *partassembly);

std::vector<bool> assemblyActionVector = {
    false,  // put 1 semi-product in container
    false,  // put 2 light-guide-cover  in semi-finished-products(left and right)
    false,  // put 2 small-board-side-B in  semi-finished-products(left and right)
    false,  // screw on 4 screws(2 on left, 2 on right)
    false,  // put wire on 
    false,  // final visual inspection: 2 small-board-side-B with 4 screw-on(each small-board-side-B contains 2 screw-on) and 1 wire
    false}; // Complete

std::vector<double> processingTime = {
    0,  // put 1 semi-product in container
    0,  // put 2 light-guide-cover in semi-finished-products(left and right)
    0,  // put 2 small-board-side-B in semi-finished-products(left and right)
    0,  // screw on 4 screws(2 on left, 2 on right)
    0,  // put wire on
    0,  // final visual inspection: 2 small-board-side-B with 4 screw-on(each small-board-side-B contains 2 screw-on) and 1 wire
    0}; // Complete
    
std::vector<float> processingRegularTime = {
    10,  // put 1 semi-product in container
    10,  // put 2 light-guide-cover in semi-finished-products(left and right)
    6,  // put 2 small-board-side-B in semi-finished-products(left and right)
    10, // screw on 4 screws(2 on left, 2 on right)
    5,  // put wire on
    5,  // final visual inspection: 2 small-board-side-B with 4 screw-on(each small-board-side-B contains 2 screw-on) and 1 wire
    5}; // Complete
    
std::vector<std::string> assemblyActionInfoVector = {
    "semi-product in container",                // put 1 semi-product in container
    "2 light-guide-cover(left and right)",      // put 2 light-guide-cover in semi-finished-products(left and right)
    "2 small-board-side-B(left and right)",     // put 2 small-board-side-B in semi-finished-products(left and right)
    "screw on 4 screws(2 on left, 2 on right)", // screw on 4 screws(2 on left, 2 on right)
    "put wire on",                              // put wire on 
    "final visual inspection",                  // final visual inspection: 2 small-board-side-B with 4 screw-on(each small-board-side-B contains 2 screw-on) and 1 wire
    "completion"};                              // Complete

struct _GstpartassemblyPrivate
{
    std::vector<std::string> bomList;
    std::vector<Material*> bomMaterial;
    int indexOfSemiProductContainer;
    int indexOfSemiProduct;
    int bestIndexObjectOfSemiProduct;
    
    bool partsDisplay;
    bool informationDisplay;
    bool alert;
    gchar* targetType;
    gchar* alertType;
};

enum
{
    PROP_0,
    PROP_TARGET_TYPE,
    PROP_ALERT_TYPE,
    PROP_PARTS_DISPLAY,
    PROP_INFORMATION_DISPLAY
};

#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gstpartassembly", 0, "debug category for gstpartassembly element");
G_DEFINE_TYPE_WITH_CODE(Gstpartassembly, gst_partassembly, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(Gstpartassembly) DEBUG_INIT)

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */
static void gst_partassembly_class_init (GstpartassemblyClass * klass)
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
      "Adlink part assembly video filter", "Filter/Video", "An ADLINK Part-Assembly demo video filter", "Dr. Paul Lin <paul.lin@adlinktech.com>");

  gobject_class->set_property = gst_partassembly_set_property;
  gobject_class->get_property = gst_partassembly_get_property;
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TARGET_TYPE,
                                   g_param_spec_string ("target-type", "Target-Type", "The target type name used in this element for processing.", "NONE"/*DEFAULT_TARGET_TYPE*/, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_TYPE,
                                   g_param_spec_string ("alert-type", "Alert-Type", "The alert type name represents the event occurred. Two alert types are offered:\n\t\t\t(1)\"idling\", which means OP is idling;\n\t\t\t(2)\"completed\", which means the assembly is completed.", "idling\0", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PARTS_DISPLAY,
                                   g_param_spec_boolean("parts-display", "Parts-display", "Show detected parts in frame.", TRUE, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INFORMATION_DISPLAY,
                                   g_param_spec_boolean("information-display", "Information-display", "Show the assembly process status.", TRUE, G_PARAM_READWRITE));
  
  gobject_class->dispose = gst_partassembly_dispose;
  gobject_class->finalize = gst_partassembly_finalize;
  base_transform_class->before_transform = GST_DEBUG_FUNCPTR(gst_partassembly_before_transform);
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_partassembly_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_partassembly_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_partassembly_set_info);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_partassembly_transform_frame_ip);

}

static void gst_partassembly_init (Gstpartassembly *partassembly)
{
    /*< private >*/
    partassembly->priv = (GstpartassemblyPrivate *)gst_partassembly_get_instance_private (partassembly);
    
    // *** define the required parts name, their required number and order to assemble
    std::vector<std::string> partNameVector = {
        "background", 
        "container-parts", 
        "container-semi-finished-products", 
        "light-guide-cover", 
        "screw", 
        "screwed-on", 
        "semi-finished-products", 
        "small-board-side-A", 
        "small-board-side-B", 
        "wire"};
    std::vector<int>  partRequiredNumberVector = {0,  0,  1, 2,  0, 4, 1,  0, 2, 1};
    std::vector<int>  partAssembleOrderVector = {-1, -1, -1, 1, -1, 3, 0, -1, 2, 4}; // -1: omit to check; 0~n: assemble order
    partassembly->bom = BASIC_INFORMATION::BOM(partNameVector, partRequiredNumberVector, partAssembleOrderVector);
    // ***

    // initialize the materials
    for(unsigned int i = 0; i < partassembly->bom.NameVector.size(); i++)
    {
        if(partRequiredNumberVector[i] > 0)
        {
            partassembly->priv->bomList.push_back(partassembly->bom.NameVector[i]);
            partassembly->priv->bomMaterial.push_back(new Material(partassembly->bom.NumberVector[i], partassembly->bom.OrderVector[i]));
        }
    }
    
    
    partassembly->priv->partsDisplay = true;
    partassembly->priv->informationDisplay = true;
    partassembly->priv->alert = false;
    partassembly->targetTypeChecked = false;
    partassembly->startTick = 0;
    partassembly->alertTick = 0;
    partassembly->runningTime = 0;
    partassembly->priv->targetType = "NONE\0";//DEFAULT_TARGET_TYPE;
    partassembly->priv->alertType = "idling\0";//DEFAULT_ALERT_TYPE;
    
    for(unsigned int i = 0; i < assemblyActionVector.size() - 1 ; ++i)
    {
        assemblyActionVector[i] = false;
        processingTime[i] = 0;
    }
    
    partassembly->emptyCounter = 0;
    partassembly->partContainerIsEmpty = false;
}

void gst_partassembly_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY (object);

  GST_DEBUG_OBJECT (partassembly, "set_property");

  switch (property_id)
  {
    case PROP_TARGET_TYPE:
    {
        partassembly->priv->targetType = g_value_dup_string(value);
        break;  
    }
    case PROP_ALERT_TYPE:
    {
        partassembly->priv->alertType = g_value_dup_string(value);
        break;  
    }
    case PROP_PARTS_DISPLAY:
    {
        partassembly->priv->partsDisplay = g_value_get_boolean(value);
        if(partassembly->priv->partsDisplay)
            GST_MESSAGE("Display part objects is enabled!");
        break;
    }
    case PROP_INFORMATION_DISPLAY:
    {
        partassembly->priv->informationDisplay = g_value_get_boolean(value);
        if(partassembly->priv->informationDisplay)
            GST_MESSAGE("Display assembly information is enabled!");
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
  }
}

void gst_partassembly_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY (object);

  GST_DEBUG_OBJECT (partassembly, "get_property");

  switch (property_id)
  {
    case PROP_TARGET_TYPE:
       //g_value_set_string (value, weardetection->priv->targetType.c_str());
	   g_value_set_string (value, partassembly->priv->targetType);
       break;
    case PROP_ALERT_TYPE:
       //g_value_set_string (value, partassembly->priv->alertType.c_str());
	   g_value_set_string (value, partassembly->priv->alertType);
       break;
    case PROP_PARTS_DISPLAY:
       g_value_set_boolean(value, partassembly->priv->partsDisplay);
       break;
    case PROP_INFORMATION_DISPLAY:
       g_value_set_boolean(value, partassembly->priv->informationDisplay);
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_partassembly_dispose (GObject * object)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY (object);

  GST_DEBUG_OBJECT (partassembly, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_partassembly_parent_class)->dispose (object);
}

void gst_partassembly_finalize (GObject * object)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY(object);

  GST_DEBUG_OBJECT (partassembly, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_partassembly_parent_class)->finalize (object);
}

static gboolean gst_partassembly_start (GstBaseTransform * trans)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY (trans);
  
  GST_DEBUG_OBJECT (partassembly, "start");
  
  return TRUE;
}

static gboolean gst_partassembly_stop (GstBaseTransform * trans)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY (trans);

  partassembly->priv->alert = false;
  partassembly->targetTypeChecked = false;
  partassembly->startTick = 0;
  for(unsigned int i = 0; i < assemblyActionVector.size() - 1 ; ++i)
  {
      assemblyActionVector[i] = false;
      processingTime[i] = 0;
  }
  
  partassembly->emptyCounter = 0;
  partassembly->partContainerIsEmpty = false;
  
  GST_DEBUG_OBJECT (partassembly, "stop");

  return TRUE;
}

static gboolean gst_partassembly_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY (filter);

  GST_DEBUG_OBJECT (partassembly, "set_info");
  
  return TRUE;
}

static void gst_partassembly_before_transform (GstBaseTransform * trans, GstBuffer * buffer)
{
    long base_time = (GST_ELEMENT (trans))->base_time;
    long current_time = gst_clock_get_time((GST_ELEMENT (trans))->clock);
    Gstpartassembly *partassembly = GST_PARTASSEMBLY (trans);
    partassembly->runningTime = (current_time - base_time)/NANO_SECOND;
}

static GstFlowReturn gst_partassembly_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  Gstpartassembly *partassembly = GST_PARTASSEMBLY (filter);
  GstMapInfo info;

  GST_DEBUG_OBJECT (partassembly, "transform_frame_ip");
  gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
  // map frame data from stream to partassembly srcMat
  mapGstVideoFrame2OpenCVMat(partassembly, frame, info);
  
  // get inference detected persons
  getDetectedBox(partassembly, frame->buffer);
  
  // do algorithm
  doAlgorithm(partassembly, frame->buffer);  
  
  // draw part objects
  if(partassembly->priv->partsDisplay)
    drawObjects(partassembly);
  
  // draw assembly information
  if(partassembly->priv->informationDisplay)
    drawStatus(partassembly);
  
  gst_buffer_unmap(frame->buffer, &info);
  return GST_FLOW_OK;
}

static void mapGstVideoFrame2OpenCVMat(Gstpartassembly *partassembly, GstVideoFrame *frame, GstMapInfo &info)
{
    if(partassembly->srcMat.cols == 0 || partassembly->srcMat.rows == 0)
        partassembly->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    else if((partassembly->srcMat.cols != frame->info.width) || (partassembly->srcMat.rows != frame->info.height))
    {
        partassembly->srcMat.release();
        partassembly->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else
        partassembly->srcMat.data = info.data;
}

static void getDetectedBox(Gstpartassembly *partassembly, GstBuffer* buffer)
{
    // reset the status
    for (auto& value : partassembly->priv->bomMaterial)
    {
        value->ClearStatus();
    }
    partassembly->priv->indexOfSemiProductContainer = -1;
    
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
            int width = partassembly->srcMat.cols;
            int height = partassembly->srcMat.rows;
            
            // full iterated all object to find "container-parts" exists alert-type(target-type used in this element)
            if(partassembly->targetTypeChecked == false)
            {
                partassembly->targetTypeChecked = (std::string(partassembly->priv->targetType).compare("NONE"/*DEFAULT_TARGET_TYPE*/) == 0);
                if(!partassembly->targetTypeChecked) // target-type was set by user
                {
                    for(unsigned int i = 0 ; i < (uint)detectionBoxResultNumber ; ++i)
                    {
                        adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
                        // check alert type was set 
                        if(detection_result.meta.find(partassembly->priv->targetType) != std::string::npos)
                        {
                            partassembly->targetTypeChecked = true;
                            break;
                        }
//                         if( i == detectionBoxResultNumber - 1)
//                             partassembly->targetTypeChecked = false;
                    }
                }
            }
//             std::cout << "partassembly->targetTypeChecked = " << partassembly->targetTypeChecked << std::endl;

            // if partassembly->targetTypeChecked is true, check whether to reset or not
            if(partassembly->targetTypeChecked == true)
            {
                for(unsigned int i = 0 ; i < (uint)detectionBoxResultNumber ; ++i)
                {
                    adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
                    // check alert type was set 
                    if(detection_result.meta.find("empty") != std::string::npos)
                    {
                        std::cout << "**************** reset the assembly parameters ************\n";
//                         partassembly->priv->alert = false;
//                         partassembly->targetTypeChecked = false;
//                         partassembly->startTick = 0;
//                         for(unsigned int i = 0; i < assemblyActionVector.size() - 1 ; ++i)
//                         {
//                             assemblyActionVector[i] = false;
//                             processingTime[i] = 0;
//                         }
                        partassembly->partContainerIsEmpty = true;
                    }
                }
            }


            for(unsigned int i = 0 ; i < (uint)detectionBoxResultNumber ; ++i)
            {
                adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
                
                // check target type is required or set
                if(partassembly->targetTypeChecked)
                {
                    int materialIndex = getPartIndexInBomList(partassembly, detection_result.obj_label);

                    if(materialIndex != -1)
                    {
                        int x1 = (int)(width * detection_result.x1);
                        int y1 = (int)(height * detection_result.y1);
                        int x2 = (int)(width * detection_result.x2);
                        int y2 = (int)(height * detection_result.y2);
                        partassembly->priv->bomMaterial[materialIndex]->Add(x1, y1, x2, y2, detection_result.prob);
                    
                        // container index
                        if(detection_result.obj_label.compare("container-semi-finished-products") == 0)
                            partassembly->priv->indexOfSemiProductContainer = materialIndex;
                        
                        // semi-product index
                        if(detection_result.obj_label.compare("semi-finished-products") == 0)
                            partassembly->priv->indexOfSemiProduct = materialIndex;
                    }
                }
            }
        }
    }
    
    // Check overlap if required in the future
    // add the check overlap code here to remove multi-detected objects.
    if(partassembly->priv->indexOfSemiProduct > 0) // more than one semi-product
        partassembly->priv->bestIndexObjectOfSemiProduct = partassembly->priv->bomMaterial[partassembly->priv->indexOfSemiProduct]->GetBestScoreObjectIndex();
}

static int getPartIndexInBomList(Gstpartassembly *partassembly, const std::string& partName)
{
    std::vector<std::string>::iterator it = std::find(partassembly->priv->bomList.begin(), partassembly->priv->bomList.end(), partName);
    if(it != partassembly->priv->bomList.end())
        return std::distance(partassembly->priv->bomList.begin(), it);
    else
        return -1;
}

static bool twoSidesCheck(Gstpartassembly *partassembly, int partIndex, int eachSideExistNumber)
{
    // get semi-product points vector
    std::vector<int> containerPointsVector = partassembly->priv->bomMaterial[partassembly->priv->indexOfSemiProduct]->GetPosition(partassembly->priv->bestIndexObjectOfSemiProduct);
    
    // central vertical line of semi-product
    int verticalCenter = (containerPointsVector[0] + containerPointsVector[2]) / 2;
    
    // calculate each side object number
    int numLeft = 0;
    int numRight = 0;
    int objectNumber = partassembly->priv->bomMaterial[partIndex]->GetObjectNumber();
    for(int i = 0; i < objectNumber; ++i)
    {
        std::vector<int> objectPointsVector = partassembly->priv->bomMaterial[partIndex]->GetPosition(i);
        if(objectPointsVector[0] < verticalCenter)
            numLeft++;
        else
            numRight++;
    }
    bool sideCheck = numLeft == eachSideExistNumber && numRight == eachSideExistNumber ? true : false;
    
    return sideCheck;
}

static void doAlgorithm(Gstpartassembly *partassembly, GstBuffer* buffer)
{
    // get base time of this element from last startTick
    long base_time = (GST_ELEMENT (partassembly))->base_time;
    
    // If metadata does not exist, return directly.
    GstAdBatchMeta *meta = gst_buffer_get_ad_batch_meta(buffer);
    if (meta == NULL)
    {
        g_message("Adlink metadata is not exist!");
        return;
    }
    AdBatch &batch = meta->batch;
    bool frame_exist = batch.frames.size() > 0 ? true : false;
    
    if(partassembly->priv->alert == true)
        if((gst_clock_get_time((GST_ELEMENT (partassembly))->clock) - base_time)/NANO_SECOND > 5)
            partassembly->priv->alert = false;

    // Check whether materials are in the container and semi-product
    // Get container
    int containerId = partassembly->priv->indexOfSemiProductContainer;
    if(containerId < 0)
        return;
    
    std::vector<int> rectContainerVector;
    if(partassembly->priv->bomMaterial[containerId]->GetObjectNumber() > 0)
        rectContainerVector = partassembly->priv->bomMaterial[containerId]->GetRectangle(0);
    
    if(rectContainerVector.size() != 4)
    {
        g_info("No Semi product container detected.");
        return;
    }
    
    // Container index is containerId in bomMaterial, take the first object one as the container
    std::vector<cv::Point> containerPointsVec = partassembly->priv->bomMaterial[containerId]->GetObjectPoints(0);
    
    // Get semi-product
    // check semi-product
    if(partassembly->priv->bestIndexObjectOfSemiProduct < 0)
    {
        g_info("No Semi product detected.");
        return;
    }
    int semiProductId = partassembly->priv->indexOfSemiProduct;
    int bestIndexObjectOfSemiProduct = partassembly->priv->bestIndexObjectOfSemiProduct;
    std::vector<cv::Point> semiProductPointsVec = partassembly->priv->bomMaterial[semiProductId]->GetObjectPoints(bestIndexObjectOfSemiProduct);
    
    //Check each material is in the container and required number
    int requiredMaterialNumber = partassembly->priv->bomMaterial.size();

    int totalPartsNum = 0; 
    for(unsigned int materialID = 0; materialID < (uint)requiredMaterialNumber; ++materialID)
    {
        int totalObjectNumber = partassembly->priv->bomMaterial[materialID]->GetObjectNumber();
        std::vector<cv::Point> intersectionPolygon;
        int numberInSemiProduct = 0;

        for(unsigned int objID = 0; objID < (uint)totalObjectNumber; objID++)
        {
            std::vector<cv::Point> objectPointsVec = partassembly->priv->bomMaterial[materialID]->GetObjectPoints(objID);
            float intersectAreaWithContainer = cv::intersectConvexConvex(containerPointsVec, objectPointsVec, intersectionPolygon, true);
            float intersectAreaWithSemiProduct = cv::intersectConvexConvex(semiProductPointsVec, objectPointsVec, intersectionPolygon, true);
            
            if(intersectAreaWithContainer > 0 && intersectAreaWithSemiProduct > 0)
            {
                numberInSemiProduct++;
                totalPartsNum++;
                partassembly->priv->bomMaterial[materialID]->SetPartNumber(numberInSemiProduct);
            }
        }
    }
    
    // check is there exist any object in semi-product container.
    // if only exist semi-product container, reset all parameters and return
    std::cout << "totalPartsNum = " << totalPartsNum << std::endl;
    if(totalPartsNum == 2 && partassembly->partContainerIsEmpty == true) // 1 container-semi-finished-products + 1 semi-finished-products
    {
        
//         if(partassembly->emptyCounter < 80)
//         {
//             partassembly->emptyCounter++;
//             std::cout << "partassembly->emptyCounter = " << partassembly->emptyCounter << std::endl;
//         }
//         else
//         {
            //std::cout << "**************** nothing in semi-product container ************\n";
            partassembly->priv->alert = false;
            partassembly->targetTypeChecked = false;
            partassembly->startTick = 0;
            for(unsigned int i = 0; i < assemblyActionVector.size() - 1 ; ++i)
            {
                assemblyActionVector[i] = false;
                processingTime[i] = 0;
            }
            partassembly->emptyCounter = 0;
            partassembly->partContainerIsEmpty = false;
        
        
            return;
//         }
    }
//     else
//         partassembly->emptyCounter = 0;
    
    //get check assembly action
    int checkAction = -1;
    for(unsigned int i = 0; i < assemblyActionVector.size(); ++i)
    {
        if(!assemblyActionVector[i])
        {
            checkAction = i;
            break;
        }
    }
    
    switch(checkAction)
    {
        case 0:     // put 1 semi-product in container
        {
            if(processingTime[checkAction] == 0)
                partassembly->startTick = partassembly->runningTime;
                
            if(partassembly->priv->bomMaterial[semiProductId]->CheckNumber())
            {
                partassembly->priv->bomMaterial[semiProductId]->SetIndexInBox(checkAction);
                assemblyActionVector[checkAction] = true;
                
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
                partassembly->startTick = 0;
                GST_MESSAGE("Put semi-product in container done.");
            }
            else
            {
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            
            break;
        }
        case 1:     // put 2 light-guide-cover in semi-finished-products(left and right)
        {
            if(processingTime[checkAction] == 0)
                partassembly->startTick = partassembly->runningTime;
            
            int id = getPartIndexInBomList(partassembly, "light-guide-cover");
            if(partassembly->priv->bomMaterial[id]->CheckNumber())
            {
                partassembly->priv->bomMaterial[id]->SetIndexInBox(checkAction);
                
                if(twoSidesCheck(partassembly, id, 1)) // Simply check each one should be at left and right side.
                {
                    assemblyActionVector[checkAction] = true;
                    
                    processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
                    partassembly->startTick = 0;
                    GST_MESSAGE("Put light-guide-cover done.");
                }
            }
            else
            {
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            break;
        }
        case 2:     // put 2 small-board-side-B in semi-finished-products(left and right)
        {
            if(processingTime[checkAction] == 0)
                partassembly->startTick = partassembly->runningTime;
            
            int id = getPartIndexInBomList(partassembly, "small-board-side-B");
            if(partassembly->priv->bomMaterial[id]->CheckNumber())
            {
                partassembly->priv->bomMaterial[id]->SetIndexInBox(checkAction);
                
                if(twoSidesCheck(partassembly, id, 1)) // Simply check each one should be at left and right side.
                {
                    assemblyActionVector[checkAction] = true;
                    
                    processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
                    partassembly->startTick = 0;
                    GST_MESSAGE("Put small-board-side-B done.");
                }
                else
                    processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            else
            {
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            break;
        }
        case 3:     // screw on 4 screws(2 on left, 2 on right)
        {
            if(processingTime[checkAction] == 0)
                partassembly->startTick = partassembly->runningTime;

            int id = getPartIndexInBomList(partassembly, "screwed-on");
            if(partassembly->priv->bomMaterial[id]->CheckNumber())
            {
                partassembly->priv->bomMaterial[id]->SetIndexInBox(checkAction);
                
                if(twoSidesCheck(partassembly, id, 2)) // Simply check each two should be at left and right side.
                {
                    assemblyActionVector[checkAction] = true;
                    
                    processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
                    partassembly->startTick = 0;
                    GST_MESSAGE("Screw on screws done.");
                }
                else
                    processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            else
            {
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            break;
        }
        case 4:     // put wire on 
        {
            if(processingTime[checkAction] == 0)
                partassembly->startTick = partassembly->runningTime;

            int id = getPartIndexInBomList(partassembly, "wire");
            if(partassembly->priv->bomMaterial[id]->CheckNumber())
            {
                partassembly->priv->bomMaterial[id]->SetIndexInBox(checkAction);
                assemblyActionVector[checkAction] = true;
                
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
                partassembly->startTick = 0;
                GST_MESSAGE("Put wire done.");
            }
            else
            {
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            break;
        }
        case 5:     // final visual inspection: 2 small-board-side-B with 4 screw-on(each small-board-side-B contains 2 screw-on) and 1 wire
        {
            if(processingTime[checkAction] == 0)
                partassembly->startTick = partassembly->runningTime;
            
            bool checkFinal = true;
            
            // 2 small-board-side-B
            int id = getPartIndexInBomList(partassembly, "small-board-side-B");
            checkFinal &= partassembly->priv->bomMaterial[id]->CheckNumber() && twoSidesCheck(partassembly, id, 1);
            
            // 4 screw on
            id = getPartIndexInBomList(partassembly, "screwed-on");
            checkFinal &= partassembly->priv->bomMaterial[id]->CheckNumber() && twoSidesCheck(partassembly, id, 2);
            
            // 1 wire
            id = getPartIndexInBomList(partassembly, "wire");
            checkFinal &= partassembly->priv->bomMaterial[id]->CheckNumber();
            
            if(checkFinal)
            {
                assemblyActionVector[checkAction] = true;  
                
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
                partassembly->startTick = 0;
                GST_MESSAGE("Visual inspection done.");
            }
            else
            {
                processingTime[checkAction] = partassembly->runningTime - partassembly->startTick;
            }
            break;
        }
        case 6:     // complete
            GST_MESSAGE("Assembly completed");
            assemblyActionVector[checkAction] = true;
            break;
        default:
            //std::cout << "no action need to check.\n";
            break;
    }
    
    if(frame_exist && containerId != -1)
    {
        // Check regular time
        for(unsigned int i = 0; i < assemblyActionVector.size() ; ++i)
        {
            if(processingTime[i] > processingRegularTime[i] && partassembly->priv->alert == false)
            {
                GST_MESSAGE("put idling alert message.");
                partassembly->priv->alert = true;
                
                std::string alertMessage = "," + std::string(partassembly->priv->alertType) + "<" + return_current_time_and_date() + ">";
            
                meta->batch.frames[0].detection_results[containerId].meta += alertMessage;
                partassembly->alertTick = cv::getTickCount();
            }
        }
        
        // If completed, set alert
        if(checkAction == assemblyActionVector.size() - 1)
        {
            std::string alertMessage = "," + std::string("Completed") + "<" + return_current_time_and_date() + ">";
            meta->batch.frames[0].detection_results[containerId].meta += alertMessage;
            GST_MESSAGE("put Completed alert message.");
        }
    }
}

static void drawObjects(Gstpartassembly *partassembly)
{
    int width = partassembly->srcMat.cols;
    int height = partassembly->srcMat.rows;
    float scale = 0.025;
    
    int font = cv::FONT_HERSHEY_COMPLEX;
    //double font_scale = 0.5;
    double font_scale = std::min(width, height)/(25/scale);
    int thickness = 2;
    
    int indexOfContainer = partassembly->priv->indexOfSemiProductContainer;
    
    if(indexOfContainer < 0)
        return;
    
    if(partassembly->priv->indexOfSemiProductContainer >= 0)
    {
        int numOfContainer = partassembly->priv->bomMaterial[indexOfContainer]->GetObjectNumber();
        if(numOfContainer <= 0)
            return;
    }
    std::vector<cv::Point> containerPointsVec = partassembly->priv->bomMaterial[indexOfContainer]->GetObjectPoints(0);
    std::vector<cv::Point> intersectionPolygon;
    
    for(unsigned int i = 0; i < partassembly->priv->bomMaterial.size(); ++i)
    {
        int numberOfObjectInMaterial = partassembly->priv->bomMaterial[i]->GetObjectNumber();
        for(unsigned int index = 0; index < (uint)numberOfObjectInMaterial; index++)
        {
            std::vector<int> pointsVector = partassembly->priv->bomMaterial[i]->GetPosition(index);
            if(pointsVector.size() > 0)
            {
                std::vector<cv::Point> objectPointsVec = partassembly->priv->bomMaterial[i]->GetObjectPoints(index);
                float intersectArea = cv::intersectConvexConvex(containerPointsVec, objectPointsVec, intersectionPolygon, true);
                
                if(intersectArea > 0)
                {
                    int x1 = pointsVector[0];
                    int y1 = pointsVector[1];
                    int x2 = pointsVector[2];
                    int y2 = pointsVector[3];
                    // draw rectangle
                    cv::rectangle(partassembly->srcMat, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 2);
                    // put label name
                    cv::putText(partassembly->srcMat, partassembly->priv->bomList[i], cv::Point(x1, y1), font, font_scale, cv::Scalar(0, 255, 255), thickness, 8, 0);
                }
            }
        }
        
    }
}

static void drawStatus(Gstpartassembly *partassembly)
{
    int width = partassembly->srcMat.cols;
    int height = partassembly->srcMat.rows;
    
    float scale = 0.02;
    int font = cv::FONT_HERSHEY_COMPLEX;
    //double font_scale = 1;
    double font_scale = std::min(width, height)/(25/scale);
    int thickness = 1; 

    cv::Scalar contentInfoColor = cv::Scalar(255, 255, 255);
    cv::Scalar actionDoneColor = cv::Scalar(0, 255, 0);
    cv::Scalar actionProcessColor = cv::Scalar(0, 255, 255);
    
    
    int startX = width * 0.03;
    int startY = height * 0.5;
    //int heightShift = 35;
    int heightShift = cv::getTextSize("Text", font, font_scale, thickness, 0).height * 1.5;
    bool metProcessingAction = false;
    double totalElapsedTime = 0;
    for(unsigned int i = 0; i < assemblyActionVector.size() - 1 ; ++i)
    {
        startY += heightShift;
        totalElapsedTime += processingTime[i];
        std::string timeString = " [" + round2String(processingTime[i], 3) + " s]";
        if(!assemblyActionVector[i] && !metProcessingAction)
        {
            metProcessingAction = true;
            
            cv::putText(partassembly->srcMat, assemblyActionInfoVector[i] + timeString, cv::Point(startX, startY), font, font_scale, actionProcessColor, thickness * 2, 4, 0);
            
            cv::circle (partassembly->srcMat, cv::Point(startX - width * 0.015, startY - heightShift / 4), heightShift / 2.5, actionProcessColor, -1, cv::LINE_8, 0);
        }
        else if(assemblyActionVector[i])
        {
            cv::putText(partassembly->srcMat, assemblyActionInfoVector[i] + timeString, cv::Point(startX, startY), font, font_scale, actionDoneColor, thickness, 4, 0);
        }
        else
        {
            cv::putText(partassembly->srcMat, assemblyActionInfoVector[i] + timeString, cv::Point(startX, startY), font, font_scale, contentInfoColor, thickness, 4, 0);
        }
    }
    
    // show total elapsed time
    startY += heightShift;
    cv::putText(partassembly->srcMat, "total elapsed time = " + round2String(totalElapsedTime, 3) + " s", cv::Point(startX, startY), font, font_scale, cv::Scalar(30, 144, 255), thickness, 4, 0);
    
    // show alert
    if(partassembly->priv->alert)
    {
        startY += 2 * heightShift;
        cv::putText(partassembly->srcMat, " idling !!", cv::Point(startX, startY), font, font_scale * 2, cv::Scalar(0, 0, 255), thickness * 2, 4, 0);
    }
    
}
