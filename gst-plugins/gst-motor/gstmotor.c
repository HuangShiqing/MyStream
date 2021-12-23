/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2021 hsq <<user@hostname.org>>
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
 * SECTION:element-motor
 *
 * FIXME:Describe motor here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! motor ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#include "gstmotor.h"

#include <gst/base/base.h>
#include <gst/controller/controller.h>
#include <gst/gst.h>
#include <stdio.h>
#include <sys/time.h>

#include "config.h"
#include "metadata.h"

GST_DEBUG_CATEGORY_STATIC(gst_motor_debug);
#define GST_CAT_DEFAULT gst_motor_debug

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

#define gst_motor_parent_class parent_class
G_DEFINE_TYPE(GstMotor, gst_motor, GST_TYPE_BASE_SINK);

static void gst_motor_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_motor_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static gboolean gst_motor_start(GstBaseSink* btrans);
static gboolean gst_motor_stop(GstBaseSink* btrans);

static GstFlowReturn render(GstBaseSink* sink, GstBuffer* buffer);

double what_time_is_it_now() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

/* GObject vmethod implementations */

/* initialize the motor's class */
static void gst_motor_class_init(GstMotorClass* klass) {
    GObjectClass* gobject_class;
    GstElementClass* gstelement_class;

    gobject_class = (GObjectClass*)klass;
    gstelement_class = (GstElementClass*)klass;

    gobject_class->set_property = gst_motor_set_property;
    gobject_class->get_property = gst_motor_get_property;

    g_object_class_install_property(gobject_class, PROP_SILENT,
                                    g_param_spec_boolean("silent", "Silent", "Produce verbose output ?", FALSE,
                                                         G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

    gst_element_class_set_details_simple(gstelement_class, "Motor", "Generic/Filter", "FIXME:Generic Template Filter",
                                         "hsq <<user@hostname.org>>");

    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&sink_template));

    GST_BASE_SINK_CLASS(klass)->start = GST_DEBUG_FUNCPTR(gst_motor_start);
    GST_BASE_SINK_CLASS(klass)->stop = GST_DEBUG_FUNCPTR(gst_motor_stop);
    GST_BASE_SINK_CLASS(klass)->render = GST_DEBUG_FUNCPTR(render);

    /* debug category for fltering log messages
     *
     * FIXME:exchange the string 'Template motor' with your description
     */
    GST_DEBUG_CATEGORY_INIT(gst_motor_debug, "motor", 0, "Template motor");
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_motor_init(GstMotor* filter) {
    filter->silent = FALSE;
}

static void gst_motor_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
    GstMotor* filter = GST_MOTOR(object);

    switch (prop_id) {
        case PROP_SILENT:
            filter->silent = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_motor_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
    GstMotor* filter = GST_MOTOR(object);

    switch (prop_id) {
        case PROP_SILENT:
            g_value_set_boolean(value, filter->silent);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

#define CLOCKWISE 1
#define COUNTER_CLOCKWISE 2
int pins[4];
/* Suspend execution for x milliseconds intervals.
 *  *  @param ms Milliseconds to sleep.
 *   */
void delayMS(int x) {
    usleep(x * 1000);
}
/* Rotate the motor.
 *  *  @param pins     A pointer which points to the pins number array.
 *  *  @param direction  CLOCKWISE for clockwise rotation, COUNTER_CLOCKWISE for counter clockwise rotation.
 *  */
void rotate(int* pins, int direction) {
    for (int i = 0; i < 4; i++) {
        if (CLOCKWISE == direction) {
            for (int j = 0; j < 4; j++) {
                if (j == i) {
                    digitalWrite(pins[3 - j], 1);  // output a high level
                } else {
                    digitalWrite(pins[3 - j], 0);  // output a low level
                }
            }
        } else if (COUNTER_CLOCKWISE == direction) {
            for (int j = 0; j < 4; j++) {
                if (j == i) {
                    digitalWrite(pins[j], 1);  // output a high level
                } else {
                    digitalWrite(pins[j], 0);  // output a low level
                }
            }
        }
        delayMS(4);
    }
}
/**
 * Initialize all resources and start the output thread
 */
static gboolean gst_motor_start(GstBaseSink* btrans) {
    GstMotor* filter = GST_MOTOR(btrans);

    if (filter->silent == FALSE) {
        g_print("[hsq] gst_motor start.\n");
    }

    int pinA = 0;  // atoi(argv[1]);
    int pinB = 1;  // atoi(argv[2]);
    int pinC = 2;  // atoi(argv[3]);
    int pinD = 3;  // atoi(argv[4]);

    pins[0] = pinA;
    pins[1] = pinB;
    pins[2] = pinC;
    pins[3] = pinD;

    if (-1 == wiringPiSetup()) {
        printf("Setup wiringPi failed!");
        return FALSE;
    }

    /* set mode to output */
    pinMode(pinA, OUTPUT);
    pinMode(pinB, OUTPUT);
    pinMode(pinC, OUTPUT);
    pinMode(pinD, OUTPUT);

    delayMS(50);  // wait for a stable status

    return TRUE;
}

static gboolean gst_motor_stop(GstBaseSink* btrans) {
    GstMotor* filter = GST_MOTOR(btrans);

    if (filter->silent == FALSE)
        g_print("[hsq] gst_motor stop.\n");
    return TRUE;
}

/* GstBaseSink vmethod implementations */
static GstFlowReturn render(GstBaseSink* sink, GstBuffer* buffer) {
    GstMotor* filter = GST_MOTOR(sink);

    // if (filter->silent == FALSE)
    //     g_print("I'm plugged, therefore I'm in.\n");

    static GQuark framemeta_quark = 0;
    if (!framemeta_quark) {
        framemeta_quark = g_quark_from_static_string(FRAME_META_STRING);
    }

    gpointer state = NULL;
    GstMeta* gst_meta;
    while ((gst_meta = gst_buffer_iterate_meta(buffer, &state))) {
        if (gst_meta_api_type_has_tag(gst_meta->info->api, framemeta_quark)) {
            // g_print("get frame_meta\r\n");
            FrameMeta* meta = (FrameMeta*)gst_meta;

            int WIDTH = 320;  // TODO:
            if (meta->num_obj_meta > 0) {
                ObjectMeta* obj = meta->obj_meta_list;

                int delta = obj->x - 0.5 * WIDTH;
                if (obj->confidence > 0.8) {
                    if (delta > 10) {  //右侧
                        // printf("bbox.x*WIDTH: %f, in right\r\n",det->bbox.x*WIDTH);
                        // for(int i=0;i<10;i++)
                        rotate(pins, COUNTER_CLOCKWISE);
                    } else if (delta < -10 && delta > -(WIDTH / 2)) {  //左侧
                        // printf("bbox.x*WIDTH: %f, in left\r\n",det->bbox.x*WIDTH);
                        // for(int i=0;i<10;i++)
                        rotate(pins, CLOCKWISE);
                    } else {  //死区
                        ;
                        // printf("in middle\r\n");
                    }
                }
                // delayMS(50);
            }
        }
    }

    return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init(GstPlugin* motor) {
    return gst_element_register(motor, "motor", GST_RANK_NONE, GST_TYPE_MOTOR);
}

/* gstreamer looks for this structure to register motors
 *
 * FIXME:exchange the string 'Template motor' with you motor description
 */
// GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
//     GST_VERSION_MINOR,
//     motor,
//     "Template motor",
//     motor_init,
//     PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, motor, "Template infer", plugin_init, PACKAGE_VERSION,
                  GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
