# Orange Pi Development Guide

This guide covers development and testing using Orange Pi boards (OPi) as an alternative development environment for the BrightSign YOLO Object Detection project.

## Overview

While not required for BrightSign deployment, Orange Pi boards can facilitate a more responsive build and debug process due to their full Linux distribution and native compiler. This approach allows for rapid prototyping and testing before final cross-compilation for BrightSign OS.

**Important**: Orange Pi cannot be used for model compilation (requires x86_64 architecture) but is excellent for application development and testing with pre-compiled models.

## Requirements

### Hardware
- Orange Pi 5 Plus (or similar ARM-based Orange Pi board)
- USB webcam (same as BrightSign requirements)
- Cables, monitors, etc.

### Software
Orange Pi boards typically run Debian (Armbian) images. Use the eMMC image for best performance.

```bash
sudo apt update 
sudo apt install -y \
    cmake \
    gdb \
    git \
    libboost-all-dev \
    libturbojpeg-dev \
    libjpeg-turbo8-dev \
    libjpeg-turbo-progs
```

## Development Workflow

### 1. Model Preparation

**IMPORTANT**: Models must be compiled on an x86_64 host machine first, as Orange Pi (ARM architecture) cannot run the RKNN toolkit compilation process.

#### On your x86_64 development machine:
```bash
# Follow the main README instructions to compile models
./setup
./compile-models
```

This creates compiled RKNN models in the following locations:
```
install/
├── RK3588/model/          # For Orange Pi 5/5B and BrightSign XT-5
│   ├── yolov8n.rknn      # YOLOv8 nano model
│   ├── yolox_s.rknn      # YOLOX small model
│   └── coco_80_labels_list.txt  # Class labels
├── RK3576/model/          # For Firebird dev boards
│   └── ...
└── RK3568/model/          # For BrightSign LS-5
    └── ...
```

#### Transfer to Orange Pi:
```bash
# Copy the entire project tree to Orange Pi
scp -r /path/to/cv-npu-yolo-object-detect orangepi:/home/user/

# Or copy just the compiled models if project already exists on Orange Pi
scp -r install/ orangepi:/home/user/cv-npu-yolo-object-detect/
```

**Model Location on Orange Pi**: After copying, compiled models will be available at:
- **Primary models**: `install/RK3588/model/` (compatible with Orange Pi 5 series)
- **YOLOv8**: `install/RK3588/model/yolov8n.rknn`
- **YOLOX**: `install/RK3588/model/yolox_s.rknn`
- **Labels**: `install/RK3588/model/coco_80_labels_list.txt`

### 2. Building on Orange Pi

All commands in this section are executed on the Orange Pi (via SSH or directly):

```bash
cd cv-npu-yolo-object-detect

# Clean any previous builds
rm -rf build

# Create build directory
mkdir -p build && cd build

# Configure for Orange Pi (native ARM build)
cmake .. -DTARGET_SOC="rk3588"

# Build the project
make

# Install to project directory
make install
```

### 3. Testing on Orange Pi

After building, you can test the application directly using the compiled models:

```bash
cd cv-npu-yolo-object-detect

# Test YOLOv8 with USB camera (V4L device mode)
./build/yolo_demo install/RK3588/model/yolov8n.rknn /dev/video0

# Test YOLOX with USB camera
./build/yolo_demo install/RK3588/model/yolox_s.rknn /dev/video0

# Test with image file (one-shot mode)
./build/yolo_demo install/RK3588/model/yolov8n.rknn /path/to/test_image.jpg

# Test with class filtering (only show specific objects)
./build/yolo_demo install/RK3588/model/yolov8n.rknn /dev/video0 --classes person,car,dog

# Test with confidence threshold (reduce false positives)
./build/yolo_demo install/RK3588/model/yolov8n.rknn /dev/video0 --confidence-threshold 0.5

# Test both class filtering and confidence threshold
./build/yolo_demo install/RK3588/model/yolox_s.rknn /dev/video0 --classes person,bicycle --confidence-threshold 0.6
```

**Available Models for Testing:**
- **YOLOv8**: `install/RK3588/model/yolov8n.rknn` (YOLO Simplified architecture)
- **YOLOX**: `install/RK3588/model/yolox_s.rknn` (YOLOX architecture)
- **Labels**: `install/RK3588/model/coco_80_labels_list.txt` (80 COCO classes)

**Model Verification:**
```bash
# Verify all required models are present
ls -la install/RK3588/model/
# Should show:
# yolov8n.rknn (several MB)
# yolox_s.rknn (several MB)  
# coco_80_labels_list.txt (small text file)

# Check model file integrity
file install/RK3588/model/*.rknn
# Should show: "data" (binary format)
```

### 4. Debug and Development

Orange Pi provides excellent debugging capabilities:

```bash
# Debug with GDB
gdb ./build/yolo_demo
(gdb) run install/RK3588/model/yolov8n.rknn /dev/video0

# Monitor system resources
htop

# Check camera devices
ls /dev/video*
v4l2-ctl --list-devices

# Monitor output files
watch -n 1 ls -la /tmp/output.jpg /tmp/results.json
```

## Platform Considerations

### Architecture Differences
- **Orange Pi**: ARM64 (aarch64) native compilation
- **BrightSign**: ARM64 (aarch64) cross-compilation from x86_64

