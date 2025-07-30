# BrightSign YOLO Object Detection BSMP

**NEW**: Now supports selective class detection - focus on specific object classes while preserving complete detection data!

This is an example BrightSign Model Package (BSMP) that implements Object Detection using YOLO frameworks (YOLO Simplified and YOLOX) on the BrightSign player NPU. This can be used as a template for development of other BSMP by partners and third-parties.

BSMP are delivered as an BrightSign OS (BOS) "extension." Extensions are delivered as firmware update files that are installed on a reboot. These are basically Linux squashfs file systems that extend the firmware to include the BSMP. You can learn more about extensions in our [Extension Template Repository](https://github.com/brightsign/extension-template).

## Table of Contents

- [Selective Class Detection Feature](#selective-class-detection-feature)
- [Supported Players](#supported-players)
- [Supported YOLO Models](#supported-yolo-models)
- [Extension Control](#extension-control)
- [Project Overview & Requirements](#project-overview--requirements)
- [Architecture & Design Documentation](#architecture--design-documentation)

## Supported Players

| Player | Platform | SOC | Minimum OS Version |
| --- | --- | --- | --- |
| XT-5: XT1145, XT2145 | Series 5 | RK3588 | [9.1.52](https://brightsignbiz.s3.amazonaws.com/firmware/xd5/9.1/9.1.52/brightsign-xd5-update-9.1.52.zip) |
| _Firebird_ | Development | RK3576 | [BETA-9.1.52](https://bsnbuilds.s3.us-east-1.amazonaws.com/firmware/brightsign-demos/9.1.52-BETA/BETA-cobra-9.1.52-update.bsfw) |
| LS-5: LS445 | Series 5 | RK3568 | [BETA-9.1.52](https://bsnbuilds.s3.us-east-1.amazonaws.com/firmware/brightsign-demos/9.1.52-BETA/BETA-cobra-9.1.52-update.bsfw) |

**All supported platforms** can be built using the instructions in this guide. The extension automatically detects the target platform at runtime.

## Supported Cameras

In general, any camera supported by Linux *should* work.  We've had great luck with Logitech cameras, especially the C270.

## Decorated Camera Output

Every frame of video captured is processed through the model. By default, every detected object has a bounding box drawn around it. With the selective class detection feature, you can choose to display only specific object classes while still preserving complete detection data. The decorated image is written to `/tmp/output.jpg` on the RAM disk to avoid impacting storage life.

A global confidence threshold is applied to all object detections.  For objects above this threshold, a _green_ bounding box is drawn whereas a _gray_ box is drawn for objects below the threshold.  See below for information about adjusting the threshold.

## Supported YOLO Models

This extension supports two YOLO model types with automatic detection:

| Model Type | Description | Processing Path |
|------------|-------------|-----------------|
| __YOLO Simplified__ | Latest YOLO architecture with DFL (Distribution Focal Loss) | `YOLO_SIMPLIFIED` - Uses DFL-based post-processing with separated tensor outputs |
| __YOLOX__ | YOLO with unified tensor format | `YOLO_STANDARD` - Uses unified tensor processing with exponential coordinate transforms |

The system automatically detects the model type based on output tensor structure and applies the appropriate post-processing pipeline. No manual configuration is required.

### Model Compatibility

- ✅ **YOLO Simplified models (nano, small, medium, large, extra-large)** - Full support with DFL processing
- ✅ **YOLOX-Nano, YOLOX-Tiny, YOLOX-S, YOLOX-M, YOLOX-L, YOLOX-X** - Full support with unified tensor processing
- ✅ **COCO 80-class models** - Default class set supported
- ✅ **Custom trained models** - Compatible if following YOLO Simplified or YOLOX output formats

## Overview

This repository gives the steps and tools to:

1. Compile ONNX formatted models for use on Rockchip NPUs (RK3588, RK3576, RK3568) -- used in BrightSign Series 5 players and development boards like OrangePi 5.
2. Develop and test an AI Application to load and run the model on the Rockchip NPUs.
3. Build the AI Application for BrightSign OS across multiple platforms
4. Package the Application and model as a BrightSign Extension

This project supports both YOLO Simplified and YOLOX models from the [Rockchip Model Zoo](https://github.com/airockchip/rknn_model_zoo). The application code in this repo was adapted from the example code from the Rockchip Model Zoo. Please ensure that you are aware of the license that your chosen model is released under. More information on model licenses can be seen [here](./model-licenses.md).

## Application Overview

This project will create an installable BrightSign Extension that supports two operational modes:

### V4L Device Mode (Continuous Inference)

When provided with a Video for Linux device (e.g., `/dev/video0`), the extension:

1. Loads the compiled YOLO model (YOLO Simplified or YOLOX) into the Rockchip NPU
2. Continuously captures video frames from an attached USB webcam using OpenCV
3. Runs YOLO object detection on each frame at ~30 FPS
4. Continuously updates the decorated image to `/tmp/output.jpg` (optionally filtering visual output by selected classes)
5. Writes complete inference results to `/tmp/results.json` once per second (all detections preserved)
6. Publishes selective class detection data via UDP to localhost ports 5002 (JSON) and 5000 (BrightScript)

### One-Shot Image Mode (Single Inference)

When provided with an image file path (e.g., `/tmp/input.jpg`), the extension:

1. Loads the compiled YOLO model (YOLO Simplified or YOLOX) into the Rockchip NPU
2. Processes the single input image file
3. Runs YOLO object detection once on the provided image
4. Saves the decorated result image to `/tmp/output.jpg` (optionally filtering visual output by selected classes)
5. Writes the complete inference results to `/tmp/results.json` (all detections preserved)
6. Exits after processing

### Extension Control

This extension allows five optional registry keys to be set to customize behavior:

* Disable the auto-start of the extension -- this can be useful in debugging or other problems
* Set the `v4l` device filename to override the auto-discovered device
* Set a custom model path to use different YOLO models
* **NEW**: Select specific object classes for visual display and UDP output
* **NEW**: Set confidence threshold to reduce false positive detections

**Registry keys are organized in the `extension` section**

| Registry Key | Values | Effect |
| --- | --- | --- |
| `bsext-yolo-disable-auto-start` | `true` or `false` | when truthy, disables the extension from autostart (`bsext_init start` will simply return). The extension can still be manually run with `bsext_init run` |
| `bsext-yolo-video-device` | a valid v4l device file name like `/dev/video0` or `/dev/video1` | normally not needed, but may be useful to override for some unusual or test condition |
| `bsext-yolo-model-path` | full path to a `.rknn` model file | allows using custom YOLO models (YOLO Simplified or YOLOX) instead of the default model |
| `bsext-yolo-classes` | comma-separated list of COCO class names like `person,car,dog` | __filters__ visual output and UDP messages to only show selected classes. Complete detection data is still preserved in `/tmp/results.json` |
| `bsext-yolo-confidence-threshold` | float value between `0.0` and `1.0` (default: `0.3`) | sets the minimum confidence threshold for object detections. Higher values reduce false positives but may miss some valid detections |

[COCO Class Names](https://gist.github.com/AruniRC/7b3dadd004da04c80198557db5da4bda)

#### Working with the Registry

The Registry can be manipulated via the Diagnostic Web Server (DWS) or the command shell on the player.

To set a registry key, go to the Registry Tab on DWS, and type the set commands in the command box and hit submit.  You can also view the entire registry on that tab.

**NOTE** the player must restart before registry changes are effective.

```bash
registry write extension bsext-yolo-disable-auto-start True
```

To remove a registry key override and revert to default behavior:

```bash
registry delete extension bsext-yolo-disable-auto-start
registry delete extension bsext-yolo-video-device  
registry delete extension bsext-yolo-classes
registry delete extension bsext-yolo-model-path
registry delete extension bsext-yolo-confidence-threshold
```

**Note**: This is particularly useful when a registry override points to an invalid path (e.g., a model path from a different environment) and you want to restore the default behavior.

### Command Line Usage

When running the application directly (e.g., for testing or via `bsext_init run`), you can use command line arguments:

```bash
./yolo_demo <model_path> <source> [--suppress-empty] [--classes class1,class2,...] [--confidence-threshold value]
```

**Arguments:**

- `<model_path>`: Path to the RKNN model file
- `<source>`: Either a V4L device (e.g., `/dev/video0`) or image file path
- `--suppress-empty`: (Optional) Suppress output when no detections are made
- `--classes`: (Optional) Comma-separated list of class names to detect (e.g., `person,car,dog`)
- `--confidence-threshold`: (Optional) Set confidence threshold for detections (0.0-1.0, default: 0.3)

**Example:**

```bash
./yolo_demo model/yolox_s.rknn /dev/video0 --classes person,car --confidence-threshold 0.5
```

__Note:__ When using the extension via `bsext_init`, registry values take precedence over command line defaults.

### Supported COCO Classes

The extension supports all 80 COCO classes. Some common examples:

| Category | Classes |
|----------|---------|
| **People & Animals** | `person`, `cat`, `dog`, `horse`, `sheep`, `cow`, `elephant`, `bear`, `zebra`, `giraffe` |
| **Vehicles** | `car`, `truck`, `bus`, `motorcycle`, `bicycle`, `airplane`, `boat`, `train` |
| **Sports & Recreation** | `sports ball`, `kite`, `baseball bat`, `skateboard`, `surfboard`, `tennis racket` |
| **Indoor Objects** | `chair`, `couch`, `bed`, `dining table`, `tv`, `laptop`, `cell phone`, `book` |
| **Kitchen & Food** | `bottle`, `wine glass`, `cup`, `bowl`, `banana`, `apple`, `pizza`, `cake` |

For the complete list of 80 supported classes, see `model/coco_80_labels_list.txt`.

- **Complete Detection**: The model always processes all objects in the scene
- **Selective Display**: Only selected classes are shown in the decorated output image (`/tmp/output.jpg`)
- **Selective UDP**: Only selected classes are counted and sent via UDP to ports 5002 and 5000
- **Complete Results**: All detections are preserved in `/tmp/results.json` regardless of selection

### Error Handling

- **Invalid class names**: Warning logged, invalid classes ignored
- **Empty class list**: Treated as "all classes selected" (backward compatible)
- **Missing labels file**: Clear error message with file path

### Performance Impact

- **Minimal**: Only adds class ID checking during output processing
- **No inference impact**: Model processing speed unchanged
- **Memory efficient**: No additional runtime memory allocations

## Quick Start (New Build System)

**Time**: 30-60 minutes | **Prerequisites**: Docker, git, x86_64 host

```bash
# Setup build environment (5-10 minutes)
./setup

# Build for all platforms (15-30 minutes)
./build

# Build for specific platform
./build XT5    # For XT-5 players (RK3588)
./build LS5    # For LS-5 players (RK3568)
./build Firebird  # For Firebird dev boards (RK3576)
```

**Output**: Compiled binaries in `build_<platform>/yolo_demo` ready for deployment

The new build system provides:
- **Automated Setup**: Single command environment preparation
- **Cross-Platform Builds**: Docker-based compilation for all supported platforms
- **Clean Builds**: Reproducible builds with dependency management
- **Fast Incremental**: Cached builds for rapid development

## Project Overview & Requirements

This repository describes building the project in these major steps:

1. **Quick Setup**: Use `./setup` and `./build` for automated cross-compilation (recommended)
2. **Model Compilation**: Compile ONNX models to RKNN format for NPU acceleration
3. **Testing**: Deploy and test on BrightSign Series 5 players or Orange Pi development boards
4. **Extension Packaging**: Package as BrightSign extension for production deployment

**Build Methods:**
- **🚀 New**: Automated Docker-based build system (`./setup` && `./build`)
- **📖 Legacy**: Manual build instructions (see sections below for detailed steps)

### Build System Troubleshooting

**Common Issues:**

- **Docker not found**: Install Docker and ensure it's running
- **Permission denied**: Ensure Docker daemon is accessible to your user
- **Architecture error**: Build system requires x86_64 host (Intel/AMD, not Apple Silicon)
- **Disk space**: Requires ~25GB free space for Docker images and builds
- **Build failures**: Use `./build --clean` to force clean rebuild

**Getting Help:**
```bash
./setup --help    # Setup options
./build --help    # Build options
```
4. Packaging the application and model as a BrightSign Extension

__IMPORTANT: THE TOOLCHAIN REFERENCED BY THIS PROJECT REQUIRES A DEVELOPMENT HOST WITH x86_64 (aka AMD64) INSTRUCTION SET ARCHITECTURE.__ This means that many common dev hosts such as Macs with Apple Silicon or ARM-based Windows and Linux computers __WILL NOT WORK.__  That also includes the OrangePi5Plus (OPi) as it is ARM-based. The OPi ___can___ be used to develop the application with good effect, but the model compilation and final build for BrigthSign OS (BSOS) ___must___ be performed on an x86_64 host.

### Requirements

1. A Series 5 player running an experimental, debug build of BrightSign OS -- signed and unsigned versions
2. A development computer with x86_64 instruction architecture to compile the model and cross-compile the executables
3. The cross compile toolchain ___matching the BSOS version___ of the player
4. USB webcam

   - Tested to work with Logitech c270
   - also known to work with [Thustar document/web cam](https://www.amazon.com/gp/product/B09C255SW7/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
   - _should_ work with any UVC device

5. Cables, switches, monitors, etc to connect it all.

#### Software Requirements -- Development Host

* [Docker](https://docs.docker.com/engine/install/) - it should be possible to use podman or other, but these instructions assume Docker
* [git](https://git-scm.com/)
* [cmake](https://cmake.org/download/)

```bash
# consult Docker installation instructions

# for others, common package managers should work
# for Ubuntu, Debian, etc.
sudo apt-get update && sudo apt-get install -y \
    cmake git
    
```

## Step 0 - Setup

1. Install [Docker](https://docs.docker.com/engine/install/)
2. Clone this repository -- later instructions will assume to start from this directory unless otherwise noted.

```bash
#cd path/to/your/directory
git clone git@github.com:brightsign/brightsign-npu-yolox.git
cd brightsign-npu-yolox

export project_root=$(pwd)
# this environment variable is used in the following scripts to refer to the root of the project
```

3. Clone the supporting Rockchip repositories (this can take a while)

```sh
cd "${project_root:-.}"
mkdir -p toolkit && cd $_

git clone https://github.com/airockchip/rknn-toolkit2.git --depth 1 --branch v2.3.0
git clone https://github.com/airockchip/rknn_model_zoo.git --depth 1 --branch v2.3.0

cd -
```

4. Install the BSOS SDK

**Build a custom SDK from public source**

The platform SDK can be built from public sources. Browse OS releases from the [BrightSign Open Source](https://docs.brightsign.biz/releases/brightsign-open-source) page.  Set the environment variable in the next code block to the desired os release version.

```sh {"promptEnv":"never"}
# Download BrightSign OS and extract
cd "${project_root:-.}"

export BRIGHTSIGN_OS_MAJOR_VERION=9.1
export BRIGHTSIGN_OS_MINOR_VERION=52
export BRIGHTSIGN_OS_VERSION=${BRIGHTSIGN_OS_MAJOR_VERION}.${BRIGHTSIGN_OS_MINOR_VERION}

wget https://brightsignbiz.s3.amazonaws.com/firmware/opensource/${BRIGHTSIGN_OS_MAJOR_VERION}/${BRIGHTSIGN_OS_VERSION}/brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz
wget https://brightsignbiz.s3.amazonaws.com/firmware/opensource/${BRIGHTSIGN_OS_MAJOR_VERION}/${BRIGHTSIGN_OS_VERSION}/brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz

tar -xzf brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz
tar -xzf brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz

# Patch BrightSign OS with some special recipes for the SDK
# Apply custom recipes to BrightSign OS source
rsync -av bsoe-recipes/ brightsign-oe/ 

# Clean up disk space
# rm brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz
# rm brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz

```

___IMPORTANT___: Building an OpenEmbedded project can be very particular in terms of packages and setup. For that reason it __strongly recommended__ to use the [Docker build](https://github.com/brightsign/extension-template/blob/main/README.md#recommended-docker) approadh.

```sh {"terminalRows":"44"}
# Build the SDK in Docker -- RECOMMENDED
cd "${project_root:-.}"

wget https://raw.githubusercontent.com/brightsign/extension-template/refs/heads/main/Dockerfile
docker build --rm --build-arg USER_ID=$(id -u) --build-arg GROUP_ID=$(id -g) --ulimit memlock=-1:-1 -t bsoe-build .

mkdir -p srv
# the build process puts some output in srv

docker run -it --rm \
  -v $(pwd)/brightsign-oe:/home/builder/bsoe -v $(pwd)/srv:/srv \
  bsoe-build

```

Then in the docker container shell

```sh
cd /home/builder/bsoe/build

MACHINE=cobra ./bsbb brightsign-sdk
# This will build the entire system and may take up to several hours depending on the speed of your build system.
```

Exit the Docker shell with `Ctl-D`

**INSTALL INTO `./sdk`**

You can access the SDK from BrightSign.  The SDK is a shell script that will install the toolchain and supporting files in a directory of your choice.  This [link](https://brightsigninfo-my.sharepoint.com/:f:/r/personal/gherlein_brightsign_biz/Documents/BrightSign-NPU-Share-Quividi?csf=1&web=1&e=bgt7F7) is limited only to those with permissions to access the SDK.

```sh
cd "${project_root:-}"

# copy the SDK to the project root
cp brightsign-oe/build/tmp-glibc/deploy/sdk/brightsign-x86_64-cobra-toolchain-*.sh ./

# can safely remove the source if you want to save space
#rm -rf brightsign-oe
```

```sh
cd "${project_root:-.}"

./brightsign-x86_64-cobra-toolchain-*.sh  -d ./sdk -y
# installs the sdk to ./sdk
```

Patch the SDK to include the Rockchip binary libraries that are closed source

```sh
cd "${project_root:-.}"/sdk/sysroots/aarch64-oe-linux/usr/lib

wget https://github.com/airockchip/rknn-toolkit2/blob/v2.3.2/rknpu2/runtime/Linux/librknn_api/aarch64/librknnrt.so
```

### Unsecure the Player

* Enabling the Diagnostic Web Server (DWS) is recommended as it's a handy way to transfer files and check various things on the player. This can be done in BrightAuthor:connected when creating setup files for a new player.

0. Power off the player
1. __Enable serial control__ | Connect a serial cable from the player to your development host.  Configure your terminal program for 115200 bps, no parity, 8 data bits, 1 stop bit (n-8-1) and start the terminal program.  Hold the __`SVC`__ button while applying power. _Quick_, like a bunny, type Ctl-C in your serial terminal to get the boot menu -- you have 3 seconds to do this.  type

```bash
=> console on
=> reboot
```

2. __Reboot the player again__ using the __`RST`__ button or the _Reboot_ button from the __Control__ tab of DWS for the player.  Within the first 3 seconds after boot, again type Ctl-C in your serial terminal program to get the boot prompt and type:

```bash
=> setenv SECURE_CHECKS 0
=> env save
=> printenv
```

Verify that `SECURE_CHECKS` is set to 0. And type `reboot`.

To enable `Ctl-C` in the ssh session, set the debug registry key from the **Registry** tab on DWS.

```bash
registry write brightscript debug 1
```

On some newer OS versions, the tool `disable_security_checks` is availabe that will handle the above for you.

**The player is now unsecured.**

## Step 1 - Compile ONNX Models for the Rockchip NPU

**This step needs only be peformed once or when the model itself changes**

To run common models on the Rockchip NPU, the models must converted, compiled, lowered on to the operational primitives supported by the NPU from the abstract operations of the model frameworkd (e.g TesnsorFlow or PyTorch). Rockchip supplies a model converter/compiler/quantizer, written in Python with lots of package dependencies. To simplify and stabilize the process a Dockerfile is provided in the `rknn-toolkit2` project.

__REQUIRES AN x86_64 INSTRUCTION ARCHITECTURE MACHINE OR VIRTUAL MACHINE__

For portability and repeatability, a Docker container is used to compile the models.

This Docker image needs only be built once and can be reused across models

```sh
cd "${project_root:-.}"/toolkit/rknn-toolkit2/rknn-toolkit2/docker/docker_file/ubuntu_20_04_cp38
docker build --rm -t rknn_tk2 -f Dockerfile_ubuntu_20_04_for_cp38 .
```

Download the models (also only necesary one time, they will be stored in the filesystem)

```sh
cd "${project_root:-.}"/toolkit/rknn_model_zoo/

# Download YOLO Simplified model
mkdir -p examples/yolov8/model/RK3588
pushd examples/yolov8/model
chmod +x ./download_model.sh && ./download_model.sh
popd

# Download YOLOX model  
mkdir -p examples/yolox/model/RK3588
pushd examples/yolox/model
chmod +x ./download_model.sh && ./download_model.sh
popd
```

Compile the models. Note the options for various SoCs.

```sh
cd "${project_root:-.}"/toolkit/rknn_model_zoo/

# Compile for RK3588 -- XT-5 players
mkdir -p examples/yolov8/model/RK3588 examples/yolox/model/RK3588

# YOLO Simplified (YOLOv8) for RK3588
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolov8/python && python convert.py ../model/yolov8n.onnx rk3588 i8 ../model/RK3588/yolov8n.rknn"

# YOLOX for RK3588
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolox/python && python convert.py ../model/yolox_s.onnx rk3588 i8 ../model/RK3588/yolox_s.rknn"

# Compile for RK3576 -- Firebird players
mkdir -p examples/yolov8/model/RK3576 examples/yolox/model/RK3576

# YOLO Simplified (YOLOv8) for RK3576
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolov8/python && python convert.py ../model/yolov8n.onnx rk3576 i8 ../model/RK3576/yolov8n.rknn"

# YOLOX for RK3576
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolox/python && python convert.py ../model/yolox_s.onnx rk3576 i8 ../model/RK3576/yolox_s.rknn"

# Compile for RK3568 -- LS-5 players
mkdir -p examples/yolov8/model/RK3568 examples/yolox/model/RK3568

# YOLO Simplified (YOLOv8) for RK3568
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolov8/python && python convert.py ../model/yolov8n.onnx rk3568 i8 ../model/RK3568/yolov8n.rknn"

# YOLOX for RK3568
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolox/python && python convert.py ../model/yolox_s.onnx rk3568 i8 ../model/RK3568/yolox_s.rknn"

# Copy models and labels to install directories for all platforms
for SOC in RK3588 RK3576 RK3568; do
    mkdir -p ../../install/${SOC}/model
    cp examples/yolov8/model/${SOC}/yolov8n.rknn ../../install/${SOC}/model/
    cp examples/yolox/model/${SOC}/yolox_s.rknn ../../install/${SOC}/model/
    # Copy the labels (both models use COCO labels) and convert spaces to underscores
    cp examples/yolox/model/coco_80_labels_list.txt ../../install/${SOC}/model/
    sed -i 's/ /_/g' ../../install/${SOC}/model/coco_80_labels_list.txt
done
```

**The necessary binaries (model, libraries) are now in the `install` directory of the project**

**IMPORTANT**:

The classnames (`coco_80_labels_list.txt`) may have spaces. Setting these names with the Brightsign registry tool will not work. The above script automatically converts spaces to underscores (e.g., "cell phone" becomes "cell_phone") for all platforms during the copy process.

## (Optional) Step 2 - Build and test on Orange Pi

While not required, it can be handy to move the project to an OrangePi (OPi) as this facilitates a more responsive build and debug process due to a fully linux distribution and native compiler. Consult the [Orange Pi Wiki](http://www.orangepi.org/orangepiwiki/index.php/Orange_Pi_5_Plus) for more information.

Use of the Debian (Armbian) image from the eMMC is recommended. Common tools like `git`, `gcc` and `cmake` are also needed to build the project. In the interest of brevity, installation instructions for those are not included with this project.

**Requirements**

```sh
sudo apt update 
sudo apt install -y \
    cmake \
    gdb \
    libboost-all-dev \
    libturbojpeg-dev libjpeg-turbo8-dev libjpeg-turbo-progs

```

**FIRST**: Copy this entire project tree to the OPi. In particular, you'll need the compiled model from above as you cannot compile the model on the OrangePI (ARM architecture).

___Unless otherwise noted all commands in this section are executed on the OrangePi -- via ssh or other means___

**Build the project**

```sh
cd "${project_root:-.}"

# this command can be used to clean old builds
rm -rf build

mkdir -p build && cd $_

cmake .. -DTARGET_SOC="rk3588"
make
make install
```

## Step 3 - Build and Test on BrightSign Players

The BrightSign SDK for the specific BSOS version must be used on an x86 host to build the binary that can be deployed on to the BrightSign players.

_Ensure you have installed the SDK in `${project_root}/sdk` as described in Step 0 - Setup._

The setup script `environment-setup-aarch64-oe-linux` will set appropriate paths for the toolchain and files. This script must be `source`d in every new shell.

### Build for XT-5 Players (RK3588)

```sh

```

```sh
cd "${project_root:-.}"
source ./sdk/environment-setup-aarch64-oe-linux

# this command can be used to clean old builds
rm -rf build_xt5

mkdir -p build_xt5 && cd $_

cmake .. -DOECORE_TARGET_SYSROOT="${OECORE_TARGET_SYSROOT}" -DTARGET_SOC="rk3588" 
make

#rm -rf ../install
make install
```

### Build for Firebird Players (RK3576)

```sh
cd "${project_root:-.}"
source ./sdk/environment-setup-aarch64-oe-linux

# this command can be used to clean old builds
rm -rf build_firebird

mkdir -p build_firebird && cd $_

cmake .. -DOECORE_TARGET_SYSROOT="${OECORE_TARGET_SYSROOT}" -DTARGET_SOC="rk3576" 
make

#rm -rf ../install
make install
```

### Build for LS-5 Players (RK3568)

```sh
cd "${project_root:-.}"
source ./sdk/environment-setup-aarch64-oe-linux

# this command can be used to clean old builds
rm -rf build_ls5

mkdir -p build_ls5 && cd $_

cmake .. -DOECORE_TARGET_SYSROOT="${OECORE_TARGET_SYSROOT}" -DTARGET_SOC="rk3568" 
make

#rm -rf ../install
make install
```

**The built binary and libraries are copied into the `install` directory alongside the model binary.**

You can now copy that directory to the player and run it.

**Suggested workflow**

* zip the install dir and upload to player sd card
* ssh to player and exit to linux shell
* expand the zip to `/usr/local/yolo` (which is mounted with exec)

_If you are unfamiliar with this workflow or have not un-secured your player, consult BrightSign._

## Step 4 - Package the Extension

### Combined Extension for All Platforms

The extension automatically detects the target platform at runtime and uses the appropriate model and binaries. This approach creates a single extension that works across all supported platforms.

Copy the extension scripts to the install directories:

```sh
cd "${project_root:-.}"

# Copy scripts to each platform's install directory
for SOC in RK3588 RK3576 RK3568; do
    cp bsext_init install/${SOC}/ && chmod +x install/${SOC}/bsext_init
    cp sh/uninstall.sh install/${SOC}/ && chmod +x install/${SOC}/uninstall.sh
done
```

### Runtime Platform Detection

The extension automatically detects the target platform at runtime and selects the appropriate model and binaries:

- **XT-5 Players (RK3588)**: Uses files from the `RK3588/` subdirectory
- **Firebird Players (RK3576)**: Uses files from the `RK3576/` subdirectory
- **LS-5 Players (RK3568)**: Uses files from the `RK3568/` subdirectory

The platform detection is handled in the `bsext_init` script, which:

1. Detects the SoC type using system information
2. Sets the appropriate model path based on the detected platform
3. Uses the platform-specific binary and library files
4. Applies any registry overrides (e.g., custom model paths, device settings)

This approach allows a single extension package to work across all supported BrightSign platforms without requiring separate builds or installations.

#### To test the program without packing into an extension

```sh
cd "${project_root:-.}/install"

# remove any old zip files
rm -f ../yolo-dev-*.zip

zip -r ../yolo-dev-$(date +%s).zip ./
```

Copy the zip to the target, expand it, and use `bsext_init run` to test

#### Run the make extension script on the install dir

```sh
cd "${project_root:-.}"/install

../sh/make-extension-lvm
# zip for convenience to transfer to player
rm -f ../yolo-demo-*.zip 
zip ../yolo-demo-$(date +%s).zip ext_npu_yolo*
# clean up
rm -rf ext_npu_yolo*
```

### for development

* Transfer the files `yolo-*.zip` to an unsecured player with the _Browse_ and _Upload_ buttons from the __SD__ tab of DWS or other means.
* Connect to the player via ssh, telnet, or serial.
* Type Ctl-C to drop into the BrightScript Debugger, then type `exit` to the BrightSign prompt and `exit` again to get to the linux command prompt.

At the command prompt, **install** the extension with:

```bash
cd /storage/sd
# if you have multiple builds on the card, you might want to delete old ones
# or modify the unzip command to ONLY unzip the version you want to install
unzip yolo-*.zip
# you may need to answer prompts to overwrite old files

# if necessary, STOP the previous running extension
#/var/volatile/bsext/ext_npu_yolo/bsext_init stop
# make sure all processes are stopped

# install the extension
bash ./ext_npu_yolo_install-lvm.sh

# the extension will be installed on reboot
reboot
```

The gaze demo application will start automatically on boot (see `bsext_init`). Files will have been unpacked to `/var/volatile/bsext/ext_npu_gaze`.

### for production

_this section under development_

* Submit the extension to BrightSign for signing
* Contact BrightSign

## Licensing

This project is released under the terms of the [Apache 2.0 License](./LICENSE.txt).  Any model used in a BSMP must adhere to the license terms for that model.  This is discussed in more detail [here](./model-licenses.md).

Components that are part of this project are licensed seperately under their own open source licenses.

* the signed extension will be packaged as a `.bsfw` file that can be applied to a player running a signed OS.

## Removing the Extension

To remove the extension, you can perform a Factory Reset or remove the extension manually.

1. Connect to the player over SSH and drop to the Linux shell.
2. STOP the extension -- e.g. `/var/volatile/bsext/ext_npu_gaze/bsext_init stop`
3. VERIFY all the processes for your extension have stopped.
4. Unmount the extension filesystem and remove it from BOTH the `/var/volatile` filesystem AND the `/dev/mapper` filesystem.

Following the outline given by the `make-extension` script.

```bash
# EXAMPLE USAGE

# stop the extension
/var/volatile/bsext/ext_npu_yolo/bsext_init stop

# check that all the processes are stopped
# ps | grep bsext_npu_yolo

# unmount the extension
umount /var/volatile/bsext/ext_npu_yolo
# remove the extension
rm -rf /var/volatile/bsext/ext_npu_yolo

# remove the extension from the system
lvremove --yes /dev/mapper/bsos-ext_npu_yolo

# rm -rf /dev/mapper/bsext_npu_yolo
rm -rf /dev/mapper/bsos-ext_npu_yolo

# reboot
echo "Uninstallation complete. Please reboot your device to finalize the changes."
```

For convenience, an `uninstall.sh` script is packaged with the extension and can be run from the player shell.  **However**, you cannot simply run the script from the extension volume as that will lock the mount.  Copy to an executable mount first.

```bash
# copy the uninstall script to a location where it can be run
cp /var/volatile/bsext/ext_npu_yolo/uninstall.sh /usr/local/
/usr/bin/chmod +x /usr/local/uninstall.sh
/usr/local/uninstall.sh
# may need to run it twice if the first time fails because processes are still running
/usr/local/uninstall.sh

# reboot to apply changes
#reboot

```

## Architecture & Design Documentation

For detailed technical documentation covering the system's design, architecture, and development guidelines, see the following documents in the `docs/` directory:

### 📋 [Software Architecture](docs/software-architecture.md)
Comprehensive analysis of the system architecture including:
- Component design and layered architecture
- Producer-Consumer pattern implementation and rationale
- Threading model and data flow
- Multi-platform support architecture
- Configuration system design

### 🔧 [Integration & Extension Points](docs/integration-extension-points.md)
Guide for developers looking to extend or integrate with the system:
- Adding new transport protocols (MQTT example)
- Creating custom message formatters (Prometheus example)
- Performance testing extensions
- Integration patterns and best practices

### ⚖️ [Design Principles Analysis](docs/design-principles-analysis.md)
In-depth evaluation of software engineering best practices:
- SOLID principles adherence analysis
- Clean Code practices assessment
- CAP theorem considerations for distributed systems
- Design patterns implementation review
- Code quality metrics and recommendations

These documents provide essential context for understanding the codebase architecture and serve as guides for contributing to or extending the project.

## UDP Debugging: Monitor Publications

### On OrangePI (Linux/x86):

To monitor UDP messages published on port 5002, use:

```bash
nc -ul 5002
```

### On BrightSign Player:

```bash

```

Use socat to listen for UDP messages:

```bash
socat - UDP-RECV:5002
```

This will display incoming UDP messages sent to port 5002.
