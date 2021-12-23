#include "metadata.h"

// *****************************************
GType object_meta_api_get_type(void) {
    static GType type;
    static const gchar* tags[] = {OBJECT_META_STRING, NULL};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("ObjectMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

static gboolean object_meta_init(GstMeta* meta, gpointer params, GstBuffer* buffer) {
    ObjectMeta* emeta = (ObjectMeta*)meta;

    emeta->class_id = 0;
    emeta->confidence = 0;
    emeta->x = 0;
    emeta->y = 0;
    emeta->w = 0;
    emeta->h = 0;
    return TRUE;
}

static gboolean object_meta_transform(GstBuffer* transbuf, GstMeta* meta, GstBuffer* buffer, GQuark type,
                                      gpointer data) {
    ObjectMeta* emeta = (ObjectMeta*)meta;

    /* we always copy no matter what transform */
    gst_buffer_add_object_meta(transbuf, emeta->class_id, emeta->confidence, emeta->x, emeta->y, emeta->w, emeta->h);

    return TRUE;
}

static void object_meta_free(GstMeta* meta, GstBuffer* buffer) {
    // TODO:
    // ObjectMeta* emeta = (ObjectMeta*)meta;
    // g_free(emeta->name);
    // emeta->name = NULL;
}

const GstMetaInfo* object_meta_get_info(void) {
    static const GstMetaInfo* meta_info = NULL;

    if (g_once_init_enter(&meta_info)) {
        const GstMetaInfo* mi = gst_meta_register(OBJECT_META_API_TYPE, "ObjectMeta", sizeof(ObjectMeta),
                                                  object_meta_init, object_meta_free, object_meta_transform);
        g_once_init_leave(&meta_info, mi);
    }
    return meta_info;
}

ObjectMeta* gst_buffer_add_object_meta(GstBuffer* buffer, gint class_id, gfloat confidence, gfloat x, gfloat y,
                                       gfloat w, gfloat h) {
    ObjectMeta* meta;

    g_return_val_if_fail(GST_IS_BUFFER(buffer), NULL);

    meta = (ObjectMeta*)gst_buffer_add_meta(buffer, OBJECT_META_INFO, NULL);

    meta->class_id = class_id;
    meta->confidence = confidence;
    meta->x = x;
    meta->y = y;
    meta->w = w;
    meta->h = h;

    return meta;
}

// *****************************************
GType frame_meta_api_get_type(void) {
    static GType type;
    static const gchar* tags[] = {FRAME_META_STRING, NULL};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("FrameMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

static gboolean frame_meta_init(GstMeta* meta, gpointer params, GstBuffer* buffer) {
    FrameMeta* emeta = (FrameMeta*)meta;

    emeta->num_obj_meta = 0;
    emeta->obj_meta_list = NULL;

    return TRUE;
}

static gboolean frame_meta_transform(GstBuffer* transbuf, GstMeta* meta, GstBuffer* buffer, GQuark type,
                                     gpointer data) {
    FrameMeta* emeta = (FrameMeta*)meta;

    /* we always copy no matter what transform */
    // gst_buffer_add_frame_meta(transbuf, emeta->num_obj_meta, emeta->obj_meta_list);
    gst_buffer_add_frame_meta(transbuf, emeta);

    return TRUE;
}

static void frame_meta_free(GstMeta* meta, GstBuffer* buffer) {
    FrameMeta* emeta = (FrameMeta*)meta;
    // g_print ("free<---%p\n", emeta->obj_meta_list);
    g_free(emeta->obj_meta_list);
    emeta->obj_meta_list = NULL;
}

const GstMetaInfo* frame_meta_get_info(void) {
    static const GstMetaInfo* meta_info = NULL;

    if (g_once_init_enter(&meta_info)) {
        const GstMetaInfo* mi = gst_meta_register(FRAME_META_API_TYPE, "FrameMeta", sizeof(FrameMeta), frame_meta_init,
                                                  frame_meta_free, frame_meta_transform);
        g_once_init_leave(&meta_info, mi);
    }
    return meta_info;
}

// FrameMeta* gst_buffer_add_frame_meta(GstBuffer* buffer, guint num_obj_meta, const ObjectMeta* obj_meta_list) {
//     FrameMeta* meta;

//     g_return_val_if_fail(GST_IS_BUFFER(buffer), NULL);

//     meta = (FrameMeta*)gst_buffer_add_meta(buffer, FRAME_META_INFO, NULL);

//     meta->num_obj_meta = num_obj_meta;
//     meta->obj_meta_list = obj_meta_list;

//     return meta;
// }
FrameMeta* gst_buffer_add_frame_meta(GstBuffer* buffer, const FrameMeta* frame_meta) {
    FrameMeta* meta;

    g_return_val_if_fail(GST_IS_BUFFER(buffer), NULL);

    meta = (FrameMeta*)gst_buffer_add_meta(buffer, FRAME_META_INFO, NULL);
    
    ObjectMeta* obj_meta_list = (ObjectMeta*)g_malloc(frame_meta->num_obj_meta*sizeof(ObjectMeta));
    // g_print ("malloc--->%p\n", obj_meta_list);
    for(int i=0;i<frame_meta->num_obj_meta;i++){
        (obj_meta_list+i)->class_id = (frame_meta->obj_meta_list+i)->class_id;
        (obj_meta_list+i)->confidence = (frame_meta->obj_meta_list+i)->confidence;
        (obj_meta_list+i)->x = (frame_meta->obj_meta_list+i)->x;
        (obj_meta_list+i)->y = (frame_meta->obj_meta_list+i)->y;
        (obj_meta_list+i)->w = (frame_meta->obj_meta_list+i)->w;
        (obj_meta_list+i)->h = (frame_meta->obj_meta_list+i)->h;        
    }
    meta->num_obj_meta = frame_meta->num_obj_meta;
    meta->obj_meta_list = obj_meta_list;
        
    return meta;
}
