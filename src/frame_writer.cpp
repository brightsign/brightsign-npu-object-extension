#include "frame_writer.h"
#include "inference.h"
#include "utils.h"
#include <cstdio>
#include <cstring>

void DecoratedFrameWriter::writeFrame(cv::Mat& frame, const InferenceResult& result) {
    // Check if we have any valid detections (score > 0 and class_id >= 0) that are also selected
    int valid_detections = 0;
    for (int i = 0; i < result.detections.count; i++) {
        if (result.detections.results[i].prop > 0.0f && 
            result.detections.results[i].cls_id >= 0 &&
            isClassSelected(result.detections.results[i].cls_id, result.selected_classes)) {
            valid_detections++;
        }
    }
    
    // If suppress_empty is enabled and no valid detections, draw "none" text
    if (suppress_empty && valid_detections == 0) {
        std::string none_text = "none";
        cv::Scalar red_color(0, 0, 255);  // Red color in RGB
        
        // Get text size to center it
        int baseline = 0;
        cv::Size text_size = cv::getTextSize(none_text, cv::FONT_HERSHEY_SIMPLEX, 2.0, 3, &baseline);
        
        // Calculate center position
        cv::Point text_pos;
        text_pos.x = (frame.cols - text_size.width) / 2;
        text_pos.y = (frame.rows + text_size.height) / 2;
        
        // Draw "none" text in red
        cv::putText(frame, none_text, text_pos, cv::FONT_HERSHEY_SIMPLEX, 2.0, red_color, 3);
    } else {
        // Draw boxes on the image for detected objects
        for (int i = 0; i < result.detections.count; i++) {
            try {
                const auto& detection = result.detections.results[i];
                
                // Skip detections with invalid scores or class_ids
                if (detection.prop <= 0.0f || detection.cls_id < 0) {
                    printf("Skipping invalid detection: prop=%.2f, cls_id=%d\n", detection.prop, detection.cls_id);
                    continue;
                }
                
                // Skip detections that are not in the selected classes
                if (!isClassSelected(detection.cls_id, result.selected_classes)) {
                    printf("Skipping unselected class: cls_id=%d\n", detection.cls_id);
                    continue;
                }
                
                auto color = cv::Scalar(0, 255, 0);  // green for detected objects
                auto& box = detection.box;
                
                // Validate box coordinates
                if (box.left < 0 || box.top < 0 || box.right >= frame.cols || box.bottom >= frame.rows ||
                    box.left >= box.right || box.top >= box.bottom) {
                    printf("Warning: Invalid bounding box coordinates: left=%d, top=%d, right=%d, bottom=%d\n",
                           box.left, box.top, box.right, box.bottom);
                    continue;  // Skip this detection
                }
                
                printf("Drawing detection %d: prop=%.2f, cls_id=%d, box=[%d,%d,%d,%d]\n", 
                       i, detection.prop, detection.cls_id, box.left, box.top, box.right, box.bottom);
                
                // Draw bounding box
                cv::rectangle(frame, cv::Point(box.left, box.top), cv::Point(box.right, box.bottom), color, 2);
                
                // Validate name pointer before using
                const char* name_ptr = detection.name;
                std::string obj_name = (name_ptr && strlen(name_ptr) > 0 && strlen(name_ptr) < 100) ? name_ptr : "unknown";
                
                // Draw label with confidence score
                char text[256];
                snprintf(text, sizeof(text), "%.2f", detection.prop);
                std::string label_text = obj_name + ": " + text;
                
                // Make sure label is drawn inside the image
                int baseline = 0;
                cv::Size text_size = cv::getTextSize(label_text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 2, &baseline);
                int y_pos = std::max(box.top - 10, text_size.height);
                
                cv::putText(frame, label_text, cv::Point(box.left, y_pos), 
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
            } catch (const std::exception& e) {
                printf("Exception while drawing box %d: %s\n", i, e.what());
            }
        }
    }

    // Convert back to BGR for OpenCV image writing
    cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
    
    // Write processed image to temporary file then rename atomically
    // Preserve extension for OpenCV codec detection by inserting .tmp before extension
    size_t last_dot = output_path.find_last_of('.');
    std::string temp_path;
    if (last_dot != std::string::npos) {
        temp_path = output_path.substr(0, last_dot) + ".tmp" + output_path.substr(last_dot);
    } else {
        // No extension found, assume .jpg
        temp_path = output_path + ".tmp.jpg";
    }
    cv::imwrite(temp_path, frame);
    std::rename(temp_path.c_str(), output_path.c_str());
}