#ifndef FRAME_WRITER_H
#define FRAME_WRITER_H

#include <string>
#include <opencv2/opencv.hpp>
#include "yolox.h"

// Forward declaration for InferenceResult
struct InferenceResult;

// Abstract interface for writing processed frames
class FrameWriter {
public:
    virtual ~FrameWriter() = default;
    virtual void writeFrame(cv::Mat& frame, const InferenceResult& result) = 0;
};

// Concrete implementation that decorates frames with bounding boxes and writes to file
class DecoratedFrameWriter : public FrameWriter {
private:
    std::string output_path;
    bool suppress_empty;
    
public:
    explicit DecoratedFrameWriter(const std::string& path, bool suppress_empty = false) 
        : output_path(path), suppress_empty(suppress_empty) {}
    void writeFrame(cv::Mat& frame, const InferenceResult& result) override;
};

#endif // FRAME_WRITER_H