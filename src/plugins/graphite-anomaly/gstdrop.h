#ifndef _GST_DROP_H_
#define _GST_DROP_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_DROP   (gst_drop_get_type())
#define GST_DROP(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DROP,GstDrop))
#define GST_DROP_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DROP,GstDropClass))
#define GST_IS_DROP(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DROP))
#define GST_IS_DROP_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DROP))

typedef struct _GstDrop GstDrop;
typedef struct _GstDropClass GstDropClass;
typedef struct _GstDropPrivate GstDropPrivate;

struct _GstDrop
{
  GstVideoFilter base_drop;
  
  /*< private >*/
  GstDropPrivate *priv;
  
  cv::RNG rng;
  int64 start_learn;
  double required_time_to_learn;
  bool bDetect;
  cv::Mat srcMat, areaMaskMat, inspectMat, foregroundMat;
  long eventTick;
  long lastEventTick;
  
  
  //create Background Subtractor objects
  cv::Ptr<cv::BackgroundSubtractor> pBackSub;
};

struct _GstDropClass
{
  GstVideoFilterClass base_drop_class;
};

GType gst_drop_get_type (void);

G_END_DECLS

#endif
