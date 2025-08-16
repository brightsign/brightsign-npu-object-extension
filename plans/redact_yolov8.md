# Plan: Complete Redaction of YOLOv8 and YOLO References

## Objective

Completely remove all YOLOv8 functionality and most YOLO references from the repository, keeping only YOLOX as the supported model.

## Key Requirements

- Remove all YOLOv8 model support (no download, compilation, or processing)
- Replace "YOLO" references with "object detection" or "objdet"
- Keep "YOLOX" references as acceptable
- Extension name must be <10 characters: `bsext-obj`
- Binary name: `object_detection_demo`
- Package names: `objdet-dev-*.zip` and `objdet-ext-*.zip`
- Registry keys: `bsext-obj-*`
- No backward compatibility needed (okay to break existing installations)

## Phase 1: Core Script Changes

### compile-models

- [x] Remove yolov8 from MODELS array (lines 33-36) - only keep yolox
- [x] Remove all yolov8 model compilation logic
- [x] Remove yolov8n.onnx prerequisite checks (lines 178-180)
- [x] Update all help text to remove yolov8 references
- [x] Remove yolov8 from copy_models function (lines 267-269, 290-293)
- [x] Update script header to "Object Detection - Model Compilation Script"

### package

- [ ] Change package names from `yolo-dev-` to `objdet-dev-`
- [ ] Change package names from `yolo-ext-` to `objdet-ext-`
- [ ] Remove `--model` argument entirely or restrict to only yolox
- [ ] Change binary name from `yolo_demo` to `object_detection_demo` throughout
- [ ] Update all help text and comments

### setup

- [ ] Remove all yolov8 model download/setup logic
- [ ] Update script headers and comments

## Phase 2: Build System Changes

### CMakeLists.txt

- [ ] Change project name from `yolo_demo` to `object_detection_demo`
- [ ] Rename executable from `yolo_demo` to `object_detection_demo`
- [ ] Rename source file reference from `yolo.cc` to `yolox.cc`

### build-apps

- [ ] Update script header from "YOLO Object Detection" to "Object Detection"
- [ ] Update all help text and comments

### build

- [ ] Update any YOLO references to "Object Detection"

## Phase 3: Shell Script Changes

### sh/make-extension-lvm

- [ ] Change `name=npu_yolo` to `name=npu_obj`
- [ ] Update `mapper_vol_name=ext_npu_obj`
- [ ] Update `tmp_vol_name=tmp_npu_obj`
- [ ] Update `mount_name=ext_npu_obj`

### bsext_init

- [ ] Change `DAEMON_NAME="bsext-objdet"` to `DAEMON_NAME="bsext-obj"`
- [ ] Fix line 727: change `run_yolo_demo` to `run_object_detection_demo`
- [ ] Update all registry key references from `bsext-objdet-*` to `bsext-obj-*`
- [ ] Update backup directories to use `objdet` instead of `yolo`
- [ ] Change `/tmp/yolo_output` to `/tmp/objdet_output`

### sh/update-extension.sh

- [ ] Change `/tmp/yolo_output` to `/tmp/objdet_output`
- [ ] Update `bsext-yolo` references to `bsext-obj`
- [ ] Update systemctl commands

### sh/rollback-extension.sh

- [ ] Update `npu_yolo` references to `npu_obj`
- [ ] Update extension name references

### sh/uninstall.sh

- [ ] Update all `npu_yolo` references to `npu_obj`
- [ ] Update `bsext_npu_yolo` to `bsext_npu_obj`
- [ ] Update mount paths

### sh/setup_python_env

- [ ] Update path references from `ext_npu_yolo` to `ext_npu_obj`

## Phase 4: C++ Code Changes

### src/yolo.cc → src/yolox.cc

- [ ] Rename file to yolox.cc
- [ ] Remove YoloModelType enum completely
- [ ] Remove detect_model_type() function
- [ ] Remove all YOLO_SIMPLIFIED and YOLOv8 detection logic
- [ ] Keep only YOLOX processing code
- [ ] Update all comments

### src/postprocess.cc

- [ ] Remove all `process_yolov8_*` functions (lines 469-656)
- [ ] Remove YOLO_SIMPLIFIED handling in process_simplified()
- [ ] Remove 9-output structure handling (lines 1058-1115)
- [ ] Update all comments to remove YOLOv8 references

### include/yolo.h → include/yolox.h

- [ ] Rename file to yolox.h
- [ ] Remove YoloModelType enum
- [ ] Update header guards
- [ ] Update function declarations

### include/postprocess.h

