#include <iostream>
#include <cassert>
#include <chrono>
#include <cstring>
#include <vector>
#include <unordered_map>

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

void testMappedMessageFormatterExtraction() {
    std::cout << "Testing MappedMessageFormatter extraction..." << std::endl;
    
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
    
    // Test SelectiveJsonMessageFormatter
    SelectiveJsonMessageFormatter json_formatter;
    std::string json_output = json_formatter.formatMessage(result);
    
    std::cout << "JSON Output: " << json_output << std::endl;
    
    // Should contain "person":2 and "car":1, but not bicycle or motorcycle
    assert(json_output.find("\"person\":2") != std::string::npos);
    assert(json_output.find("\"car\":1") != std::string::npos);
    assert(json_output.find("bicycle") == std::string::npos);
    assert(json_output.find("motorcycle") == std::string::npos);
    assert(json_output.find("timestamp") != std::string::npos);
    
    // Test SelectiveBSMessageFormatter
    SelectiveBSMessageFormatter bs_formatter;
    std::string bs_output = bs_formatter.formatMessage(result);
    
    std::cout << "BS Output: " << bs_output << std::endl;
    
    // Should contain person:2 and car:1, but not bicycle or motorcycle
    assert(bs_output.find("person:2") != std::string::npos);
    assert(bs_output.find("car:1") != std::string::npos);
    assert(bs_output.find("bicycle") == std::string::npos);
    assert(bs_output.find("motorcycle") == std::string::npos);
    assert(bs_output.find("timestamp") != std::string::npos);
    
    std::cout << "✓ MappedMessageFormatter extraction test passed" << std::endl;
}

void testClassNameMapping() {
    std::cout << "Testing class name mapping..." << std::endl;
    
    // Create custom mapping: person -> people, car -> vehicle
    std::unordered_map<std::string, std::string> mapping = {
        {"person", "people"},
        {"car", "vehicle"}
    };
    
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),
        createMockDetection(2, "car", 0.8f),
        createMockDetection(7, "truck", 0.7f)  // No mapping
    };
    
    std::vector<int> selected_classes = {0, 2, 7};
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    // Test with mapping
    SelectiveJsonMessageFormatter json_formatter(mapping);
    std::string json_output = json_formatter.formatMessage(result);
    
    std::cout << "Mapped JSON Output: " << json_output << std::endl;
    
    // Should use mapped names where available
    assert(json_output.find("\"people\":1") != std::string::npos);
    assert(json_output.find("\"vehicle\":1") != std::string::npos);
    assert(json_output.find("\"truck\":1") != std::string::npos);  // Unmapped, should stay same
    assert(json_output.find("person") == std::string::npos);       // Original should not appear
    assert(json_output.find("car") == std::string::npos);          // Original should not appear
    
    std::cout << "✓ Class name mapping test passed" << std::endl;
}

void testEmptySelection() {
    std::cout << "Testing empty selection (backward compatibility)..." << std::endl;
    
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),
        createMockDetection(1, "bicycle", 0.7f),
        createMockDetection(2, "car", 0.8f)
    };
    
    // Empty selected_classes means all classes should be included
    std::vector<int> selected_classes = {};
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    SelectiveJsonMessageFormatter json_formatter;
    std::string json_output = json_formatter.formatMessage(result);
    
    std::cout << "Empty selection JSON Output: " << json_output << std::endl;
    
    // Should include all classes when selection is empty
    assert(json_output.find("\"person\":1") != std::string::npos);
    assert(json_output.find("\"bicycle\":1") != std::string::npos);
    assert(json_output.find("\"car\":1") != std::string::npos);
    
    std::cout << "✓ Empty selection test passed" << std::endl;
}

void testFacesFormatterMapping() {
    std::cout << "Testing Faces formatter with mapping..." << std::endl;
    
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),
        createMockDetection(0, "person", 0.8f),
        createMockDetection(2, "car", 0.7f)  // Should not affect faces count
    };
    
    InferenceResult result = createMockInferenceResult(detections);
    
    // Test FacesJsonMessageFormatter
    FacesJsonMessageFormatter faces_json_formatter;
    std::string faces_json_output = faces_json_formatter.formatMessage(result);
    
    std::cout << "Faces JSON Output: " << faces_json_output << std::endl;
    
    // Should have faces_in_frame_total and faces_attending both set to 2 (people count)
    assert(faces_json_output.find("\"faces_in_frame_total\":2") != std::string::npos);
    assert(faces_json_output.find("\"faces_attending\":2") != std::string::npos);
    assert(faces_json_output.find("timestamp") != std::string::npos);
    
    // Test FacesBSMessageFormatter
    FacesBSMessageFormatter faces_bs_formatter;
    std::string faces_bs_output = faces_bs_formatter.formatMessage(result);
    
    std::cout << "Faces BS Output: " << faces_bs_output << std::endl;
    
    // Should have both face properties set to 2
    assert(faces_bs_output.find("faces_in_frame_total:2") != std::string::npos);
    assert(faces_bs_output.find("faces_attending:2") != std::string::npos);
    assert(faces_bs_output.find("timestamp") != std::string::npos);
    
    std::cout << "✓ Faces formatter mapping test passed" << std::endl;
}

void testInvalidDetections() {
    std::cout << "Testing invalid detection filtering..." << std::endl;
    
    std::vector<object_detect_result> detections = {
        createMockDetection(0, "person", 0.9f),     // Valid
        createMockDetection(1, "bicycle", 0.0f),    // Invalid: zero confidence
        createMockDetection(-1, "invalid", 0.8f),   // Invalid: negative class_id
        createMockDetection(2, "car", 0.8f)         // Valid
    };
    
    std::vector<int> selected_classes = {0, 1, 2}; // All classes selected
    InferenceResult result = createMockInferenceResult(detections, selected_classes);
    
    SelectiveJsonMessageFormatter json_formatter;
    std::string json_output = json_formatter.formatMessage(result);
    
    std::cout << "Invalid detection filtering JSON Output: " << json_output << std::endl;
    
    // Should only include valid detections
    assert(json_output.find("\"person\":1") != std::string::npos);
    assert(json_output.find("\"car\":1") != std::string::npos);
    assert(json_output.find("bicycle") == std::string::npos); // Invalid confidence
    assert(json_output.find("invalid") == std::string::npos); // Invalid class_id
    
    std::cout << "✓ Invalid detection filtering test passed" << std::endl;
}

int main() {
    std::cout << "Running refactored publisher tests..." << std::endl;
    
    try {
        testMappedMessageFormatterExtraction();
        testClassNameMapping();
        testEmptySelection();
        testFacesFormatterMapping();
        testInvalidDetections();
        
        std::cout << "\n✅ All refactored publisher tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}