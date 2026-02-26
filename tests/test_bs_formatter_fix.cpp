#include <iostream>
#include <cassert>
#include <chrono>
#include <cstring>
#include <vector>

// Include headers
#include "publisher.h"
#include "inference.h"
#include "utils.h"

// Mock detection result helper
object_detect_result createMockDetection(int cls_id, const char* name, float prop = 0.8f) {
    object_detect_result detection;
    detection.cls_id = cls_id;
    detection.prop = prop;
    strncpy(detection.name, name, sizeof(detection.name) - 1);
    detection.name[sizeof(detection.name) - 1] = '\0';
    detection.box = {10, 10, 50, 50};
    return detection;
}

// Mock inference result helper
InferenceResult createMockInferenceResult(
    const std::vector<object_detect_result>& detections,
    const std::vector<int>& selected_classes = {}
) {
    InferenceResult result;
    result.detections.count = detections.size();
    for (size_t i = 0; i < detections.size() && i < 100; ++i) {
        result.detections.results[i] = detections[i];
    }
    result.timestamp = std::chrono::system_clock::now();
    result.selected_classes = selected_classes;
    return result;
}

void testBSFormatterWithPersonDetections() {
    std::cout << "Testing BS formatter with person detections..." << std::endl;
    
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),
        createMockDetection(0, "person", 0.8f),
        createMockDetection(2, "car", 0.7f)
    };
    
    std::vector<int> selected_classes = {0, 2}; // person and car
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    SelectiveBSMessageFormatter bs_formatter;
    std::string bs_output = bs_formatter.formatMessage(result);
    
    std::cout << "BS Output with person detections: " << bs_output << std::endl;
    
    // Should have person:2!!car:1!!timestamp:XXXXX format
    assert(bs_output.find("person:2") != std::string::npos);
    assert(bs_output.find("car:1") != std::string::npos);
    assert(bs_output.find("!!timestamp:") != std::string::npos);
    
    // Check that separators are correct
    assert(bs_output.find("person:2!!car:1!!timestamp:") != std::string::npos ||
           bs_output.find("car:1!!person:2!!timestamp:") != std::string::npos);
    
    std::cout << "✓ BS formatter with person detections test passed" << std::endl;
}

void testBSFormatterWithNoPersonDetections() {
    std::cout << "Testing BS formatter with no person detections..." << std::endl;
    
    std::vector<object_detect_result> detections = {
        createMockDetection(2, "car", 0.7f),
        createMockDetection(7, "truck", 0.8f)
    };
    
    std::vector<int> selected_classes = {2, 7}; // car and truck, no person
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    SelectiveBSMessageFormatter bs_formatter;
    std::string bs_output = bs_formatter.formatMessage(result);
    
    std::cout << "BS Output with no person detections: " << bs_output << std::endl;
    
    // Should have person:0 since it's always included
    assert(bs_output.find("person:0") != std::string::npos);
    assert(bs_output.find("car:1") != std::string::npos);
    assert(bs_output.find("truck:1") != std::string::npos);
    assert(bs_output.find("!!timestamp:") != std::string::npos);
    
    // Should NOT have timestamp without separators
    assert(bs_output.find("timestamp:") == bs_output.rfind("timestamp:"));  // Only one occurrence
    
    std::cout << "✓ BS formatter with no person detections test passed" << std::endl;
}

void testBSFormatterWithNoDetections() {
    std::cout << "Testing BS formatter with no detections at all..." << std::endl;
    
    std::vector<object_detect_result> detections = {};
    std::vector<int> selected_classes = {0}; // person selected but no detections
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    SelectiveBSMessageFormatter bs_formatter;
    std::string bs_output = bs_formatter.formatMessage(result);
    
    std::cout << "BS Output with no detections: " << bs_output << std::endl;
    
    // Should have person:0!!timestamp:XXXXX format
    assert(bs_output.find("person:0") != std::string::npos);
    assert(bs_output.find("!!timestamp:") != std::string::npos);
    
    // Should NOT be just timestamp without person
    assert(bs_output.find("person:0!!timestamp:") != std::string::npos);
    
    std::cout << "✓ BS formatter with no detections test passed" << std::endl;
}

void testBSFormatterWithRemoteClass() {
    std::cout << "Testing BS formatter with remote class detection..." << std::endl;
    
    // Let's simulate detecting "remote" class (assuming it's class ID 72 in COCO)
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),
        createMockDetection(72, "remote", 0.8f)
    };
    
    std::vector<int> selected_classes = {0, 72}; // person and remote
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    SelectiveBSMessageFormatter bs_formatter;
    std::string bs_output = bs_formatter.formatMessage(result);
    
    std::cout << "BS Output with remote class: " << bs_output << std::endl;
    
    // Should have person:1!!remote:1!!timestamp:XXXXX format  
    assert(bs_output.find("person:1") != std::string::npos);
    assert(bs_output.find("remote:1") != std::string::npos);
    assert(bs_output.find("!!timestamp:") != std::string::npos);
    
    // Check that separators are correct
    assert(bs_output.find("person:1!!remote:1!!timestamp:") != std::string::npos ||
           bs_output.find("remote:1!!person:1!!timestamp:") != std::string::npos);
    
    std::cout << "✓ BS formatter with remote class test passed" << std::endl;
}

int main() {
    std::cout << "Running BS formatter fix tests..." << std::endl;
    
    try {
        testBSFormatterWithPersonDetections();
        testBSFormatterWithNoPersonDetections(); 
        testBSFormatterWithNoDetections();
        testBSFormatterWithRemoteClass();
        
        std::cout << "\n✅ All BS formatter fix tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}