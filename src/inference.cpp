#include <cstdlib>
#include <iostream>
#include <thread>

#include "inference.h"
#include "yolo.h"
#include "postprocess.h"



void cv_to_image_buffer(cv::Mat& img, image_buffer_t* image) {
    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

    image->width = img.cols;
    image->height = img.rows;
    image->width_stride = img.cols;
    image->height_stride = img.rows;
    image->format = IMAGE_FORMAT_RGB888;
    image->virt_addr = img.data;
    image->size = img.cols * img.rows * 3;
    image->fd = -1;
}

InferenceResult MLInferenceThread::runInference(cv::Mat& cap) {
    // Check if the input image is valid
    if (cap.empty()) {
        printf("Error: Empty input image passed to runInference\n");
        object_detect_result_list empty_results;
        memset(&empty_results, 0, sizeof(empty_results));
        return InferenceResult{empty_results, std::chrono::system_clock::now(), selected_classes};
    }
    
    // Ensure the image has the right format
    if (cap.channels() != 3) {
        printf("Warning: Input image has %d channels, expecting 3. Attempting to convert.\n", cap.channels());
        if (cap.channels() == 1) {
            cv::cvtColor(cap, cap, cv::COLOR_GRAY2BGR);
        } else if (cap.channels() == 4) {
            cv::cvtColor(cap, cap, cv::COLOR_BGRA2BGR);
        } else {
            printf("Error: Unsupported channel count: %d\n", cap.channels());
            object_detect_result_list empty_results;
            memset(&empty_results, 0, sizeof(empty_results));
            return InferenceResult{empty_results, std::chrono::system_clock::now()};
        }
    }
    
    image_buffer_t image;
    memset(&image, 0, sizeof(image));
    try {
        cv_to_image_buffer(cap, &image);
    } catch (const std::exception& e) {
        printf("Exception in cv_to_image_buffer: %s\n", e.what());
        object_detect_result_list empty_results;
        memset(&empty_results, 0, sizeof(empty_results));
        return InferenceResult{empty_results, std::chrono::system_clock::now(), selected_classes};
    }

    object_detect_result_list empty_results;
    memset(&empty_results, 0, sizeof(empty_results));
    InferenceResult final_result{empty_results, std::chrono::system_clock::now(), selected_classes};

    printf("calling inference_yolo_model\n");
    object_detect_result_list results;
    memset(&results, 0, sizeof(results));  // Initialize results to avoid uninitialized data
    
    int ret = inference_yolo_model(rknn_app_ctx.get(), &image, &results);
    if (ret != 0) {
        printf("inference_yolo_model fail! ret=%d\n", ret);
        return final_result;
    }

    // Set the detection results
    final_result.detections = results;
    final_result.timestamp = std::chrono::system_clock::now();
    final_result.selected_classes = selected_classes;  // Pass selected classes along
    printf("inference_yolo_model success! count=%d\n", results.count);

    frames++;
    printf("Processed frame %d\n", frames);
    return final_result;
}

MLInferenceThread::MLInferenceThread(
        const char* model_path,
        const char* source_name,
        ThreadSafeQueue<InferenceResult>& queue, 
        std::atomic<bool>& isRunning,
        int target_fps,
        std::shared_ptr<FrameWriter> writer,
        const std::vector<int>& selected_classes)
    : resultQueue(queue), running(isRunning), target_fps(target_fps), frameWriter(writer), selected_classes(selected_classes) {
    
    // Store pointer to source name (argv remains valid)
    this->source_name = source_name;

    // Initialize post-processing
    init_post_process();
    
    // Create and initialize the model with dynamic allocation
    rknn_app_ctx = std::make_unique<rknn_app_context_t>();
    memset(rknn_app_ctx.get(), 0, sizeof(rknn_app_context_t));
    auto ret = init_yolo_model(model_path, rknn_app_ctx.get());
       if (ret != 0) {
        printf("init_yolo_model fail! ret=%d model_path=%s\n", ret, model_path);
        // return -1;
    }

    printf("done initializing MLInferenceThread\n");
}

