#ifndef _GST_weardetection_H_
#define _GST_weardetection_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_weardetection   (gst_weardetection_get_type())
#define GST_weardetection(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_weardetection,Gstweardetection))
#define GST_weardetection_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_weardetection,GstweardetectionClass))
#define GST_IS_weardetection(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_weardetection))
#define GST_IS_weardetection_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_weardetection))

typedef struct _Gstweardetection Gstweardetection;
typedef struct _GstweardetectionClass GstweardetectionClass;
typedef struct _GstweardetectionPrivate GstweardetectionPrivate;

struct _Gstweardetection
{
  GstVideoFilter base_weardetection;
  /*< private >*/
  GstweardetectionPrivate *priv;
  cv::Mat srcMat;
  long eventTick;
  long lastEventTick;
  gchar* engineID;
  gchar* query;
};

struct _GstweardetectionClass{
  GstVideoFilterClass base_weardetection_class;
};

GType gst_weardetection_get_type (void);

G_END_DECLS

#endif
