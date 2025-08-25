# BrightSign Object Detection Extension BSMP

**Example: Automated AI object detection extension for BrightSign Series 5 players using Rockchip NPU acceleration.**

This project provides a complete, automated build system to create BrightSign extensions that run object detection on the NPU at ~30 FPS with selective class detection and configurable confidence thresholds.

## Use It vs. Build It

If you just want to use this BSMP extension but don't want to build it, you can just download it.

* [cobra-standalone-npu_obj-0.1.0-alpha.bsfw](https://github.com/brightsign/brightsign-npu-object-extension/releases/download/v0.1.0-alpha/cobra-standalone-npu_obj-0.1.0-alpha.bsfw)

It can be installed just like any other BrightSign firmware upgrade:  copy it to an SD card and boot the player with that card inserted.

## Pracical Real-World Example

A [simple BrightAuthor:connected presentation](https://github.com/brightsign/simple-object-detection-presentation) demonstrating the object detection BSMP is available for demonstration purposes.

## Release Status

This is an **ALPHA** quality release, intended mostly for educational purposes. This model is not tuned for optimum performance and has had only standard testing.  **NOT RECOMMENDED FOR PRODUCTION USE**.

# Building the BSMP Extension

## 🚀 Quick Start (Complete Automated Workflow)

__Total Time__: 60-90 minutes | __Prerequisites__: Docker, git, x86_64 Linux host

> ⏱️ **Time Breakdown**: Most time is spent in the OpenEmbedded SDK build (30-45 min). The process is fully automated but requires patience for the BitBake compilation.

```bash
# 1. Clone and setup environment (5-10 minutes)
git clone <repository-url>
cd brightsign-npu-object-extension
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

In a typical development workflow, steps 1 - 4 (setup, model compilation, build and install the sdk) will need to only be done once.  Building the apps and packaging them will likely be repeated as the developer changes the app code.

## 🤖 Automated Build Script

```bash
# Run all steps automatically (no prompts)
./scripts/runall.sh --auto

# Run all steps interactively (prompts between steps)
./scripts/runall.sh

# Skip already completed steps
./scripts/runall.sh --skip-setup --skip-models

# Run partial workflow (e.g., start from app building)
./scripts/runall.sh --from-step 5

# Get help with all options
./scripts/runall.sh --help
```

**Key Features:**

- **Progress tracking** with time estimates
- **Skip options** for already completed steps
- **Partial execution** (start/stop at specific steps)
- **Verbose mode** for detailed output
- **Auto mode** for CI/CD pipelines
- **Error handling** with clear failure messages

**Time estimates:** Setup (5-10 min) → Models (3-5 min) → SDK Build (30-45 min) → SDK Install (1 min) → Apps (3-8 min) → Package (1 min)

**✅ Success**: You now have production-ready extension packages:

- `objdet-dev-<timestamp>.zip` (development/testing)
- `objdet-ext-<timestamp>.zip` (production deployment)

**🎯 Deploy to Player**:

1. Transfer extension package to BrightSign player via DWS
2. Install: `bash ./ext_objdet_install-lvm.sh && reboot`
3. Extension auto-starts with USB camera detection

## 🚀 Build using Github Action

__Total Time__: approx. 10 minutes

1. Select __Actions__ -> __Build Extension__
2. __Run Workflow__
3. Select __Branch__ which should be used to build it from
4. Select __Brightsign OS Version__ - default 9.1.52
5. If models have been updated - check __Build models__ (don't needed in most cases)
6. If SDK has been changed - check __Build SDK__ (don't needed in most cases - and it'll take about 2-3h to build)
7. __Run workflow__

When Action has been completed - packages can be found in https://builds-npu.brightsign.io/browse/brightsign-npu-object-detect

## 📋 Requirements & Prerequisites

### Hardware Requirements

| Component | Requirement |
|-----------|-------------|
| __Development Host__ | x86_64 architecture (Intel/AMD) |
| __BrightSign Player__ | Series 5 (XT-5, LS-5) or Firebird dev board |
| __Camera__ | USB webcam (tested: Logitech C270, Thustar) |
| __Storage__ | 25GB+ free space for builds |

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

__Important__: Apple Silicon Macs are not supported. Use x86_64 Linux or Windows with WSL2.

__SDK and Models__ built for this project can be found in here - https://builds-npu.brightsign.io/browse/brightsign-npu-object-detect

## ⚙️ Configuration & Customization

The extension is highly configurable via BrightSign registry keys:

### Core Settings

```bash
# Auto-start control
registry write extension bsext-obj-disable-auto-start true

# Camera device override
registry write extension bsext-obj-video-device /dev/video1

# Custom model path
registry write extension bsext-obj-model-path /path/to/custom.rknn
```

### AI Configuration

```bash
# Selective class detection (only show/count specific objects)
registry write extension bsext-obj-classes person,car,dog

# Confidence threshold (0.0-1.0, default: 0.3)
registry write extension bsext-obj-confidence-threshold 0.6
```

### COCO Classes Reference

The extension supports all 80 COCO classes. Common examples:

- **People & Animals**: `person`, `cat`, `dog`, `horse`, `elephant`
- **Vehicles**: `car`, `truck`, `bus`, `motorcycle`, `bicycle`
- __Indoor Objects__: `chair`, `couch`, `tv`, `laptop`, `cell_phone`

__Note__: Use underscores instead of spaces (e.g., `cell_phone`, not `cell phone`)

### Extension Behavior

- **Visual Output**: `/tmp/output.jpg` (decorated image with bounding boxes)
- **Data Output**: `/tmp/results.json` (complete detection results)
- **UDP Streaming**: Port 5002 (JSON), Port 5000 (BrightScript format)
- **Performance**: ~30 FPS continuous inference on NPU

## 📄 Extension Versioning & Manifest

### Version Information

- **Current Version**: 1.2.0
- **Minimum OS**: BrightSign OS 9.0.0+
- **Target OS**: BrightSign OS 9.1.0
- **License**: Apache 2.0

### For Developers

To customize version information for your own extensions:

```bash
# Copy the template
cp manifest-config.template.json manifest-config.json

# Edit your extension details
# - Version number
# - Description
# - Author information
# - Compatibility requirements

# Package automatically generates manifest.json
./package
```

### Manifest Features

- **Automated Compatibility Checking**: Validates OS and hardware compatibility at runtime
- **Version Tracking**: Semantic versioning with build metadata
- **Registry Configuration**: Declares available user settings
- **Hardware Requirements**: Specifies memory, storage, and capability needs

📖 **See [Manifest Guide](docs/manifest-guide.md) for complete documentation**

## 🚀 Phase 3: Advanced Extension Management

### Update Management & Orchestration

Comprehensive update workflow with policy enforcement, version validation, and automatic configuration preservation:

```bash
# Update extension with full validation
./sh/update-extension.sh objdet-ext-20250201-123456.zip

# Test update compatibility without executing
./sh/update-extension.sh --dry-run objdet-ext-20250201-123456.zip

# Force update ignoring policy restrictions
./sh/update-extension.sh --force --verbose objdet-ext-20250201-123456.zip

# Update without configuration backup (not recommended)
./sh/update-extension.sh --no-backup objdet-ext-20250201-123456.zip
```

**Update Policies:**

- **`automatic`**: Allow automated updates (development environments)
- **`manual`**: Require explicit approval (production default)
- **`blocked`**: Prevent updates (deprecated/end-of-life versions)

### Atomic Rollback System

Safe rollback to previous versions with LVM-based atomic operations:

```bash
# Rollback to latest backup
./sh/rollback-extension.sh npu_obj

# List available backups
./sh/rollback-extension.sh --list-backups npu_obj

# Rollback to specific backup
./sh/rollback-extension.sh --backup backup_20250201_143022 npu_obj

# Test rollback capability without executing
./sh/rollback-extension.sh --dry-run npu_obj

# Force rollback even if policy doesn't support it
./sh/rollback-extension.sh --force npu_obj
```

### Configuration Backup & Restore

Built-in configuration management with automatic preservation:

```bash
# Using bsext_init for configuration management
./bsext_init backup                    # Auto-generated backup name
./bsext_init backup my_backup_name     # Custom backup name
./bsext_init restore                   # Restore from latest backup
./bsext_init restore my_backup_name    # Restore from specific backup
./bsext_init list-backups              # List available backups
```

**What gets backed up:**

- Registry configuration (user settings)
- User data files (`/tmp/objdet_output`, `/tmp/results.json`)
- Extension state and preferences
- Manifest metadata for reference

### Manifest Validation Tools

Comprehensive validation with detailed reporting:

```bash
# Basic manifest validation
./sh/validate-manifest.sh manifest.json

# Verbose validation with detailed output
./sh/validate-manifest.sh --verbose manifest.json

# Schema validation (if schema available)
./sh/validate-manifest.sh --schema schemas/extension-manifest-v1.json manifest.json

# Generate detailed validation report
./sh/validate-manifest.sh --report manifest.json > validation-report.txt

# Check formatting and suggest improvements
./sh/validate-manifest.sh --format manifest.json
```

**Validation Features:**

- JSON syntax and schema compliance
- Semantic validation of version formats
- Compatibility requirement checking
- Registry configuration validation
- Cross-reference validation
- Formatting and style suggestions

### Enhanced Installation Validation

Pre-installation checks with cross-compilation awareness:

**Host-side validation** (during packaging):

- ✅ Package structure and completeness
- ✅ Manifest schema compliance
- ✅ Size calculations vs declared requirements
- ✅ Cross-platform consistency

**Target-side validation** (during installation):

- ✅ Hardware compatibility (SOC, NPU, camera)
- ✅ OS version compatibility
- ✅ Storage space availability
- ✅ System dependency validation
- ✅ Runtime capability checking

### Development & Testing Workflows

Enhanced development experience with comprehensive tooling:

```bash
# Enhanced packaging with validation
./package --verify                     # Run full validation after packaging

# Validate manifest during development
./sh/validate-manifest.sh --verbose manifest-config.json

# Test update process in development
./sh/update-extension.sh --dry-run --verbose test-package.zip

# Create and test configuration backups
./bsext_init backup dev_test_backup
./bsext_init restore dev_test_backup
```

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
./package --soc RK3588
```

### Installation Methods

**Development Installation** (volatile, lost on reboot):

```bash
# On player
mkdir -p /usr/local/obj && cd /usr/local/obj
unzip /storage/sd/objdet-dev-*.zip
./bsext_init run  # Test in foreground
```

**Production Installation** (permanent):

```bash
# On player
cd /usr/local && unzip /storage/sd/objdet-ext-*.zip
bash ./ext_npu_obj_install-lvm.sh
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

__📋 See [OrangePI_Development.md](OrangePI_Development.md) for complete development guide__

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
./compile-models XT5           # YOLOX for XT-5 only
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
- __Wrong architecture__: Must use x86_64 host (not ARM/Apple Silicon)

**Getting Help**:

```bash
# Core build system
./setup --help                    # Setup and environment options
./compile-models --help           # Model compilation options
./build --help                    # SDK build options
./build-apps --help               # Application build options
./package --help                  # Packaging options

# Phase 3 management tools
./sh/update-extension.sh --help   # Update orchestration options
./sh/rollback-extension.sh --help # Rollback management options
./sh/validate-manifest.sh --help  # Manifest validation options
./bsext_init --help               # Extension control and configuration
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
3. Set registry key: `bsext-obj-model-path /path/to/custom.rknn`

### Multi-Platform Development

The extension automatically detects platform at runtime:

- **RK3588** (XT-5): Uses `RK3588/` subdirectory
- **RK3568** (LS-5): Uses `RK3568/` subdirectory
- **RK3576** (Firebird): Uses `RK3576/` subdirectory

### Performance Tuning

- **Confidence threshold**: Higher values reduce false positives
- **Class filtering**: Improves performance by reducing output processing
- **Model selection**: YOLOX (optimized for accuracy and performance)

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

### 📄 [Manifest Guide](docs/manifest-guide.md)

- Extension versioning system
- Compatibility declarations
- User configuration options

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
    -c "cd /zoo/examples/yolox/python && python convert.py ../model/yolox_s.onnx rk3588 i8 ../model/RK3588/yolox_s.rknn"
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

- ✅ **YOLOX** (nano, tiny, small, medium, large, xl) - YOLOX architecture
- ✅ **COCO 80-class** models (default)
- ✅ **Custom trained** models (if following YOLOX output formats)

### Licensing

This project is released under [Apache 2.0 License](LICENSE.txt). Models from Rockchip Model Zoo have their own licenses - see [model-licenses.md](model-licenses.md) for details.

---

**🎉 Ready to get started?** Run `./setup` and follow the Quick Start guide above!

For questions or issues, see the troubleshooting section or check the technical documentation.
