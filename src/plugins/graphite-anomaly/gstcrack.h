#ifndef _GST_CRACK_H_
#define _GST_CRACK_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_CRACK   (gst_crack_get_type())
#define GST_CRACK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CRACK,GstCrack))
#define GST_CRACK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CRACK,GstCrackClass))
#define GST_IS_CRACK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CRACK))
#define GST_IS_CRACK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CRACK))

typedef struct _GstCrack GstCrack;
typedef struct _GstCrackClass GstCrackClass;
typedef struct _GstCrackPrivate GstCrackPrivate;

struct _GstCrack
{
  GstVideoFilter base_crack;
  
  /*< private >*/
  GstCrackPrivate *priv;
  
  cv::Mat srcMat;
  long eventTick;
  long lastEventTick;

};

struct _GstCrackClass
{
  GstVideoFilterClass base_crack_class;
};

GType gst_crack_get_type (void);

G_END_DECLS

#endif
