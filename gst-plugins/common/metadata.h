#include <gst/gst.h>

typedef struct {
    GstMeta meta;

    gint age;
    gchar* name;
} MyExampleMeta;

GType my_example_meta_api_get_type(void);
#define MY_EXAMPLE_META_API_TYPE (my_example_meta_api_get_type())

#define gst_buffer_get_my_example_meta(b) ((MyExampleMeta*)gst_buffer_get_meta((b), MY_EXAMPLE_META_API_TYPE))

/* implementation */
const GstMetaInfo* my_example_meta_get_info(void);
#define MY_EXAMPLE_META_INFO (my_example_meta_get_info())

MyExampleMeta* gst_buffer_add_my_example_meta(GstBuffer* buffer, gint age, const gchar* name);


// *****************************************
typedef struct {
    GstMeta meta;  // The first field in the structure must be a GstMeta

    gint class_id;
    gfloat confidence;
    gfloat x;
    gfloat y;
    gfloat w;
    gfloat h;
} ObjectMeta;

GType object_meta_api_get_type(void);
#define OBJECT_META_API_TYPE (object_meta_api_get_type())
#define gst_buffer_get_object_meta(b) ((ObjectMeta*)gst_buffer_get_meta((b), OBJECT_META_API_TYPE))

/* implementation */
const GstMetaInfo* object_meta_get_info(void);
#define OBJECT_META_INFO (object_meta_get_info())
ObjectMeta* gst_buffer_add_object_meta(GstBuffer* buffer, gint class_id, gfloat confidence, gfloat x, gfloat y,
                                       gfloat w, gfloat h);


// *****************************************
typedef struct {
    GstMeta meta;  // The first field in the structure must be a GstMeta

    /** Holds the number of object meta elements attached to current frame. */
    guint num_obj_meta;
    /** Holds a pointer to a list of pointers of type @ref NvDsObjectMeta
 in use for the frame. */
    ObjectMeta* obj_meta_list;
} FrameMeta;

GType frame_meta_api_get_type(void);
#define FRAME_META_API_TYPE (frame_meta_api_get_type())
#define gst_buffer_get_frame_meta(b) ((FrameMeta*)gst_buffer_get_meta((b), FRAME_META_API_TYPE))

/* implementation */
const GstMetaInfo* frame_meta_get_info(void);
#define FRAME_META_INFO (frame_meta_get_info())
// FrameMeta* gst_buffer_add_frame_meta(GstBuffer* buffer, guint num_obj_meta, const ObjectMeta* obj_meta_list);
FrameMeta* gst_buffer_add_frame_meta(GstBuffer* buffer, const FrameMeta* frame_meta);