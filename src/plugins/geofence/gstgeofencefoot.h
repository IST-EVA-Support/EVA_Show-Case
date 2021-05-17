#ifndef _GST_GEOFENCEFOOT_H_
#define _GST_GEOFENCEFOOT_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_GEOFENCEFOOT   (gst_geofencefoot_get_type())
#define GST_GEOFENCEFOOT(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GEOFENCEFOOT,Gstgeofencefoot))
#define GST_GEOFENCEFOOT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GEOFENCEFOOT,GstgeofencefootClass))
#define GST_IS_GEOFENCEFOOT(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GEOFENCEFOOT))
#define GST_IS_GEOFENCEFOOT_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GEOFENCEFOOT))

typedef struct _Gstgeofencefoot Gstgeofencefoot;
typedef struct _GstgeofencefootClass GstgeofencefootClass;
typedef struct _GstgeofencefootPrivate GstgeofencefootPrivate;

struct _Gstgeofencefoot
{
  GstVideoFilter base_geofencefoot;
  
  /*< private >*/
  GstgeofencefootPrivate *priv;
  
  cv::Mat srcMat;
  long eventTick;
  long lastEventTick;

};

struct _GstgeofencefootClass
{
  GstVideoFilterClass base_geofencefoot_class;
};

GType gst_geofencefoot_get_type (void);

G_END_DECLS

#endif
