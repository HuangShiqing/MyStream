/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2021  <<user@hostname.org>>
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

/**
 * SECTION:element-infer
 *
 * FIXME:Describe infer here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! infer ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

// #ifdef HAVE_CONFIG_H
#include "config.h"
// #endif

#include <gst/base/base.h>
#include <gst/controller/controller.h>
#include <gst/gst.h>
#include <stdlib.h>

#include "gstinfer.h"
#include "metadata.h"

// 越界检查需要在输入前做好
// src:nhwc
void my_draw_box(unsigned char* src, int width, int height, int left, int right, int top, int bot, unsigned char r,
                 unsigned char g, unsigned char b) {
    int left_top_index = 3 * width * top + 3 * left;
    int right_top_index = 3 * width * top + 3 * right;
    int left_bot_index = 3 * width * bot + 3 * left;
    int right_bot_index = 3 * width * bot + 3 * right;
    for (int i = left_top_index; i < right_top_index; i += 3) {
        src[i] = r;
        src[i + 1] = g;
        src[i + 2] = b;
    }
    for (int i = left_bot_index; i < right_bot_index; i += 3) {
        src[i] = r;
        src[i + 1] = g;
        src[i + 2] = b;
    }
    for (int i = left_top_index; i < left_bot_index; i += (3 * width)) {
        src[i] = r;
        src[i + 1] = g;
        src[i + 2] = b;
    }
    for (int i = right_top_index; i < right_bot_index; i += (3 * width)) {
        src[i] = r;
        src[i + 1] = g;
        src[i + 2] = b;
    }
}
// 越界检查需要在输入前做好
// src:nhwc
void my_draw_prob(unsigned char* src, int width, int height, int left, int right, int top, int bot, float prob,
                  unsigned char r, unsigned char g, unsigned char b) {
    int h = (bot - top) * prob;
    int right_bot_index = 3 * width * bot + 3 * right;

    int prob_bot_index = right_bot_index;
    int prob_top_index = prob_bot_index - h * 3 * width;
    for (int i = prob_top_index; i < prob_bot_index; i += (3 * width)) {
        src[i] = r;
        src[i + 1] = g;
        src[i + 2] = b;
    }
}

GST_DEBUG_CATEGORY_STATIC(gst_infer_debug);
#define GST_CAT_DEFAULT gst_infer_debug

/* Filter signals and args */
enum {
    /* FILL ME */
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_SILENT,
};

/* the capabilities of the inputs and outputs.
 *
 * FIXME:describe the real formats here.
 */
static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("ANY"));

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("ANY"));

#define gst_infer_parent_class parent_class
G_DEFINE_TYPE(GstInfer, gst_infer, GST_TYPE_BASE_TRANSFORM);

static void gst_infer_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_infer_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static GstFlowReturn gst_infer_transform_ip(GstBaseTransform* base, GstBuffer* outbuf);
static gboolean gst_nvinfer_start(GstBaseTransform* btrans);
static gboolean gst_nvinfer_stop(GstBaseTransform* btrans);
/* GObject vmethod implementations */

/* initialize the infer's class */
static void gst_infer_class_init(GstInferClass* klass) {
    GObjectClass* gobject_class;
    GstElementClass* gstelement_class;
    GstBaseTransformClass* gstbasetransform_class;

    gobject_class = (GObjectClass*)klass;
    gstelement_class = (GstElementClass*)klass;
    gstbasetransform_class = (GstBaseTransformClass*)klass;

    gobject_class->set_property = gst_infer_set_property;
    gobject_class->get_property = gst_infer_get_property;

    gstbasetransform_class->start = GST_DEBUG_FUNCPTR(gst_nvinfer_start);
    gstbasetransform_class->stop = GST_DEBUG_FUNCPTR(gst_nvinfer_stop);
    gstbasetransform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_infer_transform_ip);

    g_object_class_install_property(gobject_class, PROP_SILENT,
                                    g_param_spec_boolean("silent", "Silent", "Produce verbose output ?", FALSE,
                                                         G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

    gst_element_class_set_details_simple(gstelement_class, "Infer", "Generic/Filter", "FIXME:Generic Template Filter",
                                         " <<user@hostname.org>>");

    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&src_template));
    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&sink_template));

    // GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
    //     GST_DEBUG_FUNCPTR (gst_infer_transform_ip);

    /* debug category for fltering log messages
     *
     * FIXME:exchange the string 'Template infer' with your description
     */
    GST_DEBUG_CATEGORY_INIT(gst_infer_debug, "infer", 0, "Template infer");
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_infer_init(GstInfer* filter) {
    filter->silent = FALSE;
}

