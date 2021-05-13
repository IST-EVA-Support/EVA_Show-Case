#ifdef HAVE_CONFIG_H
#include "config.h"
#else

#ifndef VERSION
#define VERSION "1.1.0"
#endif
#ifndef PACKAGE
#define PACKAGE "An ADLINK Geofence plugin"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "libgeofence.so"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://www.adlinktech.com/"
#endif

#endif

#include "gstgeofencebase.h"
#include "gstgeofencefoot.h"

GST_DEBUG_CATEGORY_STATIC (geofence_debug);
#define GST_CAT_DEFAULT geofence_debug
/*** GStreamer Plugin Registration ***/
// register the element to the plugin
static gboolean plugin_init (GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT (geofence_debug, "[geofence debug]", 0, "geofence plugins");
    if (!gst_element_register (plugin, "geofencebase", GST_RANK_NONE, GST_TYPE_GEOFENCEBASE))
        return FALSE;
    if(!gst_element_register (plugin, "geofencefoot", GST_RANK_NONE, GST_TYPE_GEOFENCEFOOT))
        return FALSE;
    return TRUE;
}
//define the plugin information
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    geofence,
    PACKAGE,
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
