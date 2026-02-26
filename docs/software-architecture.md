# Software Design and Architecture

## Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Layered Architecture](#layered-architecture)
- [Component Design](#component-design)
- [Threading Model](#threading-model)
- [Data Flow Architecture](#data-flow-architecture)
- [Multi-Platform Support](#multi-platform-support)
- [Configuration Architecture](#configuration-architecture)

## Overview

The BrightSign Object Detection application is a computer vision system designed for real-time object detection on BrightSign Players (embedded ARM-based hardware). The architecture features a clear separation of concerns, modular design, and extensible patterns.

### Key Architectural Characteristics

- **Layered Architecture**: Clean separation between inference, transport, formatting, and configuration
- **Producer-Consumer Pattern**: Thread-safe pipeline for processing video frames
- **Strategy Pattern**: Pluggable transport and formatting mechanisms
- **Multi-Platform Support**: Runtime detection and adaptation for different SOCs
- **Configurable Pipeline**: Registry-based configuration system

## System Architecture

```mermaid
graph TB
    subgraph "Input Layer"
        V4L[V4L Video Device]
        FILE[Image Files]
    end
    
    subgraph "Processing Core"
        ML[ML Inference Thread]
        QUEUE[Thread-Safe Queue]
        NPU[Rockchip NPU]
    end
    
    subgraph "Output Layer"
        FW[Frame Writer]
        PUB[Publisher System]
    end
    
    subgraph "Transport Layer"
        UDP[UDP Transport]
        FILE_T[File Transport]
    end
    
    subgraph "Configuration"
        REG[Registry System]
        SOC[SOC Detection]
    end
    
    V4L --> ML
    FILE --> ML
    ML --> QUEUE
    ML --> NPU
    QUEUE --> PUB
    ML --> FW
    PUB --> UDP
    PUB --> FILE_T
    REG --> ML
    SOC --> ML
    
    classDef processing fill:#e1f5fe
    classDef transport fill:#f3e5f5
    classDef config fill:#fff3e0
    
    class ML,QUEUE,NPU processing
    class UDP,FILE_T transport
    class REG,SOC config
```

## Layered Architecture

The application follows a clean layered architecture pattern with well-defined boundaries:

```mermaid
graph TB
    subgraph "Presentation Layer"
        UI[Decorated Image Output]
        UDP_OUT[UDP Messages]
        FILE_OUT[JSON Files]
    end
    
    subgraph "Application Layer"
        PUB[Publishers]
        FMT[Message Formatters]
        FW[Frame Writers]
    end
    
    subgraph "Domain Layer"
        INF[Inference Engine]
        YOLO[Object Detection Model]
        PP[Post-Processing]
    end
    
    subgraph "Infrastructure Layer"
        TRANS[Transport Layer]
        QUEUE[Thread-Safe Queue]
        CV[OpenCV]
        RKNN[RKNN Runtime]
    end
    
    subgraph "Hardware Abstraction"
        NPU[Rockchip NPU]
        CAM[Camera Hardware]
        NET[Network Stack]
    end
    
    UI --> PUB
    UDP_OUT --> PUB
    FILE_OUT --> PUB
    PUB --> FMT
    PUB --> FW
    FMT --> INF
    FW --> INF
    INF --> YOLO
    INF --> PP
    YOLO --> RKNN
    PP --> RKNN
    PUB --> TRANS
    INF --> QUEUE
    TRANS --> NET
    CV --> CAM
    RKNN --> NPU
```

### Layer Responsibilities

1. **Presentation Layer**: Handles output formatting and delivery
2. **Application Layer**: Orchestrates business logic and data transformation
3. **Domain Layer**: Contains core AI/ML processing logic
4. **Infrastructure Layer**: Provides technical services and abstractions
5. **Hardware Abstraction**: Interfaces with physical hardware components

## Component Design

### Core Components

```mermaid
classDiagram
    class MLInferenceThread {
        -ThreadSafeQueue~InferenceResult~ resultQueue
        -atomic~bool~ running
        -rknn_app_context_t rknn_app_ctx
        -FrameWriter frameWriter
        +runInference(Mat img) InferenceResult
        +runSingleInference()
        +operator()()
    }
    
    class Publisher {
        -Transport transport
        -MessageFormatter formatter
        -ThreadSafeQueue~InferenceResult~ queue
        +operator()()
    }
    
    class Transport {
        <<interface>>
        +send(string data) bool
        +isConnected() bool
    }
    
    class MessageFormatter {
        <<interface>>
        +formatMessage(InferenceResult) string
    }
    
    class ThreadSafeQueue~T~ {
        -queue~T~ queue
        -mutex mutex
        -condition_variable cond
        +push(T value)
        +pop(T value) bool
        +signalShutdown()
    }
    
    class InferenceResult {
        +object_detect_result_list detections
        +chrono::time_point timestamp
        +vector~int~ selected_classes
        +unordered_map class_mapping
        +float confidence_threshold
    }
    
    MLInferenceThread --> ThreadSafeQueue
    MLInferenceThread --> InferenceResult
    Publisher --> Transport
    Publisher --> MessageFormatter
    Publisher --> ThreadSafeQueue
    ThreadSafeQueue --> InferenceResult
```

### Key Design Patterns

1. **Strategy Pattern**: Transport and MessageFormatter interfaces allow runtime selection
2. **Producer-Consumer**: MLInferenceThread produces, Publishers consume
3. **Template Method**: FrameWriter defines processing template
4. **Dependency Injection**: Publishers receive Transport and MessageFormatter dependencies

## Producer-Consumer Pattern Design Choice

The system implements a Producer-Consumer pattern rather than alternatives like Observer-Observable, and this choice is fundamental to the architecture's success in real-time computer vision processing.

### Pattern Comparison Analysis

```mermaid
graph TB
    subgraph "Producer-Consumer (Current)"
        P1[MLInferenceThread Producer]
        Q1[Thread-Safe Queue]
        C1[File Publisher Consumer]
        C2[UDP Publisher Consumer 1]
        C3[UDP Publisher Consumer 2]
        
        P1 --> Q1
        Q1 --> C1
        Q1 --> C2
        Q1 --> C3
    end
    
    subgraph "Observer-Observable (Alternative)"
        S1[MLInference Subject]
        O1[File Observer]
        O2[UDP Observer 1]
        O3[UDP Observer 2]
        
        S1 --> O1
        S1 --> O2
        S1 --> O3
    end
    
    style Q1 fill:#c8e6c9
    style S1 fill:#ffcdd2
```

### Why Producer-Consumer Was Chosen

| Aspect | Producer-Consumer ✅ | Observer-Observable ❌ |
|--------|---------------------|----------------------|
| **Threading Model** | Asynchronous, non-blocking producer | Synchronous callbacks block producer |
| **Backpressure Handling** | Built-in queue buffering | No flow control mechanism |
| **Failure Isolation** | Consumer failures don't affect producer | Observer exceptions can crash system |
| **Performance** | Lock-free operations where possible | Requires synchronized notification |
| **Scalability** | Easy to add consumers without code changes | Must modify subject to add observers |
| **Memory Management** | Controlled queue depth prevents memory spikes | Unbounded observer notifications |

### Real-Time Processing Benefits

#### 1. **Inference Pipeline Optimization**

```mermaid
flowchart LR
    CAPTURE[Frame Capture] --> INFERENCE[NPU Inference]
    INFERENCE --> QUEUE[Queue Result]
    QUEUE --> CONTINUE[Continue Processing]
    
    subgraph "Parallel Output Processing"
        FILE[File Writing]
        UDP1[UDP Port 5002]
        UDP2[UDP Port 5000]
    end
    
    QUEUE -.-> FILE
    QUEUE -.-> UDP1
    QUEUE -.-> UDP2
    
    style INFERENCE fill:#ffcdd2
    style QUEUE fill:#c8e6c9
```

The producer (inference thread) never blocks waiting for consumers, maintaining consistent ~30 FPS processing.

#### 2. **Backpressure Management**

```cpp
// ThreadSafeQueue handles backpressure automatically
template<typename T>
void ThreadSafeQueue<T>::push(T value) {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.size() >= max_depth) {
        queue.pop();  // Drop oldest result to maintain real-time performance
    }
    queue.push(std::move(value));
    cond.notify_one();
}
```

When consumers can't keep up, the queue drops old results rather than blocking the inference pipeline.

#### 3. **Failure Isolation**

- **Producer-Consumer**: If UDP publisher fails, file publisher continues working
- **Observer-Observable**: If one observer throws exception, it could crash the entire notification system

### Embedded System Advantages

#### Memory Efficiency

- **Fixed Queue Size**: Prevents memory bloat on embedded systems
- **RAII Management**: Automatic cleanup prevents memory leaks
- **Copy Avoidance**: Move semantics reduce memory allocation overhead

#### CPU Utilization

- **Thread Affinity**: Each consumer can run on dedicated cores
- **Lock Contention**: Minimized through careful queue design
- **Cache Locality**: Sequential queue operations improve cache performance

### Performance Characteristics

Producer-Consumer maintains better frame timing by decoupling output processing from inference.

### Architectural Trade-offs

#### Benefits ✅

- **Decoupling**: Producer and consumers are completely independent
- **Scalability**: Easy to add new output formats without affecting inference
- **Performance**: Non-blocking producer maintains real-time constraints
- **Reliability**: Consumer failures don't affect core processing
- **Memory Control**: Bounded queue prevents memory exhaustion

#### Considerations ⚠️

- **Latency**: Small additional latency due to queue buffering
- **Memory Usage**: Queue requires additional memory allocation
- **Complexity**: More complex than direct method calls

#### Optimal for Computer Vision ✅

- **Frame Rate Consistency**: Critical for real-time video processing
- **Multiple Output Formats**: Common requirement in vision applications
- **Embedded Constraints**: Memory and CPU limitations require careful design
- **Fault Tolerance**: Mission-critical applications need failure isolation

The Producer-Consumer pattern choice demonstrates thoughtful architectural decision-making, prioritizing real-time performance and system reliability over simplicity. This pattern is particularly well-suited to the demanding requirements of embedded computer vision systems.

## Threading Model

The application uses a sophisticated multi-threaded architecture for optimal performance:

```mermaid
sequenceDiagram
    participant Main as Main Thread
    participant Inf as Inference Thread
    participant Queue as Thread-Safe Queue
    participant FPub as File Publisher
    participant UPub1 as UDP Publisher 1
    participant UPub2 as UDP Publisher 2
    participant FW as Frame Writer
    
    Main->>+Inf: Start inference
    Main->>+FPub: Start file publishing
    Main->>+UPub1: Start UDP publishing (5002)
    Main->>+UPub2: Start UDP publishing (5000)
    
    loop Video Processing
        Inf->>Inf: Capture frame
        Inf->>Inf: Run NPU inference
        Inf->>Queue: Push InferenceResult
        Inf->>FW: Write decorated frame
        
        Queue->>FPub: Pop result (1 Hz)
        Queue->>UPub1: Pop result (1 Hz)
        Queue->>UPub2: Pop result (1 Hz)
        
        FPub->>FPub: Format & write JSON
        UPub1->>UPub1: Format & send UDP
        UPub2->>UPub2: Format & send UDP
    end
    
    Main->>Inf: Signal shutdown
    Main->>Queue: Signal shutdown
    Inf-->>-Main: Thread complete
    FPub-->>-Main: Thread complete
    UPub1-->>-Main: Thread complete
    UPub2-->>-Main: Thread complete
```

### Thread Safety Mechanisms

1. **Atomic Variables**: Shared `running` flag for coordinated shutdown
2. **Thread-Safe Queue**: Mutex and condition variables protect shared data
3. **RAII Pattern**: Automatic resource cleanup in destructors
4. **Lock-Free Operations**: Minimal contention for high performance

## Data Flow Architecture

```mermaid
flowchart TD
    START([Application Start]) --> INIT[Initialize Components]
    INIT --> SOC_DETECT{Detect SOC Type}
    
    SOC_DETECT --> RK3588[RK3588 Config]
    SOC_DETECT --> RK3576[RK3576 Config]
    SOC_DETECT --> RK3568[RK3568 Config]
    
    RK3588 --> MODEL_LOAD[Load Object Detection Model]
    RK3576 --> MODEL_LOAD
    RK3568 --> MODEL_LOAD
    
    MODEL_LOAD --> INPUT_TYPE{Input Type?}
    
    INPUT_TYPE --> VIDEO[V4L Video Device]
    INPUT_TYPE --> IMAGE[Single Image File]
    
    VIDEO --> CONTINUOUS[Continuous Mode]
    IMAGE --> SINGLE[Single-Shot Mode]
    
    CONTINUOUS --> CAPTURE[Capture Frame]
    SINGLE --> PROCESS[Process Image]
    
    CAPTURE --> PROCESS
    PROCESS --> NPU[NPU Inference]
    NPU --> POST[Post-Processing]
    POST --> FILTER[Class Filtering]
    FILTER --> QUEUE_PUSH[Push to Queue]
    
    QUEUE_PUSH --> FRAME_WRITE[Write Decorated Frame]
    QUEUE_PUSH --> PUB_FILE[File Publisher]
    QUEUE_PUSH --> PUB_UDP1[UDP Publisher 5002]
    QUEUE_PUSH --> PUB_UDP2[UDP Publisher 5000]
    
    FRAME_WRITE --> OUTPUT_IMG["/tmp/output.jpg"]
    PUB_FILE --> OUTPUT_JSON["/tmp/results.json"]
    PUB_UDP1 --> UDP_JSON[JSON over UDP]
    PUB_UDP2 --> UDP_BS[BrightScript over UDP]
    
    CONTINUOUS --> CAPTURE
    SINGLE --> END([Application End])
    
    style NPU fill:#ffcdd2
    style QUEUE_PUSH fill:#c8e6c9
    style OUTPUT_IMG fill:#fff3e0
    style OUTPUT_JSON fill:#fff3e0
```

## Multi-Platform Support

The architecture includes sophisticated runtime platform detection and adaptation:

```mermaid
graph TD
    START[Application Start] --> READ_DT[Read Device Tree]
    READ_DT --> PARSE[Parse Compatible String]
    
    PARSE --> RK3588_CHECK{Contains 'rockchip,rk3588'?}
    PARSE --> RK3576_CHECK{Contains 'rockchip,rk3576'?}
    PARSE --> RK3568_CHECK{Contains 'rockchip,rk3568'?}
    
    RK3588_CHECK -->|Yes| RK3588_CONFIG[Configure for RK3588]
    RK3576_CHECK -->|Yes| RK3576_CONFIG[Configure for RK3576]
    RK3568_CHECK -->|Yes| RK3568_CONFIG[Configure for RK3568]
    
    RK3588_CONFIG --> SET_VIDEO1[Use /dev/video1]
    RK3576_CONFIG --> SET_VIDEO0[Use /dev/video0]
    RK3568_CONFIG --> SET_VIDEO0
    
    SET_VIDEO1 --> LOAD_MODEL[Load Platform Model]
    SET_VIDEO0 --> LOAD_MODEL
    
    LOAD_MODEL --> CHECK_REGISTRY[Check Registry Overrides]
    CHECK_REGISTRY --> FINAL_CONFIG[Final Configuration]
    
    style RK3588_CONFIG fill:#e3f2fd
    style RK3576_CONFIG fill:#f3e5f5
    style RK3568_CONFIG fill:#e8f5e8
```

### Platform-Specific Adaptations

| Platform | SOC | Video Device | Model Path | Optimization |
|----------|-----|--------------|------------|--------------|
| XT-5 | RK3588 | /dev/video1 | RK3588/yolox_s.rknn | High Performance |
| Firebird | RK3576 | /dev/video0 | RK3576/yolox_s.rknn | Balanced |
| LS-5 | RK3568 | /dev/video0 | RK3568/yolox_s.rknn | Power Efficient |

## Configuration Architecture

The system uses a hierarchical configuration approach:

```mermaid
graph TD
    subgraph "Configuration Sources"
        DEFAULT[Default Values]
        REGISTRY[Registry Settings]
        CMDLINE[Command Line Args]
        RUNTIME[Runtime Detection]
    end
    
    subgraph "Configuration Categories"
        DEVICE[Device Settings]
        MODEL[Model Settings]
        DETECTION[Detection Settings]
        OUTPUT[Output Settings]
    end
    
    subgraph "Configuration Keys"
        VIDEO_DEV[bsext-obj-video-device]
        MODEL_PATH[bsext-obj-model-path]
        CLASSES[bsext-obj-classes]
        CONFIDENCE[bsext-obj-confidence-threshold]
        AUTO_START[bsext-obj-disable-auto-start]
    end
    
    DEFAULT --> DEVICE
    REGISTRY --> DEVICE
    CMDLINE --> DEVICE
    RUNTIME --> DEVICE
    
    DEFAULT --> MODEL
    REGISTRY --> MODEL
    CMDLINE --> MODEL
    
    DEFAULT --> DETECTION
    REGISTRY --> DETECTION
    CMDLINE --> DETECTION
    
    DEFAULT --> OUTPUT
    REGISTRY --> OUTPUT
    CMDLINE --> OUTPUT
    
    DEVICE --> VIDEO_DEV
    MODEL --> MODEL_PATH
    DETECTION --> CLASSES
    DETECTION --> CONFIDENCE
    OUTPUT --> AUTO_START
    
    style REGISTRY fill:#fff3e0
    style CMDLINE fill:#e8f5e8
    style RUNTIME fill:#f3e5f5
```

### Configuration Priority Order

1. **Command Line Arguments** (Highest priority)
2. **Registry Settings**
3. **Runtime Detection**
4. **Default Values** (Lowest priority)

## Architectural Benefits

### Strengths

- **Modularity**: Clear component boundaries enable independent development
- **Extensibility**: Strategy patterns allow easy addition of new features
- **Performance**: Multi-threaded design maximizes hardware utilization
- **Reliability**: Thread-safe operations and proper error handling
- **Maintainability**: Clean abstractions and separation of concerns

### Design Quality Metrics

- **Coupling**: Low coupling between components through interfaces
- **Cohesion**: High cohesion within individual components
- **Testability**: Dependency injection enables comprehensive testing
- **Scalability**: Thread pool patterns support performance scaling
- **Configurability**: Registry system enables runtime customization

This architecture provides a solid foundation for a production-ready computer vision system while maintaining flexibility for future enhancements and platform support.