#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstpartpreparation.h"
#include "gstadmeta.h"
#include "utils.h"

// *** define the required parts name, their required number and index in the box
// std::vector<std::string> partNameVector = {"background", "container-parts", 
//         "container-semi-finished-products", "light-guide-cover", "screw", "screwed-on", 
//         "semi-finished-products", "small-board-side-A", "small-board-side-B", "wire"};
// std::vector<int>  partRequiredNumberVector = {0, 1, 0, 2, 4, 0, 0, 2, 0, 1};
// std::vector<int>  partRequiredIndexVector = {-1, -1, -1, 0, 2, -1, -1, 1, -1, 3}; // -1: omit to check; 0~n: index in box from left to right
// ***

GST_DEBUG_CATEGORY_STATIC (gst_partpreparation_debug_category);
#define GST_CAT_DEFAULT gst_partpreparation_debug_category

static void gst_partpreparation_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_partpreparation_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_partpreparation_dispose (GObject * object);
static void gst_partpreparation_finalize (GObject * object);

// static GstStateChangeReturn gst_partpreparation_change_state (GstElement * element, GstStateChange transition);

static gboolean gst_partpreparation_start (GstBaseTransform * trans);
static gboolean gst_partpreparation_stop (GstBaseTransform * trans);
static gboolean gst_partpreparation_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
//static GstFlowReturn gst_partpreparation_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_partpreparation_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame);

static void mapGstVideoFrame2OpenCVMat(GstPartpreparation *partpreparation, GstVideoFrame *frame, GstMapInfo &info);
static void getDetectedBox(GstPartpreparation *partpreparation, GstBuffer* buffer);
static void doAlgorithm(GstPartpreparation *partpreparation, GstBuffer* buffer);
static void drawObjects(GstPartpreparation *partpreparation);
static void drawInformation(GstPartpreparation *partpreparation);
static void drawStatus(GstPartpreparation *partpreparation);

// std::string round2String(double d, int r)
// {
//     float div = 1;
//     for(int i = 0 ; i < r ; ++ i)
//         div *= 10;
//     
//     int s = d * div;
//     float t = s / div;
//     std::string str = std::to_string (t);
//     str.erase(str.find_last_not_of('0') + 1, std::string::npos);
//     return str;
// }

struct _GstPartPreparationPrivate
{
    // Information of each parts
    std::vector<std::string> bomList;
    std::vector<Material*> bomMaterial;
    int indexOfPartContainer;
    bool alert;
    gchar* alertType;
    bool partsDisplay;
    bool informationDisplay;
    bool statusDisplay;
};

enum
{
    PROP_0,
    PROP_ALERT_TYPE,
    PROP_PARTS_DISPLAY,
    PROP_INFORMATION_DISPLAY,
    PROP_STATUS_DISPLAY
};

#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gstpartpreparation", 0, "debug category for gstpartpreparation element");
G_DEFINE_TYPE_WITH_CODE(GstPartpreparation, gst_partpreparation, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(GstPartpreparation) DEBUG_INIT)

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */
static void gst_partpreparation_class_init (GstPartpreparationClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
//   GstElementClass *gstelement_class = G_ELEMENT_CLASS (klass);
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
      "Adlink part preparation video filter", "Filter/Video", "An ADLINK PartPreparation demo video filter", "Dr. Paul Lin <paul.lin@adlinktech.com>");

  gobject_class->set_property = gst_partpreparation_set_property;
  gobject_class->get_property = gst_partpreparation_get_property;
  
  // Install the properties to GObjectClass
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALERT_TYPE,
                                   g_param_spec_string ("alert-type", "Alert-Type", "The alert type name represents the event occurred. Two alert types are offered:\n\t\t\t(1)\"ready\", which means the parts in the box are all prepared;\n\t\t\t(2)\"order-incorrect\", which means the parts order in the box are incorrect.", "order-incorrect\0", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PARTS_DISPLAY,
                                   g_param_spec_boolean("parts-display", "Parts-display", "Show detected parts in frame.", TRUE, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INFORMATION_DISPLAY,
                                   g_param_spec_boolean("information-display", "Information-display", "Show each part's current number and order in the box.", FALSE, G_PARAM_READWRITE));
  
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_STATUS_DISPLAY,
                                   g_param_spec_boolean("status-display", "Status-display", "Show current preparation Status.", TRUE, G_PARAM_READWRITE));
  
  
