# BrightSign YOLO Object Detection Extension

**Automated AI object detection extension for BrightSign Series 5 players using Rockchip NPU acceleration.**

This project provides a complete, automated build system to create BrightSign extensions that run YOLO object detection (YOLOv8 and YOLOX) on the NPU at ~30 FPS with selective class detection and configurable confidence thresholds.

**NEW**: Now supports selective class detection - focus on specific object classes while preserving complete detection data!

## 🚀 Quick Start (Complete Automated Workflow)

**Total Time**: 60-90 minutes | **Prerequisites**: Docker, git, x86_64 Linux host

> ⏱️ **Time Breakdown**: Most time is spent in the OpenEmbedded SDK build (30-45 min). The process is fully automated but requires patience for the BitBake compilation.

```bash
# 1. Clone and setup environment (5-10 minutes)
git clone <repository-url>
cd cv-npu-yolo-object-detect
./setup

# 2. Compile ONNX models to RKNN format (3-5 minutes)
./compile-models

# 3. Build OpenEmbedded SDK (30-45 minutes)
./build --extract-sdk

# 4. Install the SDK (1 minute)
./brightsign-x86_64-cobra-toolchain-*.sh -d ./sdk -y

# 5. Build C++ applications for all platforms (3-8 minutes)
./build-apps

# 6. Package extension for deployment (1 minute)
./package
```

**✅ Success**: You now have production-ready extension packages:
- `yolo-dev-<timestamp>.zip` (development/testing)
- `yolo-ext-<timestamp>.zip` (production deployment)

**🎯 Deploy to Player**:
1. Transfer extension package to BrightSign player via DWS
2. Install: `bash ./ext_npu_yolo_install-lvm.sh && reboot`
3. Extension auto-starts with USB camera detection

## 📋 Requirements & Prerequisites

### Hardware Requirements
| Component | Requirement |
|-----------|-------------|
| **Development Host** | x86_64 architecture (Intel/AMD) |
| **BrightSign Player** | Series 5 (XT-5, LS-5) or Firebird dev board | 
| **Camera** | USB webcam (tested: Logitech C270, Thustar) |
| **Storage** | 25GB+ free space for builds |

### Supported Players
| Player | SOC | Platform Code | Status |
|--------|-----|---------------|---------|
| XT-5 (XT1145, XT2145) | RK3588 | XT5 | ✅ Production |
| LS-5 (LS445) | RK3568 | LS5 | ✅ Beta |
| Firebird | RK3576 | Firebird | 🧪 Development |

### Software Requirements
- **Docker** (for containerized builds)
- **Git** (for repository cloning)
- **25GB+ disk space** (for OpenEmbedded builds)

**Important**: Apple Silicon Macs are not supported. Use x86_64 Linux or Windows with WSL2.

## ⚙️ Configuration & Customization

The extension is highly configurable via BrightSign registry keys:

### Core Settings
```bash
# Auto-start control
registry write extension bsext-yolo-disable-auto-start true

# Camera device override
registry write extension bsext-yolo-video-device /dev/video1

# Custom model path
registry write extension bsext-yolo-model-path /path/to/custom.rknn
```

### AI Configuration
```bash
# Selective class detection (only show/count specific objects)
registry write extension bsext-yolo-classes person,car,dog

# Confidence threshold (0.0-1.0, default: 0.3)
registry write extension bsext-yolo-confidence-threshold 0.6
```

### COCO Classes Reference
The extension supports all 80 COCO classes. Common examples:
- **People & Animals**: `person`, `cat`, `dog`, `horse`, `elephant`
- **Vehicles**: `car`, `truck`, `bus`, `motorcycle`, `bicycle`
- **Indoor Objects**: `chair`, `couch`, `tv`, `laptop`, `cell_phone`

**Note**: Use underscores instead of spaces (e.g., `cell_phone`, not `cell phone`)

### Extension Behavior
- **Visual Output**: `/tmp/output.jpg` (decorated image with bounding boxes)
- **Data Output**: `/tmp/results.json` (complete detection results)
- **UDP Streaming**: Port 5002 (JSON), Port 5000 (BrightScript format)
- **Performance**: ~30 FPS continuous inference on NPU

## 📦 Production Deployment

### Package Options
```bash
# Create both development and production packages
./package

# Development package only (volatile installation)
./package --dev-only

# Production extension only (permanent installation)
./package --ext-only

# Package specific platform/model combinations
./package --soc RK3588 --model yolov8
```

### Installation Methods

**Development Installation** (volatile, lost on reboot):
```bash
# On player
mkdir -p /usr/local/yolo && cd /usr/local/yolo
unzip /storage/sd/yolo-dev-*.zip
./bsext_init run  # Test in foreground
```

**Production Installation** (permanent):
```bash
# On player
cd /usr/local && unzip /storage/sd/yolo-ext-*.zip
bash ./ext_npu_yolo_install-lvm.sh
reboot  # Extension auto-starts after reboot
```

### Validation & Testing
```bash
# Test specific platform build
./build-apps XT5 && ./package --soc RK3588 --dev-only

# Verify models compiled correctly
ls install/*/model/*.rknn

# Check extension functionality
./package --verify
```

