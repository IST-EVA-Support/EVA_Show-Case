#ifndef _GST_PARTASSEMBLY_H_
#define _GST_PARTASSEMBLY_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "assembly_utils/Status.h"

using namespace BASIC_INFORMATION;
using namespace PROGRESS;
using namespace PREPARE;

G_BEGIN_DECLS

#define GST_TYPE_PARTASSEMBLY   (gst_partassembly_get_type())
#define GST_PARTASSEMBLY(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PARTASSEMBLY,Gstpartassembly))
#define GST_PARTASSEMBLY_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PARTASSEMBLY,GstpartassemblyClass))
#define GST_IS_PARTASSEMBLY(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PARTASSEMBLY))
#define GST_IS_PARTASSEMBLY_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PARTASSEMBLY))

typedef struct _Gstpartassembly Gstpartassembly;
typedef struct _GstpartassemblyClass GstpartassemblyClass;
typedef struct _GstpartassemblyPrivate GstpartassemblyPrivate;

struct _Gstpartassembly
{
  GstVideoFilter base_partassembly;
  
  /*< private >*/
  GstpartassemblyPrivate *priv;
  
  cv::Mat srcMat;
  BOM bom;
  long startTick;
  long endTick;

};

struct _GstpartassemblyClass
{
  GstVideoFilterClass base_partassembly_class;
};

GType gst_partassembly_get_type (void);

G_END_DECLS

#endif
