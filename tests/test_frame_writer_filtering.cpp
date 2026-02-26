#include <iostream>
#include <cassert>
#include <chrono>
#include <cstring>
#include <vector>
#include <opencv2/opencv.hpp>

// Include headers
#include "frame_writer.h"
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

void testFrameWriterSelectiveFiltering() {
    std::cout << "Testing frame writer selective filtering..." << std::endl;
    
    // Create test image
    cv::Mat test_frame(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    
    // Create mock detections with mixed classes
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),    // Selected
        createMockDetection(1, "bicycle", 0.7f),   // Not selected
        createMockDetection(0, "person", 0.8f),    // Selected
        createMockDetection(2, "car", 0.85f),      // Selected  
        createMockDetection(3, "motorcycle", 0.6f) // Not selected
    };
    
    // Set selected classes: person (0) and car (2)
    std::vector<int> selected_classes = {0, 2};
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    // Test DecoratedFrameWriter with selective filtering
    DecoratedFrameWriter frame_writer("/tmp/test_selective_output.jpg", false);
    
    // Create copy of frame for testing
    cv::Mat test_frame_copy = test_frame.clone();
    
    // This should only draw boxes for person and car detections
    frame_writer.writeFrame(test_frame_copy, result);
    
    // Verify the image was written
    cv::Mat written_image = cv::imread("/tmp/test_selective_output.jpg");
    assert(!written_image.empty());
    
    std::cout << "✓ Frame writer selective filtering test passed" << std::endl;
}

void testFrameWriterEmptySelection() {
    std::cout << "Testing frame writer with empty selection (all classes)..." << std::endl;
    
    // Create test image
    cv::Mat test_frame(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    
    // Create mock detections
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),
        createMockDetection(1, "bicycle", 0.7f),
        createMockDetection(2, "car", 0.8f)
    };
    
    // Empty selected_classes means all classes should be drawn
    std::vector<int> selected_classes = {};
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    // Test DecoratedFrameWriter with all classes
    DecoratedFrameWriter frame_writer("/tmp/test_all_classes_output.jpg", false);
    
    // Create copy of frame for testing
    cv::Mat test_frame_copy = test_frame.clone();
    
    // This should draw boxes for all detections
    frame_writer.writeFrame(test_frame_copy, result);
    
    // Verify the image was written
    cv::Mat written_image = cv::imread("/tmp/test_all_classes_output.jpg");
    assert(!written_image.empty());
    
    std::cout << "✓ Frame writer empty selection test passed" << std::endl;
}

void testFrameWriterSuppressEmpty() {
    std::cout << "Testing frame writer with suppress empty and no selected detections..." << std::endl;
    
    // Create test image
    cv::Mat test_frame(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    
    // Create mock detections that won't be selected
    std::vector<object_detect_result> detections = {
        createMockDetection(1, "bicycle", 0.7f),   // Not selected
        createMockDetection(3, "motorcycle", 0.6f) // Not selected
    };
    
    // Select only person class (0) which is not in detections
    std::vector<int> selected_classes = {0};
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    // Test DecoratedFrameWriter with suppress_empty enabled
    DecoratedFrameWriter frame_writer("/tmp/test_suppress_empty_output.jpg", true);
    
    // Create copy of frame for testing
    cv::Mat test_frame_copy = test_frame.clone();
    
    // This should draw "none" text since no selected classes are detected
    frame_writer.writeFrame(test_frame_copy, result);
    
    // Verify the image was written
    cv::Mat written_image = cv::imread("/tmp/test_suppress_empty_output.jpg");
    assert(!written_image.empty());
    
    std::cout << "✓ Frame writer suppress empty test passed" << std::endl;
}

void testIsClassSelectedFunction() {
    std::cout << "Testing isClassSelected utility function..." << std::endl;
    
    // Test with specific classes selected
    std::vector<int> selected_classes = {0, 2, 5};
    
    assert(isClassSelected(0, selected_classes) == true);
    assert(isClassSelected(1, selected_classes) == false);
    assert(isClassSelected(2, selected_classes) == true);
    assert(isClassSelected(3, selected_classes) == false);
    assert(isClassSelected(5, selected_classes) == true);
    assert(isClassSelected(10, selected_classes) == false);
    
    // Test with empty selection (all classes selected)
    std::vector<int> empty_selection = {};
    assert(isClassSelected(0, empty_selection) == true);
    assert(isClassSelected(1, empty_selection) == true);
    assert(isClassSelected(100, empty_selection) == true);
    
    std::cout << "✓ isClassSelected function test passed" << std::endl;
}

int main() {
    std::cout << "Running frame writer filtering tests..." << std::endl;
    
    try {
        testIsClassSelectedFunction();
        testFrameWriterSelectiveFiltering();
        testFrameWriterEmptySelection();
        testFrameWriterSuppressEmpty();
        
        std::cout << "\n✅ All frame writer filtering tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}