//   // Assign status change function to GstElementClass
//   gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_partpreparation_change_state);
  
  gobject_class->dispose = gst_partpreparation_dispose;
  gobject_class->finalize = gst_partpreparation_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_partpreparation_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_partpreparation_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_partpreparation_set_info);
  //video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_partpreparation_transform_frame);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_partpreparation_transform_frame_ip);

}

static void gst_partpreparation_init (GstPartpreparation *partpreparation)
{
    /*< private >*/
    partpreparation->priv = (GstPartpreparationPrivate *)gst_partpreparation_get_instance_private (partpreparation);
    
    // *** define the required parts name, their required number and index in the box
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
    std::vector<int>  partRequiredNumberVector = {0, 1, 0, 2, 4, 0, 0, 2, 0, 1};
    std::vector<int>  partRequiredIndexVector = {-1, -1, -1, 0, 2, -1, -1, 1, -1, 3}; // -1: omit to check; 0~n: index in box from left to right
    partpreparation->bom = BOM(partNameVector, partRequiredNumberVector, partRequiredIndexVector);
    // ***
    
    // initialize the materials
    for(unsigned int i = 0; i < partpreparation->bom.NameVector.size(); i++)
    {
        if(partRequiredNumberVector[i] > 0)
        {
            partpreparation->priv->bomList.push_back(partpreparation->bom.NameVector[i]);
            partpreparation->priv->bomMaterial.push_back(new Material(partpreparation->bom.NumberVector[i], partpreparation->bom.OrderVector[i]));
        }
    }

    partpreparation->priv->alertType = "order-incorrect\0";
    partpreparation->prepareStatus = PrepareStatus::GetInstance();
    partpreparation->priv->alert = false;
    partpreparation->priv->partsDisplay = true;
    partpreparation->priv->informationDisplay = false;
    partpreparation->priv->statusDisplay = true;
    partpreparation->prepareStartTick = 0;
    partpreparation->prepareEndTick = 0;
}

void gst_partpreparation_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION (object);

  GST_DEBUG_OBJECT (partpreparation, "set_property");

  switch (property_id)
  {
    case PROP_ALERT_TYPE:
    {
        partpreparation->priv->alertType = g_value_dup_string(value);
        break;  
    }
    case PROP_PARTS_DISPLAY:
    {
        partpreparation->priv->partsDisplay = g_value_get_boolean(value);
        if(partpreparation->priv->partsDisplay)
            GST_MESSAGE("Display parts is enabled!");
        break;
    }
    case PROP_INFORMATION_DISPLAY:
    {
        partpreparation->priv->informationDisplay = g_value_get_boolean(value);
        if(partpreparation->priv->informationDisplay)
            GST_MESSAGE("Display information is enabled!");
        break;
    }
    case PROP_STATUS_DISPLAY:
    {
        partpreparation->priv->statusDisplay = g_value_get_boolean(value);
        if(partpreparation->priv->statusDisplay)
            GST_MESSAGE("Display status is enabled!");
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
  }
}

void gst_partpreparation_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION (object);

  GST_DEBUG_OBJECT (partpreparation, "get_property");

  switch (property_id)
  {
    case PROP_ALERT_TYPE:
       //g_value_set_string (value, partpreparation->priv->alertType.c_str());
       g_value_set_string (value, partpreparation->priv->alertType);
       break;
    case PROP_PARTS_DISPLAY:
       g_value_set_boolean(value, partpreparation->priv->partsDisplay);
       break;
    case PROP_INFORMATION_DISPLAY:
       g_value_set_boolean(value, partpreparation->priv->informationDisplay);
       break;
    case PROP_STATUS_DISPLAY:
       g_value_set_boolean(value, partpreparation->priv->statusDisplay);
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_partpreparation_dispose (GObject * object)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION (object);

  GST_DEBUG_OBJECT (partpreparation, "dispose");
  
  partpreparation->prepareStatus->ReleaseInstance();
  delete partpreparation->prepareStatus;

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_partpreparation_parent_class)->dispose (object);
}

void gst_partpreparation_finalize (GObject * object)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION(object);

  GST_DEBUG_OBJECT (partpreparation, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_partpreparation_parent_class)->finalize (object);
}

