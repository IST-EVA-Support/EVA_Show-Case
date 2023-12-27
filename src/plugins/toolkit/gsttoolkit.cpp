/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2023 parmod <<user@hostname.org>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gsttoolkit.h"


GST_DEBUG_CATEGORY_STATIC (gst_toolkit_debug);
#define GST_CAT_DEFAULT gst_toolkit_debug
//#define gst_toolkit_parent_class parent_class G_DEFINE_TYPE (Gsttoolkit, gst_toolkit, GST_TYPE_ELEMENT);
#define DEBUG_INIT GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "gsttoolkit", 0, "debug category for gstweardetection element");
G_DEFINE_TYPE_WITH_CODE(Gsttoolkit, gst_toolkit, GST_TYPE_VIDEO_FILTER, G_ADD_PRIVATE(Gsttoolkit)  DEBUG_INIT);

/* Filter signals and args */
enum
{
    /* FILL ME */
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_LABEL,
    PROP_QUERY,
    PROP_SILENT
};

static void gst_toolkit_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_toolkit_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
/* initialize the toolkit's class */
static void gst_toolkit_class_init (GsttoolkitClass * klass){

    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
    GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

    gobject_class->set_property = gst_toolkit_set_property;
    gobject_class->get_property = gst_toolkit_get_property;
    g_object_class_install_property (gobject_class, PROP_SILENT, g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?", FALSE, G_PARAM_READWRITE));
    gst_element_class_set_details_simple (GST_ELEMENT_CLASS(klass), "toolkit", "FIXME:Generic", "FIXME:Generic Template Element", "parmod.singh <<parmod.singh@adlinktech.com>>");
    /* Setting up pads and setting metadata should be moved to
        base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,gst_caps_from_string (VIDEO_SRC_CAPS)));
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,gst_caps_from_string (VIDEO_SINK_CAPS)));
    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_QUERY,g_param_spec_string ("query", "query", "ROI query", "", (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_LABEL,g_param_spec_string("label", "label","inferred labels file","", (GParamFlags)G_PARAM_READWRITE));

    video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_toolkit_set_info);
    video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_toolkit_transform_frame_ip);
}
static gboolean gst_toolkit_set_info (GstVideoFilter * filter, GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info){
    Gsttoolkit *toolkit = GST_toolkit (filter);
    GST_DEBUG_OBJECT (toolkit, "set_info");

    int width = in_info->width;
    int height = in_info->height;
    //relative to absolute size
    for (std::vector<std::array<double,2>>& poly : toolkit->aoi){
        toolkit->aoi_cv.push_back(std::vector<cv::Point>());//aoi in cv format initialize
        for (std::array<double,2>& point: poly)
            toolkit->aoi_cv.back().push_back(cv::Point2d((int)(point[0]*width),(int)(point[1]*height)));
    }
    return TRUE;
}
static GstFlowReturn gst_toolkit_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
    Gsttoolkit *toolkit = GST_toolkit (filter);
    GST_DEBUG_OBJECT (toolkit, "transform_frame_ip");
    if(toolkit->query == "") /* metadata version 1 not implemented*/
        toolkit->query = "//adrt";
    GstMapInfo info;
    gst_buffer_map(frame->buffer, &info, GST_MAP_READ);
    std::vector<QueryResult> results = gst_buffer_adroi_query(frame->buffer, toolkit->query);
    std::cout << "results.size()\t"<<results.size()<<"\n";
    cv::Mat image =  cv::Mat(frame->info.height, frame->info.width, CV_8UC3, info.data);
     //   show_polygon_on_img(image,each_aoi_poly,cv::Scalar(255, 0, 0) );
        std::set<std::string> label_inferred,label_not_inferred;
        for(unsigned int result_index = 0; result_index < results.size(); ++result_index) {
            QueryResult query_result = results[result_index];
            int width = frame->info.width, height = frame->info.height;
            for (unsigned int i = 0; i < query_result.rois.size(); ++i) {
                if (query_result.rois[i]->category.compare("box") != 0)
                    continue;
                std::shared_ptr <Classification> labelInfo = std::static_pointer_cast<Classification>(
                        query_result.rois[i]->datas.at(0));
                label_inferred.insert(labelInfo->label);
                std::cout << labelInfo->label << "\t LABEL" << "\n";

            }
            std:: string labels_missing = "missing:  ";
            std:: string labels_present = "present:  ";
            for (std::set<std::string>::iterator it = toolkit->labels.begin(); it != toolkit->labels.end(); ++it){
                if (label_inferred.find(*it) == label_inferred.end())
                    label_not_inferred.emplace(*it);
            }
            for (std::set<std::string>::iterator it = label_inferred.begin(); it != label_inferred.end() ; ++it) {
                cv::putText(image, "Correct Position!", getDisplayPos(image, *it), cv::FONT_HERSHEY_DUPLEX, 1.0,
                            CV_RGB(118, 185, 0), 2);
                labels_present += "  " + *it;
            }
            for (std::set<std::string>::iterator it = label_not_inferred.begin(); it != label_not_inferred.end() ; ++it) {
                cv::putText(image, "Missing Toolkit", getDisplayPos(image, *it), cv::FONT_HERSHEY_DUPLEX, 1.0,
                            CV_RGB(118, 185, 0), 2);
                labels_missing += "  " + *it;
            }

            cv::putText(image,labels_present , cv::Point(1200, image.rows /6),cv::FONT_HERSHEY_DUPLEX,1.0,CV_RGB(118, 185, 0),2);
            cv::putText(image,labels_missing , cv::Point(1200, image.rows /4),cv::FONT_HERSHEY_DUPLEX,1.0,CV_RGB(118, 185, 0),2);
           // std::string out = "missing:->" + std::to_string(label_not_inferred.size());
            //cv::putText(image,out, cv::Point(1200, image.rows / 10),cv::FONT_HERSHEY_DUPLEX,1.0,CV_RGB(118, 185, 0),2);
        }
   // }
    gst_buffer_unmap(frame->buffer, &info);
    return GST_FLOW_OK;
}
cv::Point getDisplayPos(const cv::Mat& image,std::string label){
    if (label.compare("lt"))
        return cv::Point(300, image.rows / 4);
    else if (label.compare("lm"))
        return cv::Point(300, image.rows / 2.5);
    else if (label.compare("lb"))
        return cv::Point(300, image.rows / 1.5);
    else if (label.compare("rt"))
        return cv::Point(1200, image.rows / 4);
    else if (label.compare("rm"))
        return cv::Point(1200, image.rows / 2.5);
    else if (label.compare("rb"))
        return cv::Point(1500, image.rows / 1.5);
    return cv::Point(1200, image.rows / 10);
}

static void gst_toolkit_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
    Gsttoolkit *filter = GST_toolkit (object);

    switch (prop_id) {
        case PROP_SILENT:
            filter->silent = g_value_get_boolean(value);
            break;
        case PROP_QUERY:
            filter->query = g_value_dup_string(value);
            break;

        case PROP_LABEL:{
            filter->label_file = g_value_dup_string(value + '\0');
            std::ifstream fin;
            fin.open(filter->label_file, std::ios::in);
            if (!fin.is_open()) {
                GST_ERROR(" labels file open error %s", filter->label_file);
                printf("\n\n\n\n file open error \t %s ",filter->label_file);
                break;
            }
            int index = 0;
            for (std::string label; std::getline(fin, label);)
                filter->labels.emplace(label);
                //filter->label_map.emplace(label,index++);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

}

static void gst_toolkit_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec){
    Gsttoolkit *filter = GST_toolkit (object);
    switch (prop_id) {
        case PROP_SILENT:
            g_value_set_boolean (value, filter->silent);
            break;
        case PROP_QUERY:
            g_value_set_string (value, filter->query);
            break;
        case PROP_LABEL:
            g_value_set_string(value, filter->label_file);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}
/* initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void gst_toolkit_init (Gsttoolkit * filter){
    filter->silent = FALSE;
    filter ->query = (gchar*) "";
    filter->label_file = nullptr;
    filter->labels = {};
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean toolkit_init (GstPlugin * toolkit){
    /* debug category for filtering log messages
   *
   * exchange the string 'Template toolkit' with your description
   */
   // GST_DEBUG_CATEGORY_INIT (gst_toolkit_debug, "toolkit", 0, "Template toolkit");

    return gst_element_register (toolkit, "toolkit", GST_RANK_NONE, GST_TYPE_toolkit);//GST_ELEMENT_REGISTER (toolkit, toolkit);
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "toolkit"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.0.0"
#endif
#ifndef GST_PACKAGE_NAME
#define GST_PACKAGE_NAME "libtoolkit.so"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://www.adlinktech.com/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, toolkit, PACKAGE, toolkit_init, PACKAGE_VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
