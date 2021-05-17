#ifndef _GST_GEOFENCEBASE_H_
#define _GST_GEOFENCEBASE_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_GEOFENCEBASE   (gst_geofencebase_get_type())
#define GST_GEOFENCEBASE(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GEOFENCEBASE,GstGeofencebase))
#define GST_GEOFENCEBASE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GEOFENCEBASE,GstGeofencebaseClass))
#define GST_IS_GEOFENCEBASE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GEOFENCEBASE))
#define GST_IS_GEOFENCEBASE_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GEOFENCEBASE))

typedef struct _GstGeofencebase GstGeofencebase;
typedef struct _GstGeofencebaseClass GstGeofencebaseClass;
typedef struct _GstGeoFenceBasePrivate GstGeofencebasePrivate;

struct _GstGeofencebase
{
  GstVideoFilter base_geofencebase;
  
  /*< private >*/
  GstGeofencebasePrivate *priv;
  
  cv::Mat srcMat;
  long eventTick;
  long lastEventTick;

};

struct _GstGeofencebaseClass
{
  GstVideoFilterClass base_geofencebase_class;
};

GType gst_geofencebase_get_type (void);

G_END_DECLS

#endif