// // Change state (while overriding and using open/close/start/stop)
// static GstStateChangeReturn gst_partpreparation_change_state (GstElement * element, GstStateChange transition)
// {
//     GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
//     switch (transition) 
//     {
//         case GST_STATE_CHANGE_NULL_TO_READY:
//         {
//             std:: cout << "change_state: NULL_TO_READY" << std::endl;
//             break;
//         }
//         case GST_STATE_CHANGE_READY_TO_PAUSED:
//             std:: cout << "change_state: READY_TO_PAUSED" << std::endl;
//             break;
//         case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
//             std:: cout << "change_state: PAUSED_TO_PLAYING" << std::endl;
//             break;
//         default:
//             break;
//     }
//     switch (transition)
//     {
//         case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
//             std:: cout << "change_state: PLAYING_TO_PAUSED" << std::endl;
//             break;
//         case GST_STATE_CHANGE_PAUSED_TO_READY:
//             std:: cout << "change_state: PAUSED_TO_READY" << std::endl;
//             break;
//         case GST_STATE_CHANGE_READY_TO_NULL:
//             std:: cout << "change_state: READY_TO_NULL" << std::endl;
//             break;
//         default:
//             break;
//     }
//     
//     ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
//     return ret;
//     change_failed:
//     {
//        return GST_STATE_CHANGE_FAILURE;
//     }
// }

static gboolean gst_partpreparation_start (GstBaseTransform * trans)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION (trans);

  GST_DEBUG_OBJECT (partpreparation, "start");
  
//   partpreparation->prepareStartTick = 0;
//   partpreparation->prepareEndTick = 0;
  partpreparation->prepareStatus = PrepareStatus::GetInstance();
  
  std::cout << "<Start part preparation element>" << std::endl;
  
  return TRUE;
}

static gboolean gst_partpreparation_stop (GstBaseTransform * trans)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION (trans);

  partpreparation->prepareStartTick = 0;
  partpreparation->prepareEndTick = 0;
  
  GST_DEBUG_OBJECT (partpreparation, "stop");
  
  std::cout << "<Stop part preparation element>" << std::endl;

  return TRUE;
}

static gboolean gst_partpreparation_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION (filter);

  GST_DEBUG_OBJECT (partpreparation, "set_info");

  return TRUE;
}

/* transform */
// static GstFlowReturn gst_partpreparation_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe, GstVideoFrame * outframe)
// {
//   GstPartpreparation *partpreparation = GST_PARTPREPARATION (filter);
// 
//   GST_DEBUG_OBJECT (partpreparation, "transform_frame");
// 
//   return GST_FLOW_OK;
// }

static GstFlowReturn gst_partpreparation_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  GstPartpreparation *partpreparation = GST_PARTPREPARATION (filter);
  GstMapInfo info;

  GST_DEBUG_OBJECT (partpreparation, "transform_frame_ip");
  gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
  // map frame data from stream to partpreparation srcMat
  mapGstVideoFrame2OpenCVMat(partpreparation, frame, info);
  
  // get inference detected parts
  getDetectedBox(partpreparation, frame->buffer);
  
  // do algorithm
  doAlgorithm(partpreparation, frame->buffer);
  
  // draw objects
  if(partpreparation->priv->partsDisplay)
    drawObjects(partpreparation);
  
  // draw information
  if(partpreparation->priv->informationDisplay)
    drawInformation(partpreparation);
  
  // draw status
  if(partpreparation->priv->statusDisplay)
    drawStatus(partpreparation);
  
  gst_buffer_unmap(frame->buffer, &info);
  return GST_FLOW_OK;
}

static void mapGstVideoFrame2OpenCVMat(GstPartpreparation *partpreparation, GstVideoFrame *frame, GstMapInfo &info)
{
    if(partpreparation->srcMat.cols == 0 || partpreparation->srcMat.rows == 0)
        partpreparation->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    else if((partpreparation->srcMat.cols != frame->info.width) || (partpreparation->srcMat.rows != frame->info.height))
    {
        partpreparation->srcMat.release();
        partpreparation->srcMat = cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
    }
    else
        partpreparation->srcMat.data = info.data;
}

