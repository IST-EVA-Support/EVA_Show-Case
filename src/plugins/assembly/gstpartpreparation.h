#ifndef _GST_PARTPREPARATION_H_
#define _GST_PARTPREPARATION_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "assembly_utils/Status.h"

using namespace BASIC_INFORMATION;
using namespace PROGRESS;
using namespace PREPARE;

G_BEGIN_DECLS

#define GST_TYPE_PARTPREPARATION   (gst_partpreparation_get_type())
#define GST_PARTPREPARATION(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PARTPREPARATION,GstPartpreparation))
#define GST_PARTPREPARATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PARTPREPARATION,GstPartPreparationClass))
#define GST_IS_PARTPREPARATION(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PARTPREPARATION))
#define GST_IS_PARTPREPARATION_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PARTPREPARATION))

typedef struct _GstPartpreparation GstPartpreparation;
typedef struct _GstPartpreparationClass GstPartpreparationClass;
typedef struct _GstPartPreparationPrivate GstPartpreparationPrivate;

struct _GstPartpreparation
{
  GstVideoFilter base_partpreparation;
  GstPartpreparationPrivate *priv;
  cv::Mat srcMat;
  BOM bom;
  PrepareStatus* prepareStatus;
  long baseTick;
  double prepareStartTime;
  double prepareEndTime;
  double runningTime;
  
  int emptyCounter;
};

struct _GstPartpreparationClass
{
  GstVideoFilterClass base_partpreparation_class;
};

GType gst_partpreparation_get_type (void);

G_END_DECLS

#endif
