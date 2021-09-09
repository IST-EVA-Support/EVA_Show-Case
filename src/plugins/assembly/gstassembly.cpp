#ifdef HAVE_CONFIG_H
#include "config.h"
#else

#ifndef VERSION
#define VERSION "1.1.0"
#endif
#ifndef PACKAGE
#define PACKAGE "An ADLINK Assembly plugin"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "libassembly.so"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://www.adlinktech.com/"
#endif

#endif

#include "gstpartpreparation.h"
//#include "gstgeofencefoot.h"

GST_DEBUG_CATEGORY_STATIC (assembly_debug);
#define GST_CAT_DEFAULT assembly_debug
/*** GStreamer Plugin Registration ***/
// register the element to the plugin
static gboolean plugin_init (GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT (assembly_debug, "[assembly debug]", 0, "assembly plugins");
    if (!gst_element_register (plugin, "partpreparation", GST_RANK_NONE, GST_TYPE_PARTPREPARATION))
        return FALSE;
//     if(!gst_element_register (plugin, "geofencefoot", GST_RANK_NONE, GST_TYPE_GEOFENCEFOOT))
//         return FALSE;
    return TRUE;
}
//define the plugin information
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    assembly,
    PACKAGE,
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