static void getDetectedBox(GstPartpreparation *partpreparation, GstBuffer* buffer)
{
    // reset the status
    for (auto& value : partpreparation->priv->bomMaterial)
    {
        value->ClearStatus();
    }
    partpreparation->priv->indexOfPartContainer = -1;
    
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
            int width = partpreparation->srcMat.cols;
            int height = partpreparation->srcMat.rows;
            for(unsigned int i = 0 ; i < (uint)detectionBoxResultNumber ; ++i)
            {
                adlink::ai::DetectionBoxResult detection_result = frame_info.detection_results[i];
                std::vector<std::string>::iterator it = std::find(partpreparation->priv->bomList.begin(), partpreparation->priv->bomList.end(), detection_result.obj_label);
                if(it != partpreparation->priv->bomList.end())
                {
                    int materialIndex = std::distance(partpreparation->priv->bomList.begin(), it);
                    int x1 = (int)(width * detection_result.x1);
                    int y1 = (int)(height * detection_result.y1);
                    int x2 = (int)(width * detection_result.x2);
                    int y2 = (int)(height * detection_result.y2);
                    partpreparation->priv->bomMaterial[materialIndex]->Add(x1, y1, x2, y2, detection_result.prob);
                
                    if(detection_result.obj_label.compare("container-parts") == 0)
                        partpreparation->priv->indexOfPartContainer = materialIndex;
                }
            }
        }
    }
    
    // Check overlap if required in the future
    // add the check overlap code here to remove multi-detected objects.
}

