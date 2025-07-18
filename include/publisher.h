#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>

#include "inference.h"
#include "transport.h"

using json = nlohmann::json;

// Abstract message formatter interface
class MessageFormatter {
public:
    virtual ~MessageFormatter() = default;
    virtual std::string formatMessage(const InferenceResult& result) = 0;
};

// Abstract base class for formatters with optional class name mapping
class MappedMessageFormatter : public MessageFormatter {
protected:
    std::unordered_map<std::string, std::string> class_mapping;
    
    // Extract only detections that match selected classes
    std::vector<object_detect_result> extractSelectedClasses(const InferenceResult& result) const;
    
    // Apply class name mapping if configured
    std::string mapClassName(const std::string& original_name) const;
    
public:
    // Constructor with optional class name mapping
    MappedMessageFormatter(const std::unordered_map<std::string, std::string>& mapping = {})
        : class_mapping(mapping) {}
    
    virtual ~MappedMessageFormatter() = default;
};

// Concrete implementation of MessageFormatter for JSON format
class JsonMessageFormatter : public MessageFormatter {
private:
    bool suppress_empty;
    
public:
    explicit JsonMessageFormatter(bool suppress_empty = false) : suppress_empty(suppress_empty) {}
    std::string formatMessage(const InferenceResult& result) override;
};

// Concrete implementation of MessageFormatter for BrightScript variable format
//  e.g. "faces_attending:0!!faces_in_frame_total:0!!timestamp:1746732409"
class BSVariableMessageFormatter : public MessageFormatter {
public:
    std::string formatMessage(const InferenceResult& result) override;
};

// Concrete implementation for faces JSON format (UDP port 5002)
// Maps people count to faces_* properties using generic mapping system
class FacesJsonMessageFormatter : public MappedMessageFormatter {
public:
    FacesJsonMessageFormatter();
    std::string formatMessage(const InferenceResult& result) override;
};

// Concrete implementation for faces BrightScript format (UDP port 5000)  
// Maps people count to faces_* properties in BrightScript format using generic mapping system
class FacesBSMessageFormatter : public MappedMessageFormatter {
public:
    FacesBSMessageFormatter();
    std::string formatMessage(const InferenceResult& result) override;
};

// Concrete implementation for selective JSON format
// Extracts and outputs counts only for selected classes as JSON object
class SelectiveJsonMessageFormatter : public MappedMessageFormatter {
public:
    SelectiveJsonMessageFormatter(const std::unordered_map<std::string, std::string>& mapping = {})
        : MappedMessageFormatter(mapping) {}
    std::string formatMessage(const InferenceResult& result) override;
};

// Concrete implementation for selective BrightScript format
// Extracts and outputs counts only for selected classes in BrightScript format
class SelectiveBSMessageFormatter : public MappedMessageFormatter {
public:
    SelectiveBSMessageFormatter(const std::unordered_map<std::string, std::string>& mapping = {})
        : MappedMessageFormatter(mapping) {}
    std::string formatMessage(const InferenceResult& result) override;
};

// Concrete implementation for full JSON format
// Outputs all detection details including bounding boxes, confidence scores, etc.
class FullJsonMessageFormatter : public MessageFormatter {
private:
    bool suppress_empty;
    
public:
    explicit FullJsonMessageFormatter(bool suppress_empty = false) : suppress_empty(suppress_empty) {}
    std::string formatMessage(const InferenceResult& result) override;
};


// Generic publisher class using transport injection
class Publisher {
public:
    Publisher(
        std::shared_ptr<Transport> transport,
        ThreadSafeQueue<InferenceResult>& queue,
        std::atomic<bool>& isRunning,
        std::shared_ptr<MessageFormatter> formatter,
        int messages_per_second = 1);
    
    ~Publisher() = default;
    
    void operator()();

private:
    std::shared_ptr<Transport> transport;
    ThreadSafeQueue<InferenceResult>& resultQueue;
    std::atomic<bool>& running;
    int target_mps;
    std::shared_ptr<MessageFormatter> formatter;
};

// Backward compatibility: UDPPublisher using transport injection
class UDPPublisher : public Publisher {
public:
    UDPPublisher(
        const std::string& ip,
        const int port,
        ThreadSafeQueue<InferenceResult>& queue,
        std::atomic<bool>& isRunning,
        std::shared_ptr<MessageFormatter> formatter,
        int messages_per_second = 1);
};