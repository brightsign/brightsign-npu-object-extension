#include <chrono>
#include <memory>
#include <sys/time.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <string>
#include <thread>
#include <cstring>
#include <signal.h>

#include "image_utils.h"
#include "inference.h"
#include "publisher.h"
#include "queue.h"
#include "transport.h"
#include "utils.h"
#include "yolo.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <unordered_map>


std::atomic<bool> running{true};
ThreadSafeQueue<InferenceResult> resultQueue(1);

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";

    // Cleanup and shutdown
    running = false;
    resultQueue.signalShutdown();
}

int main(int argc, char **argv) {
    char *model_name = NULL;
    char *source_name = NULL;
    bool suppress_empty = false;
    bool is_file_input = false;
    std::string classes_str;
    
    if (argc < 3) {
        printf("Usage: %s <rknn model> <source> [--suppress-empty] [--classes class1,class2,...]\n", argv[0]);
        printf("  <source>: V4L device (e.g. /dev/video0) or image file (e.g. /tmp/bus.jpg)\n");
        printf("  --suppress-empty: suppress output when no detections (optional)\n");
        printf("  --classes: comma-separated list of class names to detect (optional)\n");
        return -1;
    }

    // The path where the model is located
    model_name = (char *)argv[1];
    source_name = argv[2];
    
    // Parse optional flags
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--suppress-empty") == 0) {
            suppress_empty = true;
            printf("Suppress-empty mode enabled\n");
        } else if (strcmp(argv[i], "--classes") == 0) {
            if (i + 1 < argc) {
                classes_str = argv[i + 1];
                printf("Selected classes: %s\n", classes_str.c_str());
                i++;  // Skip the next argument as it's the classes string
            } else {
                printf("Error: --classes flag requires a value\n");
                return -1;
            }
        } else {
            printf("Warning: Unknown flag '%s'\n", argv[i]);
        }
    }
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Parse class names if provided
    std::vector<int> selected_classes = {}; // Default to include class 0 (usually "person" in COCO)

    if (!classes_str.empty()) {
        // Determine model directory to find labels file
        std::string model_dir = std::filesystem::path(model_name).parent_path();
        if (model_dir.empty()) {
            model_dir = ".";
        }
        std::string labels_path = model_dir + "/coco_80_labels_list.txt";
        
        // Try alternative paths if not found
        if (!std::filesystem::exists(labels_path)) {
            labels_path = "model/coco_80_labels_list.txt";
        }
        if (!std::filesystem::exists(labels_path)) {
            labels_path = "../model/coco_80_labels_list.txt";
        }
        
        auto class_mapping = loadCocoClassMapping(labels_path);
        if (class_mapping.empty()) {
            printf("Error: Could not load COCO class mapping from %s\n", labels_path.c_str());
            return -1;
        }
        
        selected_classes = parseClassNames(classes_str, class_mapping);
        if (selected_classes.empty() && !classes_str.empty()) {
            printf("Warning: No valid classes found in '%s', using all classes\n", classes_str.c_str());
        }
    }
    // ensure selected_classes always has class 0
    if (std::find(selected_classes.begin(), selected_classes.end(), 0) == selected_classes.end()) {
        selected_classes.push_back(0);
    }
    
    // Determine if source is a file or device
    if (strstr(source_name, "/dev/video") == source_name) {
        is_file_input = false;
        printf("Using V4L device: %s\n", source_name);
    } else if (std::filesystem::exists(source_name)) {
        is_file_input = true;
        printf("Using image file: %s\n", source_name);
    } else {
        printf("Error: Source '%s' is neither a valid V4L device nor an existing file\n", source_name);
        return -1;
    }

    // Create frame writer for decorated output
    auto frameWriter = std::make_shared<DecoratedFrameWriter>("/tmp/output.jpg", suppress_empty);
    
    if (is_file_input) {
        // Single-shot inference mode for file input
        MLInferenceThread mlThread(
            model_name,
            source_name,
            resultQueue, 
            running,
            1, // Single frame
            frameWriter,
            selected_classes);
        
        // Create formatters
        auto json_formatter = std::make_shared<JsonMessageFormatter>(suppress_empty);
        
        // Create file publisher using transport injection
        auto file_transport = std::make_shared<FileTransport>("/tmp/results.json");
        Publisher file_publisher(
            file_transport,
            resultQueue,
            running,
            json_formatter,
            1);
        
        // Run single inference and exit
        mlThread.runSingleInference();
        
        // Process result if any
        std::thread file_publisherThread(std::ref(file_publisher));
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Give time for processing
        running = false;
        resultQueue.signalShutdown();
        file_publisherThread.join();
        
    } else {
        // Continuous inference mode for video device
        MLInferenceThread mlThread(
            model_name,
            source_name,
            resultQueue, 
            running,
            30,
            frameWriter,
            selected_classes);

        // Create formatters
        auto json_formatter = std::make_shared<JsonMessageFormatter>(suppress_empty);
        auto selective_json_formatter = std::make_shared<SelectiveJsonMessageFormatter>();
        auto selective_bs_formatter = std::make_shared<SelectiveBSMessageFormatter>();
        
        // Create file publisher using transport injection
        auto file_transport = std::make_shared<FileTransport>("/tmp/results.json");
        Publisher file_publisher(
            file_transport,
            resultQueue,
            running,
            json_formatter,
            1); // Write to file once per second

        // Create UDP publishers for selective class data
        UDPPublisher udp_json_publisher(
            "127.0.0.1", 5002,
            resultQueue,
            running,
            selective_json_formatter,
            1); // Send JSON to port 5002

        UDPPublisher udp_bs_publisher(
            "127.0.0.1", 5000,
            resultQueue,
            running,
            selective_bs_formatter,
            1); // Send BrightScript to port 5000

        std::thread inferenceThread(std::ref(mlThread));
        std::thread file_publisherThread(std::ref(file_publisher));
        std::thread udp_json_publisherThread(std::ref(udp_json_publisher));
        std::thread udp_bs_publisherThread(std::ref(udp_bs_publisher));

        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Cleanup and shutdown
        running = false;
        resultQueue.signalShutdown();

        inferenceThread.join();
        file_publisherThread.join();
        udp_json_publisherThread.join();
        udp_bs_publisherThread.join();
    }

    return 0;
}