static void doAlgorithm(GstPartpreparation *partpreparation, GstBuffer* buffer)
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
    
    if(partpreparation->priv->alert == true)
        if((cv::getTickCount() - partpreparation->prepareEndTick) / cv::getTickFrequency() > 5)
            partpreparation->priv->alert = false;
        else
            return;
    
    // Check where materials are in the container box
    //Get container-parts(index=0 in bomList)    
    std::vector<int> rectVector;
    if(partpreparation->priv->bomMaterial[0]->GetObjectNumber() > 0)
        rectVector = partpreparation->priv->bomMaterial[0]->GetRectangle(0);
    
    if(rectVector.size() != 4)
        return;
    
    // Container index is 0 in bomMaterial
    std::vector<cv::Point> containerPointsVec = partpreparation->priv->bomMaterial[0]->GetObjectPoints(0);
    
    //Check each material is in the container-parts and required number
    int requiredMaterialNumber = partpreparation->priv->bomMaterial.size();
    float width = partpreparation->srcMat.cols;
    float *averagePositionInContainerArray = new float[requiredMaterialNumber];
    
    int totalPartsNum = 0;
    for(unsigned int materialID = 0; materialID < (uint)requiredMaterialNumber; ++materialID)
    {
        int totalObjectNumber = partpreparation->priv->bomMaterial[materialID]->GetObjectNumber();
        std::vector<cv::Point> intersectionPolygon;
        int numberInContainer = 0;
        float averagePosition = 0;
        for(unsigned int objID = 0; objID < (uint)totalObjectNumber; objID++)
        {
            std::vector<cv::Point> objectPointsVec = partpreparation->priv->bomMaterial[materialID]->GetObjectPoints(objID);
            float intersectArea = cv::intersectConvexConvex(containerPointsVec, objectPointsVec, intersectionPolygon, true);
            
            // Set the number
            if(intersectArea > 0)
            {
                numberInContainer++;
                partpreparation->priv->bomMaterial[materialID]->SetPartNumber(numberInContainer);
                
                // sum the position for average
                std::vector<int> objRectVec = partpreparation->priv->bomMaterial[materialID]->GetRectangle(objID);
                averagePosition += objRectVec[0] + (objRectVec[2] - objRectVec[0] + 1) / 2; // object center x position
            }
        }
        averagePosition = numberInContainer > 0 ? averagePosition / numberInContainer : width;
        
        // Those required index equal -1, like container itself, do not need to be indexed
        if(partpreparation->priv->bomMaterial[materialID]->GetPartRequiredIndex() == -1) 
            averagePositionInContainerArray[materialID] = width;
        else
            averagePositionInContainerArray[materialID] = averagePosition;
        
        totalPartsNum += numberInContainer;
    }
    
    // if status is assembly, means already ready before.
    if(partpreparation->prepareStatus->GetStatus() == Prepare::Assembly)
    {
        return;
    }

    float find_min = width;
    for(unsigned int i = 0; i < (uint)requiredMaterialNumber; ++i)
    {   
        float min = find_min;
        int id = 0;
        for(unsigned int j = i; j < (uint)requiredMaterialNumber; ++j)
        {
            if(averagePositionInContainerArray[j] < min)
            {
                min = averagePositionInContainerArray[j];
                id = j;
            }
        }
        
        if(partpreparation->priv->bomMaterial[id]->GetPartRequiredIndex() != -1)
        {
            partpreparation->priv->bomMaterial[id]->SetIndexInBox(i);
            averagePositionInContainerArray[id] = width;
        }
        
    }
    delete [] averagePositionInContainerArray;
    
    // If part is placed inside the container, start to count time
    if(totalPartsNum > 1 && partpreparation->prepareStartTick == 0)
        partpreparation->prepareStartTick = cv::getTickCount();
    else if(partpreparation->prepareStatus->GetStatus() == Prepare::NotReady)
        partpreparation->prepareEndTick = cv::getTickCount(); // for dynamic display
        
    
    // Check ready or not
    bool numReady = true;
    bool partsReady = true;
    for(unsigned int i = 0; i < (uint)requiredMaterialNumber; ++i)
    {
        partsReady &= partpreparation->priv->bomMaterial[i]->IsReady();
        numReady &= partpreparation->priv->bomMaterial[i]->CheckNumber();
    }

    bool orderReady = true;
    if(numReady)
    {
        for(unsigned int i = 0; i < (uint)requiredMaterialNumber; ++i)
        {
            orderReady &= partpreparation->priv->bomMaterial[i]->CheckOrder();
        }
    }  
    
    // if is Not Ready, do not change the status
    if(partpreparation->prepareStatus->GetStatus() == Prepare::NotReady)
    {
        if(partsReady)
        {
            partpreparation->prepareStatus->SetStatus(Prepare::Ready);
            // record end time
            {
                partpreparation->prepareEndTick = cv::getTickCount();
                //partpreparation->prepareEndTick - partpreparation->prepareStartTick) / cv::getTickFrequency()
                //partpreparation->priv->requiredFinishTime
            }
        }
    }
    else if(partpreparation->prepareStatus->GetStatus() == Prepare::Ready)
    {
        if(!partsReady)
        {
            if(totalPartsNum == 1)
                partpreparation->prepareStatus->SetStatus(Prepare::NotReady);
            else
                partpreparation->prepareStatus->SetStatus(Prepare::Assembly);
        }
    }
    else if(partpreparation->prepareStatus->GetStatus() == Prepare::Assembly)
    {
        if(!partsReady && totalPartsNum == 1)
            partpreparation->prepareStatus->SetStatus(Prepare::NotReady);
    }
    
    
    // judge alert
    if(frame_exist && partpreparation->priv->indexOfPartContainer != -1)
    {
        // incorrect order alert to metadata
        if(numReady && !orderReady)
        {
            partpreparation->priv->alert = true;
            
            std::string alertMessage = "," + std::string(partpreparation->priv->alertType) + "<" + return_current_time_and_date() + ">";
        
            meta->batch.frames[0].detection_results[partpreparation->priv->indexOfPartContainer].meta += alertMessage;
        }
        
        // ready alert to metadata
        if(partpreparation->prepareStatus->GetStatus() == Prepare::Ready)
        {
            std::string alertMessage = "," + std::string("ready") + "<" + return_current_time_and_date() + ">";
        
            meta->batch.frames[0].detection_results[partpreparation->priv->indexOfPartContainer].meta += alertMessage;
        }
    }
}

static void drawObjects(GstPartpreparation *partpreparation)
{
    int font = cv::FONT_HERSHEY_COMPLEX;
    double font_scale = 0.5;
    int thickness = 2;
    for(unsigned int i = 0; i < partpreparation->priv->bomMaterial.size(); ++i)
    {
        int numberOfObjectInMaterial = partpreparation->priv->bomMaterial[i]->GetObjectNumber();
        for(unsigned int index = 0; index < (uint)numberOfObjectInMaterial; index++)
        {
            std::vector<int> pointsVector = partpreparation->priv->bomMaterial[i]->GetPosition(index);
            if(pointsVector.size() > 0)
            {
                int x1 = pointsVector[0];
                int y1 = pointsVector[1];
                int x2 = pointsVector[2];
                int y2 = pointsVector[3];
                // draw rectangle
                cv::rectangle(partpreparation->srcMat, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 2);
                // put label name
                cv::putText(partpreparation->srcMat, partpreparation->priv->bomList[i], cv::Point(x1, y1), font, font_scale, cv::Scalar(0, 255, 255), thickness, 8, 0);
            }
        }
        
    }
}