static void gst_infer_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
    GstInfer* filter = GST_INFER(object);

    switch (prop_id) {
        case PROP_SILENT:
            filter->silent = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_infer_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
    GstInfer* filter = GST_INFER(object);

    switch (prop_id) {
        case PROP_SILENT:
            g_value_set_boolean(value, filter->silent);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* GstBaseTransform vmethod implementations */

/* this function does the actual processing
 */
static GstFlowReturn gst_infer_transform_ip(GstBaseTransform* base, GstBuffer* outbuf) {
    GstInfer* filter = GST_INFER(base);

    if (GST_CLOCK_TIME_IS_VALID(GST_BUFFER_TIMESTAMP(outbuf)))
        gst_object_sync_values(GST_OBJECT(filter), GST_BUFFER_TIMESTAMP(outbuf));

    // if (filter->silent == FALSE)
    //     g_print("I'm plugged, therefore I'm in.\n");

    // retrieve
    GstMapInfo map;
    if (!gst_buffer_map(outbuf, &map, GST_MAP_READ)) {
        g_print("GstMapInfo error\r\n");
        return GST_FLOW_ERROR;
    }
    guint8* gstData = map.data;    // GST_BUFFER_DATA(gstBuffer);
    const int gstSize = map.size;  // GST_BUFFER_SIZE(gstBuffer);
    if (!gstData) {
        g_print("gstData error\r\n");
        return GST_FLOW_ERROR;
    }
    // g_print("gstSize = %d\r\n", gstSize);

    guint8* data = g_malloc(gstSize * sizeof(guint8));
    wrapper_rfb320_nhwc2nchw(filter->rfb320, data, gstData, 320, 240);
    wrapper_rfb320_pre_process(filter->rfb320, data);
    wrapper_rfb320_forward(filter->rfb320);
    wrapper_rfb320_post_process(filter->rfb320);
    Detection* dets = NULL;
    int count = 0;
    wrapper_rfb320_get_detections(filter->rfb320, &dets, &count);

    // g_print("[hsq] count: %d\r\n", count);
    // if (count)
    //     g_print("[hsq] confidence: %f\r\n", d->confidence);
    g_free(data);

    // draw box
    float thresh = 0.5;
    int WIDTH = 320;
    int HEIGHT = 240;
    for (int i = 0; i < count; i++) {
        if (dets[i].confidence < thresh)
            continue;
        BBox b = dets[i].box;
        int left = b.x1;
        int right = b.x2;
        int top = b.y1;
        int bot = b.y2;

        if (left < 0)
            left = 0;
        if (right > WIDTH - 1)
            right = WIDTH - 1;
        if (top < 0)
            top = 0;
        if (bot > HEIGHT - 1)
            bot = HEIGHT - 1;
        my_draw_box(gstData, WIDTH, HEIGHT, left, right, top, bot, 255, 0, 0);
        my_draw_prob(gstData, WIDTH, HEIGHT, left, right, top, bot, dets[i].confidence, 0, 255, 0);
    }

    // //nhwc
    // for(int h=0;h<240;h++){
    //     for(int w=0;w<320;w++){
    //         for(int c=0;c<3;c++){
    //             if(h>120) {
    //                 gstData[h*320*3+w*3+c]=255;
    //             }
    //         }
    //     }
    // }

    // gst_buffer_add_my_example_meta(outbuf, 1, "hi");
    // MyExampleMeta* meta = gst_buffer_get_my_example_meta(outbuf);
    // if (filter->silent == FALSE)
    //   g_print ("meta->name = %s.\n", meta->name);

    // add the meta
    FrameMeta frame_meta;
    frame_meta.num_obj_meta = 0;
    for (int i = 0; i < count; i++) {
        ObjectMeta obj_meta;
        obj_meta.class_id = 0;  // TODO:
        obj_meta.confidence = dets[i].confidence;
        obj_meta.x = dets[i].box.x_center;
        obj_meta.y = dets[i].box.y_center;
        obj_meta.w = dets[i].box.w;
        obj_meta.h = dets[i].box.h;

        frame_meta.obj_meta_list = &obj_meta;
        frame_meta.num_obj_meta += 1;
    }
    gst_buffer_add_frame_meta(outbuf, &frame_meta);

    // show the meta
    // FrameMeta* meta = gst_buffer_get_frame_meta(outbuf);
    // if (filter->silent == FALSE)
    //     // g_print("meta->num_obj_meta = %d.\n", meta->num_obj_meta);
    //   g_print ("meta->obj_meta_list[0]->class_id = %d.\n", meta->obj_meta_list->class_id);

    return GST_FLOW_OK;
}

/**
 * Initialize all resources and start the output thread
 */
static gboolean gst_nvinfer_start(GstBaseTransform* btrans) {
    GstInfer* filter = GST_INFER(btrans);

    char model_path[] = "/home/hsq/DeepLearning/code/MyNet/resource/model/RFB-320.mnn";
    filter->rfb320 = wrapper_rfb320_init(model_path);

    if (filter->silent == FALSE) {
        g_print("[hsq] start.\n");
    }

    return TRUE;
}

static gboolean gst_nvinfer_stop(GstBaseTransform* btrans) {
    GstInfer* filter = GST_INFER(btrans);

    wrapper_rfb320_delete(filter->rfb320);

    if (filter->silent == FALSE)
        g_print("[hsq] stop.\n");
    return TRUE;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
// static gboolean infer_init(GstPlugin* infer) {
//     return gst_element_register(infer, "infer", GST_RANK_NONE, GST_TYPE_INFER);
// }
static gboolean plugin_init(GstPlugin* plugin) {
    return gst_element_register(plugin, "infer", GST_RANK_NONE, GST_TYPE_INFER);
}

/* gstreamer looks for this structure to register infers
 *
 * FIXME:exchange the string 'Template infer' with you infer description
 */

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, infer, "Template infer", plugin_init, PACKAGE_VERSION,
                  GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