- [ ] Remove YOLOv8-related function declarations
- [ ] Update comments

### src/inference.cpp

- [ ] Update includes from yolo.h to yolox.h
- [ ] Update any YOLO references

### src/main.cpp

- [ ] Update includes and references
- [ ] Update any YOLO mentions in output messages

### include/inference.h

- [ ] Update any YOLO references in comments

### include/frame_writer.h

- [ ] Update any YOLO references in comments

## Phase 5: Configuration Changes

### manifest-config.json

- [ ] Change description to "NPU-accelerated object detection"
- [ ] Update all documentation references

### manifest-config.template.json

- [ ] Update template with same changes
- [ ] Remove YOLO references from comments

### Registry Keys (throughout codebase)

Change all registry keys:

- [ ] `bsext-yolo-disable-auto-start` → `bsext-obj-disable-auto-start`
- [ ] `bsext-yolo-video-device` → `bsext-obj-video-device`
- [ ] `bsext-yolo-model-path` → `bsext-obj-model-path`
- [ ] `bsext-yolo-classes` → `bsext-obj-classes`
- [ ] `bsext-yolo-confidence-threshold` → `bsext-obj-confidence-threshold`

## Phase 6: Documentation Updates

### README.md

- [ ] Complete the partial updates already started
- [ ] Fix all registry key examples to use `bsext-obj-*`
- [ ] Update installation script name to `ext_npu_obj_install-lvm.sh`
- [ ] Remove YOLOv8 from supported models section
- [ ] Update package names in examples
- [ ] Remove all YOLOv8 references from model compatibility
- [ ] Update GitHub Actions output directory reference

### docs/software-architecture.md

- [ ] Update registry key diagram to use `bsext-obj-*`
- [ ] Remove YOLOv8 from architecture description
- [ ] Update all YOLO references to "object detection"

### docs/manifest-guide.md

- [ ] Update extension name examples
- [ ] Update registry key examples

### docs/integration-extension-points.md

- [ ] Update any YOLO references

### docs/design-principles-analysis.md

- [ ] Update any YOLO references

### OrangePI_Development.md

- [ ] Remove YOLOv8 references
- [ ] Update example commands

### versioning_plan.md

- [ ] Update extension name references

### model-licenses.md

- [ ] **KEEP UNCHANGED for reference**

## Phase 7: GitHub Actions

### .github/workflows/build-extension.yml

- [ ] Change S3_URL from `brightsign-npu-yolox` to `brightsign-npu-object-detect`
- [ ] Change uploaded file names from `yolo-dev-*.zip` to `objdet-dev-*.zip`
- [ ] Change uploaded file names from `yolo-ext-*.zip` to `objdet-ext-*.zip`

## Phase 8: Scripts and Automation

### scripts/runall.sh

- [ ] Update all "YOLO Object Detection" references to "Object Detection"
- [ ] Update package file references
- [ ] Update installation script name

## Phase 9: Test and Configuration Files

### tests/CMakeLists.txt

- [ ] Update executable name references
- [ ] Update source file references

### .vscode/launch.json

- [ ] Update program paths from yolo_demo to object_detection_demo
- [ ] Update any YOLOv8 references

### .vscode/tasks.json

- [ ] Update task labels and commands

### .gitignore

- [ ] Update patterns if needed for new naming

## Validation Checklist

After all changes:

- [ ] No "yolo" or "yolov8" appears in code except "YOLOX"
- [ ] Binary name is `object_detection_demo`
- [ ] Extension name is `bsext-obj` (9 chars, <10 requirement)
- [ ] Package files use `objdet-dev-` and `objdet-ext-`
- [ ] Registry keys use `bsext-obj-*`
- [ ] Output files use `/tmp/objdet_output`
- [ ] No YOLOv8 model support in compile-models
- [ ] No YOLOv8 processing in C++ code
- [ ] LVM volume name is `npu_obj`
- [ ] Installation script is `ext_npu_obj_install-lvm.sh`
- [ ] GitHub Actions S3 path is `brightsign-npu-object-detect`
- [ ] Only YOLOX model is supported
- [ ] Documentation is fully updated

## Files Summary

**Total files to modify:** 37
**Files to rename:** 2 (yolo.cc → yolox.cc, yolo.h → yolox.h)
**Files to keep unchanged:** 1 (model-licenses.md)

## Testing After Implementation

1. Build and compile only YOLOX model
2. Test package creation with new names
3. Verify extension installation with new name
4. Test registry key functionality
5. Verify object detection still works with YOLOX
6. Check GitHub Actions workflow
7. Validate all documentation accuracy