## 🛠️ Development & Testing

### Rapid Development Workflow
For faster iteration during development, consider using Orange Pi boards:

**📋 See [OrangePI_Development.md](OrangePI_Development.md) for complete development guide**

Benefits:
- **Faster builds**: Native ARM compilation vs cross-compilation  
- **Better debugging**: Full GDB support and system monitoring
- **Same hardware**: Uses identical Rockchip SoCs as BrightSign players

### Build System Options
```bash
# Build specific platforms only
./build-apps XT5      # XT-5 players only
./build-apps LS5      # LS-5 players only

# Compile specific models only
./compile-models XT5 yolov8    # YOLOv8 for XT-5 only
./compile-models --clean       # Clean rebuild all models

# SDK build options
./build --help                 # See all build options
./build --clean brightsign-sdk # Clean SDK rebuild
```

### Troubleshooting

**Common Issues**:
- **Docker not running**: `systemctl start docker`
- **Permission denied**: Add user to docker group
- **Out of space**: Need 25GB+ for OpenEmbedded builds
- **Wrong architecture**: Must use x86_64 host (not ARM/Apple Silicon)

**Getting Help**:
```bash
./setup --help          # Setup and environment options
./compile-models --help # Model compilation options  
./build --help          # SDK build options
./build-apps --help     # Application build options
./package --help        # Packaging options
```

**Build Failures**:
```bash
# Clean and retry
./build-apps --clean
./compile-models --clean
./setup  # Re-run if Docker images corrupted
```

## 🎯 Advanced Usage

### Custom Models
Replace default models with your own ONNX models:
1. Place ONNX model in `toolkit/rknn_model_zoo/examples/custom/model/`
2. Run `./compile-models` to convert to RKNN format
3. Set registry key: `bsext-yolo-model-path /path/to/custom.rknn`

### Multi-Platform Development
The extension automatically detects platform at runtime:
- **RK3588** (XT-5): Uses `RK3588/` subdirectory
- **RK3568** (LS-5): Uses `RK3568/` subdirectory  
- **RK3576** (Firebird): Uses `RK3576/` subdirectory

### Performance Tuning
- **Confidence threshold**: Higher values reduce false positives
- **Class filtering**: Improves performance by reducing output processing
- **Model selection**: YOLOv8 (faster) vs YOLOX (more accurate)

## 📚 Technical Documentation

For in-depth technical information:

### 🏗️ [Software Architecture](docs/software-architecture.md)
- Component design and threading model
- Producer-Consumer pattern implementation
- Multi-platform support architecture

### 🔌 [Integration Guide](docs/integration-extension-points.md)
- Adding custom transport protocols
- Creating message formatters
- Performance testing extensions

### 🍊 [Orange Pi Development](OrangePI_Development.md)
- Rapid prototyping workflow
- Native development environment
- Testing and debugging guide

### ⚖️ [Design Principles](docs/design-principles-analysis.md)
- SOLID principles adherence
- Clean Code practices assessment
- Architecture design patterns

## 📖 Legacy Manual Build Process

<details>
<summary>Click to expand manual build instructions (for advanced users)</summary>

For users who need fine-grained control over the build process, the original manual steps are still available:

### Manual Model Compilation
```bash
# Setup toolkit manually
cd toolkit/rknn-toolkit2/docker
./build.sh

# Compile models manually
cd ../../../rknn_model_zoo
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolov8/python && python convert.py ../model/yolov8n.onnx rk3588 i8 ../model/RK3588/yolov8n.rknn"
```

### Manual SDK Building
```bash
# Download BrightSign OS sources
wget https://brightsignbiz.s3.amazonaws.com/firmware/opensource/9.1/9.1.52/brightsign-9.1.52-src-*.tar.gz

# Build SDK with BitBake
docker run -it --rm -v $(pwd)/brightsign-oe:/home/builder/bsoe bsoe-build
cd /home/builder/bsoe/build && MACHINE=cobra ./bsbb brightsign-sdk
```

### Manual Cross-Compilation
```bash
# Source SDK environment
source ./sdk/environment-setup-aarch64-oe-linux

# Build for specific platform
mkdir build_xt5 && cd build_xt5
cmake .. -DOECORE_TARGET_SYSROOT="${OECORE_TARGET_SYSROOT}" -DTARGET_SOC=rk3588
make && make install
```

</details>

## 🔗 Model Compatibility & Licensing

### Supported Models
- ✅ **YOLOv8** (nano, small, medium, large, xl) - YOLO Simplified architecture
- ✅ **YOLOX** (nano, tiny, small, medium, large, xl) - YOLOX architecture  
- ✅ **COCO 80-class** models (default)
- ✅ **Custom trained** models (if following YOLO/YOLOX output formats)

### Licensing
This project is released under [Apache 2.0 License](LICENSE.txt). Models from Rockchip Model Zoo have their own licenses - see [model-licenses.md](model-licenses.md) for details.

---

**🎉 Ready to get started?** Run `./setup` and follow the Quick Start guide above!

For questions or issues, see the troubleshooting section or check the technical documentation.