### Development Benefits
- **Faster iteration**: Native compilation is faster than cross-compilation
- **Better debugging**: Full GDB support, system monitoring tools
- **Flexible testing**: Easy to test different models, parameters, and configurations
- **Complete Linux environment**: Access to all standard Linux development tools

### Limitations
- **Model compilation**: Cannot compile ONNX to RKNN (requires x86_64 + Docker)
- **Final deployment**: Must still cross-compile for BrightSign OS using the SDK
- **Hardware differences**: Some hardware-specific features may behave differently

## Model Compatibility

Orange Pi boards use the same Rockchip SoCs as BrightSign players:

| Orange Pi Board | SoC | Compatible BrightSign Platform |
|-----------------|-----|-------------------------------|
| Orange Pi 5/5B  | RK3588 | XT-5 Series (XT1145, XT2145) |
| Orange Pi 5 Plus | RK3588 | XT-5 Series (XT1145, XT2145) |

This means models compiled for RK3588 will work on both Orange Pi 5 series and BrightSign XT-5 players.

## Transferring to BrightSign

After development and testing on Orange Pi:

1. **Cross-compile for BrightSign OS** using the main README instructions:
   ```bash
   # On x86_64 host
   ./setup
   ./build --extract-sdk
   ./brightsign-x86_64-cobra-toolchain-9.1.52.sh -d ./sdk -y
   ./build-apps
   ```

2. **Package for deployment**:
   ```bash
   # Create deployment package
   cd install
   zip -r ../yolo-demo-$(date +%s).zip ./
   ```

3. **Deploy to BrightSign player** following the extension installation process.

## Development Best Practices

### Testing Strategy
1. **Rapid prototyping** on Orange Pi for algorithm changes
2. **Cross-compilation testing** for BrightSign compatibility
3. **Final validation** on actual BrightSign hardware

### Code Organization
- Keep platform-specific code clearly separated
- Use CMake variables for platform detection
- Test both native (Orange Pi) and cross-compiled (BrightSign) builds

### Performance Considerations
- Orange Pi performance may differ from BrightSign players
- Always validate performance on target BrightSign hardware
- Use Orange Pi for functional testing, BrightSign for performance validation

## Troubleshooting

### Common Issues

**Camera not detected**:
```bash
# Check available cameras
v4l2-ctl --list-devices

# Test camera functionality
ffplay /dev/video0
```

**Model loading failures**:
```bash
# Verify model files exist and are readable
ls -la install/RK3588/model/
file install/RK3588/model/yolov8n.rknn
file install/RK3588/model/yolox_s.rknn

# Check model file sizes (should be several MB each)
du -h install/RK3588/model/*.rknn

# Verify labels file exists
cat install/RK3588/model/coco_80_labels_list.txt | head -10
```

**Missing models on Orange Pi**:
```bash
# If models are missing, they need to be compiled on x86_64 first
# On x86_64 host:
./setup
./compile-models

# Then copy to Orange Pi:
scp -r install/ orangepi:/home/user/cv-npu-yolo-object-detect/
```

**Build errors**:
```bash
# Ensure all dependencies are installed
sudo apt install -y cmake libboost-all-dev libturbojpeg-dev

# Clean and rebuild
rm -rf build && mkdir build && cd build
cmake .. -DTARGET_SOC="rk3588"
make VERBOSE=1
```

### Performance Monitoring

Monitor Orange Pi performance during development:

```bash
# CPU and memory usage
htop

# GPU/NPU usage (if available)
cat /sys/class/devfreq/fdab0000.npu/cur_freq

# Temperature monitoring
cat /sys/class/thermal/thermal_zone*/temp
```

## Integration with Main Workflow

Orange Pi development integrates with the main BrightSign workflow:

```
┌─────────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│    x86_64 Host      │    │    Orange Pi     │    │ BrightSign      │
│                     │    │                  │    │ Player          │
│ • ./setup           │───▶│ • Native build   │───▶│ • Final deploy  │
│ • ./compile-models  │    │ • App develop    │    │ • Production    │
│ • ./build (SDK)     │    │ • Testing        │    │ • Validation    │
│ • ./build-apps      │    │ • Debug          │    │                 │
│   (cross-compile)   │    │                  │    │                 │
└─────────────────────┘    └──────────────────┘    └─────────────────┘
```

**Model Compilation Flow**:
```
x86_64 Host                    Orange Pi
┌─────────────────┐           ┌──────────────────┐
│ ./setup         │           │                  │
│ • Clone toolkit │           │                  │
│ • Download ONNX │           │                  │
│ • Build Docker  │           │                  │
│                 │           │                  │
│ ./compile-models│           │                  │
│ • ONNX → RKNN   │──────────▶│ Use compiled     │
│ • All platforms │   Copy    │ models in        │
│ • Save to       │   models  │ install/RK3588/  │
│   install/      │           │                  │
└─────────────────┘           └──────────────────┘
```

This approach provides the best of both worlds: rapid development iteration on Orange Pi and reliable deployment on BrightSign hardware.

## Related Documentation

- [Main README](README.md) - Complete project documentation
- [Software Architecture](docs/software-architecture.md) - System design details
- [Orange Pi Wiki](http://www.orangepi.org/orangepiwiki/index.php/Orange_Pi_5_Plus) - Hardware documentation