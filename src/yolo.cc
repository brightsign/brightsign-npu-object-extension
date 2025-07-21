// Copyright (c) 2023 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "file_utils.h"
#include "image_utils.h"
#include "yolo.h"
#include "postprocess.h"

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

yolo_model_type_t detect_yolo_model_type(rknn_app_context_t *app_ctx)
{
    if (!app_ctx || !app_ctx->output_attrs) {
        return YOLO_UNKNOWN;
    }

    // Model type detection based on output structure:
    // - Standard YOLO (YOLOX): 3 outputs with 85 channels each [1, 85, H, W]
    // - Simplified YOLO (YoloV8): 9 outputs with different structure:
    //   * 3 box regression outputs (64 channels each): [1, 64, H, W]  
    //   * 3 class prediction outputs (80 channels each): [1, 80, H, W]
    //   * 3 objectness outputs (1 channel each): [1, 1, H, W]
    
    int n_outputs = app_ctx->io_num.n_output;
    
    if (n_outputs == 3) {
        // Standard YOLO (YOLOX) pattern: 3 outputs with 85 channels each
        // Check if all outputs have 85 channels (4 box + 1 obj + 80 classes)
        bool all_have_85_channels = true;
        for (int i = 0; i < 3; i++) {
            rknn_tensor_attr *output = &app_ctx->output_attrs[i];
            if (output->n_dims == 4 && output->fmt == RKNN_TENSOR_NCHW) {
                int channels = output->dims[1];
                if (channels != 85) {
                    all_have_85_channels = false;
                    break;
                }
            } else {
                all_have_85_channels = false;
                break;
            }
        }
        
        if (all_have_85_channels) {
            printf("Model type detection: Standard YOLO format detected (3 outputs, 85 channels each)\n");
            return YOLO_STANDARD;
        }
    }
    else if (n_outputs == 9) {
        // Simplified YOLO (YoloV8) pattern: 9 outputs in groups of 3
        // Expected pattern: 64-channel, 80-channel, 1-channel outputs repeated 3 times
        bool is_yolov8_pattern = true;
        int expected_channels[] = {64, 80, 1}; // Pattern repeated 3 times
        
        for (int i = 0; i < 9; i++) {
            rknn_tensor_attr *output = &app_ctx->output_attrs[i];
            int expected_ch = expected_channels[i % 3];
            
            if (output->n_dims == 4 && output->fmt == RKNN_TENSOR_NCHW) {
                int channels = output->dims[1];
                if (channels != expected_ch) {
                    is_yolov8_pattern = false;
                    break;
                }
            } else {
                is_yolov8_pattern = false;
                break;
            }
        }
        
        if (is_yolov8_pattern) {
            printf("Model type detection: Simplified YOLO format detected (9 outputs, YoloV8 pattern)\n");
            return YOLO_SIMPLIFIED;
        }
    }
    
    printf("Model type detection: Unable to determine model type (outputs: %d), defaulting to Standard YOLO\n", n_outputs);
    return YOLO_STANDARD;  // Default to Standard YOLO for backwards compatibility
}

int init_yolo_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL)
    {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0, NULL);
    free(model);
    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }


    // Set to context
    app_ctx->rknn_ctx = ctx;

    // Check if the model is quantized
    if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC && output_attrs[0].type == RKNN_TENSOR_INT8) {
        app_ctx->is_quant = true;
    } else {
        app_ctx->is_quant = false;
    }

    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        printf("model is NCHW input fmt\n");
        app_ctx->model_channel = input_attrs[0].dims[1];
        app_ctx->model_height = input_attrs[0].dims[2];
        app_ctx->model_width = input_attrs[0].dims[3];
    } else {
        printf("model is NHWC input fmt\n");
        app_ctx->model_height = input_attrs[0].dims[1];
        app_ctx->model_width = input_attrs[0].dims[2];
        app_ctx->model_channel = input_attrs[0].dims[3];
    }

    printf("model input height=%d, width=%d, channel=%d\n",
           app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    // Detect YOLO model type based on output tensor characteristics
    app_ctx->model_type = detect_yolo_model_type(app_ctx);
    const char* model_type_str = (app_ctx->model_type == YOLO_STANDARD) ? "Standard YOLO" : 
                                (app_ctx->model_type == YOLO_SIMPLIFIED) ? "Simplified YOLO" : "Unknown";
    printf("Detected model type: %s\n", model_type_str);

    return 0;
}

int release_yolo_model(rknn_app_context_t *app_ctx)
{    
    if (app_ctx->input_attrs != NULL)
    {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL)
    {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->rknn_ctx != 0)
    {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_yolo_model(rknn_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results, float conf_threshold) {
    int ret;
    image_buffer_t dst_img;
    letterbox_t letter_box;
    rknn_input inputs[app_ctx->io_num.n_input];
    rknn_output outputs[app_ctx->io_num.n_output];
    const float nms_threshold = NMS_THRESH;      // Default NMS threshold
    const float box_conf_threshold = 0.1f; // Use lower threshold to preserve more detections for visual feedback
    int bg_color = 114;  // Default letterbox background color for YOLO models
    
    if ((!app_ctx) || !(img) || (!od_results)) {
        return -1;
    }
    memset(od_results, 0x00, sizeof(*od_results));
    memset(&letter_box, 0, sizeof(letterbox_t));
    memset(&dst_img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Pre Process
    dst_img.width = app_ctx->model_width;
    dst_img.height = app_ctx->model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
    if (dst_img.virt_addr == NULL) {
        printf("malloc buffer size:%d fail!\n", dst_img.size);
        return -1;
    }

    // letterbox - maintain aspect ratio when resizing
    ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
    if (ret < 0) {
        printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        goto out;
    }

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
    inputs[0].buf = dst_img.virt_addr;

    ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        goto out;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        goto out;
    }

    // Get Output
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        outputs[i].index = i;
        outputs[i].want_float = (!app_ctx->is_quant);
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    // Post Process
    post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);

    // Remember to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);

out:
    if (dst_img.virt_addr != NULL) {
        free(dst_img.virt_addr);
    }
    
    return ret;
}