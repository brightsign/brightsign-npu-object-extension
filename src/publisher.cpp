#include "publisher.h"
#include "utils.h"

#include <iostream>
#include <thread>
#include <unordered_map>

// Implementation of the JsonMessageFormatter
std::string JsonMessageFormatter::formatMessage(const InferenceResult& result) {
    json j;
    
    // Add timestamp
    j["timestamp"] = std::chrono::system_clock::to_time_t(result.timestamp);
    
    // Count detections by class name for selected classes only
    std::unordered_map<std::string, int> class_counts {{"person", 0}};
    
    for (int i = 0; i < result.detections.count; ++i) {
        const auto& detection = result.detections.results[i];
        
        // Skip invalid detections or if not in selected classes
        if ((detection.prop <= 0.0f ||  detection.cls_id < 0) || 
            !isClassSelected(detection.cls_id, result.selected_classes)) {
            continue;
        }
        
        std::string class_name = std::string(detection.name);
        class_counts[class_name]++;
    }
    
    // Add class counts to JSON
    for (const auto& [class_name, count] : class_counts) {
        j[class_name] = count;
    }
    
    return j.dump();
}

// Implementation of MappedMessageFormatter helper functions
std::vector<object_detect_result> MappedMessageFormatter::extractSelectedClasses(const InferenceResult& result) const {
    std::vector<object_detect_result> selected_detections;
    
    for (int i = 0; i < result.detections.count; ++i) {
        const auto& detection = result.detections.results[i];
        
        // Skip invalid detections
        if (detection.prop <= 0.0f || detection.cls_id < 0) {
            continue;
        }
        
        // Only include selected classes
        if (isClassSelected(detection.cls_id, result.selected_classes)) {
            selected_detections.push_back(detection);
        }
    }
    
    return selected_detections;
}

std::string MappedMessageFormatter::mapClassName(const std::string& original_name) const {
    auto it = class_mapping.find(original_name);
    if (it != class_mapping.end()) {
        return it->second;
    }
    return original_name;
}

// Implementation of the BSVariableMessageFormatter
std::string BSVariableMessageFormatter::formatMessage(const InferenceResult& result) {
    // format the message as a string like detection_count:2!!timestamp:1746732409
    std::string message = 
        "detection_count:" + std::to_string(result.detections.count) + "!!" +
        "timestamp:" + std::to_string(std::chrono::system_clock::to_time_t(result.timestamp));
    return message;
}

// Implementation of the FacesJsonMessageFormatter constructor
FacesJsonMessageFormatter::FacesJsonMessageFormatter() {
    class_mapping["person"] = "faces";
}

// Implementation of the FacesJsonMessageFormatter
std::string FacesJsonMessageFormatter::formatMessage(const InferenceResult& result) {
    json j;
    
    // Count people (class_id == 0)
    int people_count = 0;
    for (int i = 0; i < result.detections.count; ++i) {
        const auto& detection = result.detections.results[i];
        if (detection.cls_id == 0) { // "person" class
            people_count++;
        }
    }
    
    // Map people count to faces properties (doubling up as requested)
    j["faces_in_frame_total"] = people_count;
    j["faces_attending"] = people_count;
    j["timestamp"] = std::chrono::system_clock::to_time_t(result.timestamp);
    
    return j.dump();
}

// Implementation of the FacesBSMessageFormatter constructor
FacesBSMessageFormatter::FacesBSMessageFormatter() {
    class_mapping["person"] = "faces";
}

// Implementation of the FacesBSMessageFormatter  
std::string FacesBSMessageFormatter::formatMessage(const InferenceResult& result) {
    // Count people (class_id == 0)
    int people_count = 0;
    for (int i = 0; i < result.detections.count; ++i) {
        const auto& detection = result.detections.results[i];
        if (detection.cls_id == 0) { // "person" class
            people_count++;
        }
    }
    
    // Map people count to faces properties in BrightScript format
    std::string message = 
        "faces_in_frame_total:" + std::to_string(people_count) + "!!" +
        "faces_attending:" + std::to_string(people_count) + "!!" +
        "timestamp:" + std::to_string(std::chrono::system_clock::to_time_t(result.timestamp));
    return message;
}

