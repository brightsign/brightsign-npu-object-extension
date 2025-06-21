#ifndef INFERENCE_H
#define INFERENCE_H

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <opencv2/opencv.hpp>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include "queue.h"
#include "yolo.h"
#include "frame_writer.h"

// Struct to hold ML inference results
struct InferenceResult {
    object_detect_result_list detections;  // YOLO detection results
    std::chrono::system_clock::time_point timestamp;
};


class MLInferenceThread {
private:
    ThreadSafeQueue<InferenceResult>& resultQueue;
    std::atomic<bool>& running;
    int target_fps;
    int frames{0};
    const char* source_name;
    std::unique_ptr<rknn_app_context_t> rknn_app_ctx;
    std::shared_ptr<FrameWriter> frameWriter;
    
    // Simulated ML model inference
    InferenceResult runInference(cv::Mat& img);

public:
    MLInferenceThread(
        const char* model_path,
        const char* source_name,
        ThreadSafeQueue<InferenceResult>& queue, 
        std::atomic<bool>& isRunning,
        int target_fps,
        std::shared_ptr<FrameWriter> writer = nullptr);
    ~MLInferenceThread(); // Destructor declaration
    void operator()();
};

#endif // INFERENCE_H