MLInferenceThread::~MLInferenceThread() {
    if (rknn_app_ctx) {
        auto ret = release_yolo_model(rknn_app_ctx.get());
        if (ret != 0) {
            printf("release_yolo_model fail! ret=%d\n", ret);
        }  
    }

    running = false;
    resultQueue.signalShutdown();
}

void MLInferenceThread::runSingleInference() {
    printf("Running single inference on file: %s\n", source_name);
    
    // Load image from file
    cv::Mat img = cv::imread(source_name);
    if (img.empty()) {
        printf("Failed to load image from file: %s\n", source_name);
        running = false;
        return;
    }
    
    printf("Loaded image %dx%d from file: %s\n", img.cols, img.rows, source_name);
    
    // Run inference on the loaded image
    InferenceResult result = runInference(img);
    
    // Push result to queue for publisher to process
    resultQueue.push(result);
    
    // Write decorated frame if frameWriter is available
    if (frameWriter) {
        frameWriter->writeFrame(img, result);
    }
    
    printf("Single inference completed\n");
}

void MLInferenceThread::operator()() {
    // Create local capture object, just like in do_test()
    cv::VideoCapture capture;
    capture.open(source_name, cv::CAP_V4L2);

    if (!capture.isOpened()) {
        printf("Failed to open capture device: %s\n", source_name);
        running = false;
        return;
    }
    
    // Set camera properties
    printf("Setting camera resolution to 640x640...\n");
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 640);
    
    printf("Camera initialized successfully\n");
    
    while (running) {
        if (!capture.isOpened()) {
            printf("Capture is not opened\n");
            break;
        }

        auto frame_start_time = std::chrono::steady_clock::now();
        
        printf("Reading frame from capture\n");
        cv::Mat captured_img;
        try {
            // Use the same approach as main.cpp - single read() operation
            if (!capture.read(captured_img)) {
                printf("Failed to read frame from capture - device may be busy or disconnected\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            
            printf("Frame read from capture %d x %d\n", captured_img.size().width, captured_img.size().height);

            // Ensure the captured frame is not empty
            if (captured_img.empty()) {
                printf("Captured frame is empty\n");
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1000 / target_fps));
                continue;
            }
        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV exception caught: " << e.what() << std::endl;
            printf("Failed to read frame due to OpenCV exception: %s\n", e.what());
            break;
        } catch (const std::exception& e) {
            std::cerr << "Standard exception caught: " << e.what() << std::endl;
            printf("Failed to read frame due to standard exception: %s\n", e.what());
            break;
        } catch (...) {
            std::cerr << "Unknown exception caught!" << std::endl;
            printf("Failed to read frame due to unknown exception!\n");
            break;
        }

        printf("Running inference on frame\n");
        InferenceResult result;
        try {
            // Create a copy of the image to prevent potential memory issues
            cv::Mat frame_copy = captured_img.clone();
            if (frame_copy.empty()) {
                printf("Warning: Frame copy is empty, skipping inference\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / target_fps));
                continue;
            }
            
            // Run inference on the copied frame
            result = runInference(frame_copy);
            
            // Optionally write decorated frame using injected FrameWriter
            if (frameWriter) {
                frameWriter->writeFrame(frame_copy, result);
            }
            
            resultQueue.push(std::move(result));
            frame_copy.release();
        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV exception during inference: " << e.what() << std::endl;
            printf("Failed during inference due to OpenCV exception: %s\n", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;  // Continue instead of breaking to make the loop more resilient
        } catch (const std::exception& e) {
            std::cerr << "Standard exception during inference: " << e.what() << std::endl;
            printf("Failed during inference due to standard exception: %s\n", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        } catch (...) {
            std::cerr << "Unknown exception during inference!" << std::endl;
            printf("Failed during inference due to unknown exception!\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        auto current_time = std::chrono::steady_clock::now();
        auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>
            (current_time - frame_start_time);
        // FPS limiting
        auto frame_interval = std::chrono::milliseconds(1000 / target_fps);
        
        if (frame_interval > frame_duration) {
            auto sleep_time = std::chrono::duration_cast<std::chrono::milliseconds>
                (frame_interval - frame_duration);
            std::this_thread::sleep_for(sleep_time);
        }
    }
}