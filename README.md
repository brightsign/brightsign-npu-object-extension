# BrightSign YOLO Object Detection BSMP (ALPHA RELEASE)

This is an example BrightSign Model Package (BSMP) that implements Object Detection using YOLO frameworks (YOLO Simplified and YOLOX) on the BrightSign player NPU. This can be used as a template for development of other BSMP by partners and third-parties.

BSMP are delivered as an BrightSign OS (BOS) "extension." Extensions are delivered as firmware update files that are installed on a reboot. These are basically Linux squashfs file systems that extend the firmware to include the BSMP. You can learn more about extensions in our [Extension Template Repository](https://github.com/brightsign/extension-template).

## Supported Players

| player | minimum OS Version required |
| --- | --- |
| XT-5: XT1145, XT2145 | [9.1.52](https://brightsignbiz.s3.amazonaws.com/firmware/xd5/9.1/9.1.52/brightsign-xd5-update-9.1.52.zip) |
| _Firebird_ | [BETA-9.1.52](https://bsnbuilds.s3.us-east-1.amazonaws.com/firmware/brightsign-demos/9.1.52-BETA/BETA-cobra-9.1.52-update.bsfw) |
| _LS-5: LS445_ | [BETA-9.1.52](https://bsnbuilds.s3.us-east-1.amazonaws.com/firmware/brightsign-demos/9.1.52-BETA/BETA-cobra-9.1.52-update.bsfw) |

**NOTE-** This guide is written **ONLY** for the XT-5. Supporting Firebird or LS-5 is a straightforward exercise for the motivated reader.

## Supported Cameras

In general, any camera supported by Linux *should* work.  We've had great luck with Logitech cameras, especially the C270.

## Decorated Camera Output

Every frame of video captured is processed through the model.  Every detected object has a bounding box drawn around it.  The image is written to a file on the /tmp folder.  This is a ram disk so it will not impact the life of the storage.

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

1. Compile ONNX formatted models for use on the Rockchip RK3588 SoC -- used in the OrangePi 5 and XT-5 Player.
2. Develop and test an AI Application to load and run the model on the RK3588.
3. Build the AI Application for BrightSign OS
4. Package the Application and model as a BrightSign Extension

This project supports both YOLO Simplified and YOLOX models from the [Rockchip Model Zoo](https://github.com/airockchip/rknn_model_zoo). The application code in this repo was adapted from the example code from the Rockchip Model Zoo. Please ensure that you are aware of the license that your chosen model is released under. More information on model licenses can be seen [here](./model-licenses.md).

## Application Overview

This project will create an installable BrightSign Extension that supports two operational modes:

### V4L Device Mode (Continuous Inference)

When provided with a Video for Linux device (e.g., `/dev/video0`), the extension:

1. Loads the compiled YOLO model (YOLO Simplified or YOLOX) into the Rockchip NPU
2. Continuously captures video frames from an attached USB webcam using OpenCV
3. Runs YOLO object detection on each frame at ~30 FPS
4. Continuously updates the decorated image to `/tmp/output.jpg`
5. Writes inference results to `/tmp/results.json` once per second
6. Publishes person detection data via UDP to localhost ports 5002 and 5003

### One-Shot Image Mode (Single Inference)

When provided with an image file path (e.g., `/tmp/input.jpg`), the extension:

1. Loads the compiled YOLO model (YOLO Simplified or YOLOX) into the Rockchip NPU
2. Processes the single input image file
3. Runs YOLO object detection once on the provided image
4. Saves the decorated result image to `/tmp/output.jpg`
5. Writes the inference results to `/tmp/results.json`
6. Exits after processing

### Extension Control

This extension allows three optional registry keys to be set to customize behavior:

* Disable the auto-start of the extension -- this can be useful in debugging or other problems
* Set the `v4l` device filename to override the auto-discovered device
* Set a custom model path to use different YOLO models

**Registry keys are organized in the `extension` section**

| Registry Key | Values | Effect |
| --- | --- | --- |
| `bsext-yolo-auto-start` | `true` or `false` | when truthy, disables the extension from autostart (`bsext_init start` will simply return). The extension can still be manually run with `bsext_init run` |
| `bsext-yolo-video-device` | a valid v4l device file name like `/dev/video0` or `/dev/video1` | normally not needed, but may be useful to override for some unusual or test condition |
| `bsext-yolo-model-path` | full path to a `.rknn` model file | allows using custom YOLO models (YOLO Simplified or YOLOX) instead of the default model |

## Project Overview & Requirements

This repository describes building the project in these major steps:

1. Compile the ONNX formatted model into _RKNN_ format for the Rockchip NPU
2. Building and testing the model and application code on an [Orange Pi 5 Plus](http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/service-and-support/Orange-Pi-5-plus.html). ___NB__-_ this is optional, but is included as a guide to developing other applications
3. Building and testing the model and application code on a [BrightSign XT-5 Player](https://www.brightsign.biz/brightsign-players/series-5/xt5/)
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
rm brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz
rm brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz

```

___IMPORTANT___: Building an OpenEmbedded project can be very particular in terms of packages and setup. For that reason it __strongly recommended__ to use the [Docker build](https://github.com/brightsign/extension-template/blob/main/README.md#recommended-docker) approach.

```sh
# Build the SDK in Docker -- RECOMMENDED
cd "${project_root:-.}"

wget https://raw.githubusercontent.com/brightsign/extension-template/refs/heads/main/Dockerfile
docker build --rm --build-arg USER_ID=$(id -u) --build-arg GROUP_ID=$(id -g) --ulimit memlock=-1:-1 -t bsoe-build .

mkdir -p srv
# the build process puts some output in srv

sudo sh -c 'echo 120000 > /proc/sys/vm/max_map_count'
# increase the map count to allow docker to build
sudo sh -c 'echo "vm.max_map_count = 120000" > /etc/sysctl.d/local.conf'
# make it permanent

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
# Compile YOLO Simplified for RK3588 -- XT-5 players
cd "${project_root:-.}"/toolkit/rknn_model_zoo/

mkdir -p examples/yolov8/model/RK3588
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolov8/python && python convert.py ../model/yolov8n.onnx rk3588 i8 ../model/RK3588/yolov8n.rknn"

# Compile YOLOX for RK3588 -- XT-5 players  
mkdir -p examples/yolox/model/RK3588
docker run -it --rm -v $(pwd):/zoo rknn_tk2 /bin/bash \
    -c "cd /zoo/examples/yolox/python && python convert.py ../model/yolox_s.onnx rk3588 i8 ../model/RK3588/yolox_s.rknn"

# Copy models and labels to install directory
mkdir -p ../../install/RK3588/model
cp examples/yolov8/model/RK3588/yolov8n.rknn ../../install/RK3588/model/
cp examples/yolox/model/RK3588/yolox_s.rknn ../../install/RK3588/model/
# copy the labels (both models use COCO labels)
cp examples/yolox/model/coco_80_labels_list.txt ../../install/RK3588/model/
```

**The necessary binaries (model, libraries) are now in the `install` directory of the project**

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

## Step 3 - Build and Test on XT5

The BrightSign SDK for the specific BSOS version must be used on an x86 host to build the binary that can be deployed on to the XT5 player.

_Ensure you have installed the SDK in `${project_root}/sdk` as described in Step 0 - Setup._

The setup script `environment-setup-aarch64-oe-linux` will set appropriate paths for the toolchain and files. This script must be `source`d in every new shell.

### Build the app

```sh
cd "${project_root:-.}"
source ./sdk/environment-setup-aarch64-oe-linux

# this command can be used to clean old builds
# rm -rf build_xt5

mkdir -p build_xt5 && cd $_

cmake .. -DOECORE_TARGET_SYSROOT="${OECORE_TARGET_SYSROOT}" -DTARGET_SOC="rk3588" 
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

Copy the extension scripts to the install dir

```sh
cd "${project_root:-.}"

cp bsext_init install/ && chmod +x install/bsext_init
cp sh/uninstall.sh install/ && chmod +x install/uninstall.sh

# cp -rf model install/
```

#### To test the program without packing into an extension

```sh
cd "${project_root:-.}/install"

# remove any old zip files
#rm -f ../yolo-dev-*.zip

zip -r ../yolo-dev-$(date +%s).zip ./
```

Copy the zip to the target, expand it, and use `bsext_init run` to test

#### Run the make extension script on the install dir

```sh
cd "${project_root:-.}"/install

../sh/make-extension-lvm
# zip for convenience to transfer to player
#rm -f ../yolo-demo-*.zip 
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