static void drawInformation(GstPartpreparation *partpreparation)
{
    int font = cv::FONT_HERSHEY_COMPLEX;
    double font_scale = 1;
    int thickness = 1;
    int width = partpreparation->srcMat.cols;
    int height = partpreparation->srcMat.rows;
    int startX = width / 10;
    int startY = height / 2;
    for(unsigned int i = 0; i < partpreparation->priv->bomMaterial.size(); ++i)
    {
        startY = height / 2 + i * 20;
        
        std::string numMsgTxt = partpreparation->priv->bomList[i] + " number = " + std::to_string(partpreparation->priv->bomMaterial[i]->GetValidatePartCurrentNumber());
        std::string orderMsgTxt = ", order = " + std::to_string(partpreparation->priv->bomMaterial[i]->GetValidatePartCurrentIndex());
        
        bool isReady = partpreparation->priv->bomMaterial[i]->IsReady();
        
        std::string outputMsg = "";
        if(i > 0)
            outputMsg = numMsgTxt + orderMsgTxt;
        else
            outputMsg = numMsgTxt;
            
        if(!isReady)
            cv::putText(partpreparation->srcMat, outputMsg, cv::Point(startX, startY), font, font_scale, cv::Scalar(0, 0, 255), thickness, 8, 0);
        else
            cv::putText(partpreparation->srcMat, outputMsg, cv::Point(startX, startY), font, font_scale, cv::Scalar(0, 255, 0), thickness+1, 8, 0);
    }
}

static void drawStatus(GstPartpreparation *partpreparation)
{
    int width = partpreparation->srcMat.cols;
    int height = partpreparation->srcMat.rows;
    float scale = 0.03;
    int font = cv::FONT_HERSHEY_COMPLEX;
    double font_scale = std::min(width, height)/(25/scale);
    int thickness = 1; 
    
    // Show Ready or not
    int startX = width / 3;
    int startY = height / 8;
    if(partpreparation->prepareStatus->GetStatus() == Prepare::Ready)
    {
        // calculate preparation time
        if(partpreparation->prepareStartTick != 0)
        {
            double duration = (partpreparation->prepareEndTick - partpreparation->prepareStartTick) / cv::getTickFrequency();
            std::string t = round2String(duration, 3);
            cv::putText(partpreparation->srcMat, "elapsed: " + t + " s", cv::Point(50, 50), font, font_scale, cv::Scalar(0, 255, 0), thickness*2, 8, 0);
        }
        
        cv::putText(partpreparation->srcMat, "Ready!", cv::Point(startX, startY), font, font_scale * 2, cv::Scalar(0, 255, 0), thickness*3, 8, 0);
    }
    else if(partpreparation->prepareStatus->GetStatus() == Prepare::NotReady)
    {
        // elapsed time
        if(partpreparation->prepareStartTick != 0)
        {
            double elapsedTime = (cv::getTickCount() - partpreparation->prepareStartTick) / cv::getTickFrequency();
            std::string t = round2String(elapsedTime, 3);
            
            cv::putText(partpreparation->srcMat, "elapsed: " + t + " s", cv::Point(50, 50), font, font_scale, cv::Scalar(15, 185, 255), thickness*2, 8, 0);
        }
        
        if(!partpreparation->priv->alert)
            cv::putText(partpreparation->srcMat, "Not Ready!", cv::Point(startX, startY), font, font_scale * 2, cv::Scalar(15, 185, 255), thickness*3, 8, 0);
        else
            cv::putText(partpreparation->srcMat, "incorrect order!", cv::Point(startX, startY), font, font_scale * 3, cv::Scalar(0, 0, 255), thickness*3, 8, 0);
    }
    else
        cv::putText(partpreparation->srcMat, "Assembly", cv::Point(startX, startY), font, font_scale * 2, cv::Scalar(0, 255, 255), thickness*3, 8, 0);
}
