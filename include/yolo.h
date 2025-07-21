#ifndef _RKNN_DEMO_MOBILENET_H_
#define _RKNN_DEMO_MOBILENET_H_

#include "rknn_api.h"
#include "common.h"

#define BOX_THRESH 0.25   // Default box confidence threshold
#define NMS_THRESH 0.45   // Default NMS threshold
#define OBJ_CLASS_NUM 80
#define OBJ_NUMB_MAX_SIZE 128
#define OBJ_NAME_MAX_SIZE 64

// YOLO model type enumeration
typedef enum {
    YOLO_STANDARD,    // Standard YOLO with DFL encoding and separate box/score tensors
    YOLO_SIMPLIFIED,  // Simplified YOLO with unified tensors and objectness scoring
    YOLO_UNKNOWN     // Unknown or unsupported model type
} yolo_model_type_t;

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    int model_channel;
    int model_width;
    int model_height;
    bool is_quant;
    yolo_model_type_t model_type;  // Detected YOLO model type
} rknn_app_context_t;

typedef struct box_rect_t {
    int left;    ///< Most left coordinate
    int top;     ///< Most top coordinate
    int right;   ///< Most right coordinate
    int bottom;  ///< Most bottom coordinate
} box_rect_t;

typedef struct object_detect_result {
    box_rect_t box;
    float prop;
    int cls_id;
    char name[OBJ_NAME_MAX_SIZE];
} object_detect_result_t;

typedef struct object_detect_result_list {
    int count;
    object_detect_result_t results[OBJ_NUMB_MAX_SIZE];
} object_detect_result_list;

int init_yolo_model(const char *model_path, rknn_app_context_t *app_ctx);
int release_yolo_model(rknn_app_context_t *app_ctx);
int inference_yolo_model(rknn_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results, float conf_threshold = BOX_THRESH);

// Model type detection function
yolo_model_type_t detect_yolo_model_type(rknn_app_context_t *app_ctx);

#endif //_RKNN_DEMO_MOBILENET_H_