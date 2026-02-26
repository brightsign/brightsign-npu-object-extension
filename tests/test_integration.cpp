#include <iostream>
#include <cassert>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

#include "utils.h"
#include "inference.h"
#include "publisher.h"
#include "frame_writer.h"
#include "transport.h"

using json = nlohmann::json;

// Helper function to check if a file exists
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

// Helper function to read JSON file
json readJsonFile(const std::string& filename) {
    std::ifstream file(filename);
    json j;
    file >> j;
    return j;
}

void test_selectiveClassFiltering() {
    std::cout << "Testing selective class filtering..." << std::endl;
    
    // Load class mapping
    auto class_mapping = loadCocoClassMapping("data/coco_80_labels_list.txt");
    assert(!class_mapping.empty());
    
    // Test with "person" class only
    auto selected_classes = parseClassNames("person", class_mapping);
    assert(selected_classes.size() == 1);
    assert(selected_classes[0] == 0);  // person is class 0
    
    // Test isClassSelected function
    assert(isClassSelected(0, selected_classes) == true);   // person
    assert(isClassSelected(1, selected_classes) == false);  // bicycle
    assert(isClassSelected(2, selected_classes) == false);  // car
    
    // Test with multiple classes
    selected_classes = parseClassNames("person,car,dog", class_mapping);
    assert(selected_classes.size() == 3);
    assert(isClassSelected(0, selected_classes) == true);   // person
    assert(isClassSelected(2, selected_classes) == true);   // car
    assert(isClassSelected(16, selected_classes) == true);  // dog
    assert(isClassSelected(1, selected_classes) == false);  // bicycle
    
    std::cout << "✓ Selective class filtering test passed" << std::endl;
}

void test_publisherFormatters() {
    std::cout << "Testing publisher formatters..." << std::endl;
    
    // Create mock detection results
    object_detect_result_list detections;
    memset(&detections, 0, sizeof(detections));
    detections.count = 3;
    
    // Add person detection
    detections.results[0].cls_id = 0;
    detections.results[0].prop = 0.9f;
    strcpy(detections.results[0].name, "person");
    
    // Add car detection
    detections.results[1].cls_id = 2;
    detections.results[1].prop = 0.8f;
    strcpy(detections.results[1].name, "car");
    
    // Add dog detection
    detections.results[2].cls_id = 16;
    detections.results[2].prop = 0.7f;
    strcpy(detections.results[2].name, "dog");
    
    // Create inference result with selected classes (person, car only)
    InferenceResult result;
    result.detections = detections;
    result.timestamp = std::chrono::system_clock::now();
    result.selected_classes = {0, 2};  // person, car
    result.class_mapping = class_mapping;
    
    // Test SelectiveJsonMessageFormatter
    SelectiveJsonMessageFormatter json_formatter;
    std::string json_output = json_formatter.formatMessage(result);
    json j = json::parse(json_output);
    
    assert(j.contains("person"));
    assert(j.contains("car"));
    assert(!j.contains("dog"));  // Should not be included
    assert(j["person"] == 1);
    assert(j["car"] == 1);
    assert(j.contains("timestamp"));
    
    // Test SelectiveBSMessageFormatter
    SelectiveBSMessageFormatter bs_formatter;
    std::string bs_output = bs_formatter.formatMessage(result);
    
    assert(bs_output.find("person:1") != std::string::npos);
    assert(bs_output.find("car:1") != std::string::npos);
    assert(bs_output.find("dog:1") == std::string::npos);  // Should not be included
    assert(bs_output.find("timestamp:") != std::string::npos);
    
    std::cout << "✓ Publisher formatters test passed" << std::endl;
}

void test_frameWriterSelection() {
    std::cout << "Testing frame writer selection..." << std::endl;
    
    // Create a test image
    cv::Mat test_image = cv::Mat::zeros(480, 640, CV_8UC3);
    cv::rectangle(test_image, cv::Rect(100, 100, 200, 200), cv::Scalar(255, 255, 255), -1);
    
    // Create mock detection results
    object_detect_result_list detections;
    memset(&detections, 0, sizeof(detections));
    detections.count = 2;
    
    // Add person detection
    detections.results[0].cls_id = 0;
    detections.results[0].prop = 0.9f;
    detections.results[0].box = {50, 50, 150, 150};
    strcpy(detections.results[0].name, "person");
    
    // Add car detection
    detections.results[1].cls_id = 2;
    detections.results[1].prop = 0.8f;
    detections.results[1].box = {200, 200, 300, 300};
    strcpy(detections.results[1].name, "car");
    
    // Create inference result with person only selected
    InferenceResult result;
    result.detections = detections;
    result.timestamp = std::chrono::system_clock::now();
    result.selected_classes = {0};  // person only
    result.class_mapping = class_mapping;
    
    // Test DecoratedFrameWriter
    DecoratedFrameWriter writer("/tmp/test_selective_output.jpg", false);
    writer.writeFrame(test_image, result);
    
    // Check that output file was created
    assert(fileExists("/tmp/test_selective_output.jpg"));
    
    std::cout << "✓ Frame writer selection test passed" << std::endl;
}

void test_backwardCompatibility() {
    std::cout << "Testing backward compatibility..." << std::endl;
    
    // Test with empty selected classes (should select all)
    std::vector<int> empty_classes;
    
    assert(isClassSelected(0, empty_classes) == true);   // person
    assert(isClassSelected(1, empty_classes) == true);   // bicycle
    assert(isClassSelected(2, empty_classes) == true);   // car
    assert(isClassSelected(79, empty_classes) == true);  // toothbrush
    
    // Create inference result with empty selected classes
    object_detect_result_list detections;
    memset(&detections, 0, sizeof(detections));
    detections.count = 1;
    detections.results[0].cls_id = 5;
    detections.results[0].prop = 0.8f;
    strcpy(detections.results[0].name, "bus");
    
    InferenceResult result;
    result.detections = detections;
    result.timestamp = std::chrono::system_clock::now();
    result.selected_classes = {};  // Empty = all classes
    result.class_mapping = class_mapping;
    
    // Test that formatters work with empty selection
    SelectiveJsonMessageFormatter json_formatter;
    std::string json_output = json_formatter.formatMessage(result);
    json j = json::parse(json_output);
    
    assert(j.contains("bus"));
    assert(j["bus"] == 1);
    
    std::cout << "✓ Backward compatibility test passed" << std::endl;
}

int main() {
    std::cout << "Running integration tests..." << std::endl;
    
    try {
        test_selectiveClassFiltering();
        test_publisherFormatters();
        test_frameWriterSelection();
        test_backwardCompatibility();
        
        std::cout << "All integration tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed: " << e.what() << std::endl;
        return 1;
    }
}