// Implementation of the SelectiveJsonMessageFormatter
std::string SelectiveJsonMessageFormatter::formatMessage(const InferenceResult& result) {
    json j;
    
    // Add timestamp
    j["timestamp"] = std::chrono::system_clock::to_time_t(result.timestamp);
    
    // Extract only selected class detections
    std::vector<object_detect_result> selected_detections = extractSelectedClasses(result);
    
    // Count detections by class name for selected classes only
    std::unordered_map<std::string, int> class_counts {{"person", 0}};
    
    for (const auto& detection : selected_detections) {
        std::string class_name = std::string(detection.name);
        std::string mapped_name = mapClassName(class_name);
        class_counts[mapped_name]++;
    }
    
    // Add class counts to JSON
    for (const auto& [class_name, count] : class_counts) {
        j[class_name] = count;
    }
    
    return j.dump();
}

// Implementation of the SelectiveBSMessageFormatter
std::string SelectiveBSMessageFormatter::formatMessage(const InferenceResult& result) {
    // Extract only selected class detections
    std::vector<object_detect_result> selected_detections = extractSelectedClasses(result);
    
    // Count detections by class name for selected classes only, always include person
    std::unordered_map<std::string, int> class_counts{{"person", 0}};
    
    for (const auto& detection : selected_detections) {
        std::string class_name = std::string(detection.name);
        std::string mapped_name = mapClassName(class_name);
        class_counts[mapped_name]++;
    }
    
    // Build BrightScript format message
    std::string message;
    bool first = true;
    
    for (const auto& [class_name, count] : class_counts) {
        if (!first) {
            message += "!!";
        }
        message += class_name + ":" + std::to_string(count);
        first = false;
    }
    
    // Add timestamp
    if (!message.empty()) {
        message += "!!";
    }
    message += "timestamp:" + std::to_string(std::chrono::system_clock::to_time_t(result.timestamp));
    
    return message;
}

// Generic Publisher implementation
Publisher::Publisher(
        std::shared_ptr<Transport> transport,
        ThreadSafeQueue<InferenceResult>& queue, 
        std::atomic<bool>& isRunning,
        std::shared_ptr<MessageFormatter> formatter,
        int messages_per_second)
    : transport(transport),
      resultQueue(queue), 
      running(isRunning), 
      target_mps(messages_per_second),
      formatter(formatter) {
}

void Publisher::operator()() {
    InferenceResult result;
    while (resultQueue.pop(result)) {
        if (!transport->isConnected()) {
            std::cerr << "Transport not connected, skipping message" << std::endl;
            continue;
        }
        
        std::string message = formatter->formatMessage(result);
        
        if (!transport->send(message)) {
            std::cerr << "Failed to send message via transport" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / target_mps));
    }
}

// Implementation of the FullJsonMessageFormatter
std::string FullJsonMessageFormatter::formatMessage(const InferenceResult& result) {
    json j;
    
    // Add timestamp
    j["timestamp"] = std::chrono::system_clock::to_time_t(result.timestamp);
    
    // Create detections array
    json detections = json::array();
    
    for (int i = 0; i < result.detections.count; ++i) {
        const auto& detection = result.detections.results[i];
        
        // Skip invalid detections or if not in selected classes
        if ((detection.prop <= 0.0f || detection.cls_id < 0) || 
            !isClassSelected(detection.cls_id, result.selected_classes)) {
            continue;
        }
        
        // Create detection object with all details
        json det;
        det["class_id"] = detection.cls_id;
        det["class_name"] = std::string(detection.name);
        det["confidence"] = detection.prop;
        det["bbox"] = {
            {"left", detection.box.left},
            {"top", detection.box.top},
            {"right", detection.box.right},
            {"bottom", detection.box.bottom}
        };
        
        detections.push_back(det);
    }
    
    // Add detections array to JSON
    j["detections"] = detections;
    j["detection_count"] = detections.size();
    
    // Handle suppress_empty flag
    if (suppress_empty && detections.empty()) {
        return "";
    }
    
    return j.dump();
}

// UDPPublisher backward compatibility wrapper
UDPPublisher::UDPPublisher(
        const std::string& ip,
        const int port,
        ThreadSafeQueue<InferenceResult>& queue, 
        std::atomic<bool>& isRunning,
        std::shared_ptr<MessageFormatter> formatter,
        int messages_per_second)
    : Publisher(std::make_shared<UDPTransport>(ip, port), queue, isRunning, formatter, messages_per_second) {
}