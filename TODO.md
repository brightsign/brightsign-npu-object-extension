# TODO

## purpose and tasks

[ ] compeletely redact all mentions of yolo other than YOLOX. 

[ ] support for yolov8 must be removed - do not download the model

[ ] the features should be referred to as 'object detection'

[ ] using the term 'YOLOX' is acceptable; "YOLO" or "YOLOV8" must be removed

[ ] replace filename references to "yolo" with "object-detect" (preferred) or 'objdet' when shorter names are needed 

[ ] remove cli args from scripts and other programs that may select model variants by name

## background

Big picture:  I don't want to say yolo much in the repo if it can be avoided.  Consistency wise, the others are 'gaze-extension' and 'voice-extension' so I don't want it to be 'yolo-extension' - it needs to be 'object-extension.'
AND, the name of the artifacts and the extension itself need to not say yolo.
AND, it seems we build yolov8n - and we just need to clip that out.  Can't even have the sniff of that.
      elif [[ -f "install/RK3568/model/yolov8n.rknn" && -f "install/RK3576/model/yolov8n.rknn" && -f "install/RK3588/model/yolov8n.rknn" ]]; then
  # Compile models if we need to (either not built or forced)
            if [[ "$FORCE_MODELS" == true ]] || [[ ! -f "install/RK3568/model/yolov8n.rknn" || ! -f "install/RK3576/model/yolov8n.rknn" || ! -f "install/RK3588/model/yolov8n.rknn" ]]; then


## checks

[ ] all documentation follows the lead from the edited README
[ ] all scripts do not use 'yolo' or 'yolov8' anywhere -- the only exception is the model download 
[ ] no code comments say 'yolo' or 'yolov8'
[ ] compile-models script has with no model choices -- e.g. `yolov8`. ONLY yolox
[ ] executable binary name should not contain 'yolo' -- use 'object_detection_demo'
[ ] extension name should not contain 'yolo' -- use `bsext-objdet`
[ ] no output files such as `/tmp/yolo_output*` should contain 'yolo', use 'objdet'
[ ] manifest files and samples should not refer to 'yolo' only 'objdet'
[ ] github action output directory should not contain 'yolo' use 'object-detect'
[ ] registry keys should be renamed to not cotain 'yolo' use 'object-detect'. this includes class names with should just be referred to as object classes not yolo-classes
[ ] the extension name is less than 10 characters
[ ] the package script does not take a model argument and only uses yolox
[ ] make lvm script uses object-detect or objdet not yolo


## completion

Create a summary of the changes that
- gives the size of the changes -- number of files, lines of code
- details the name changes someone who integrates with the extension needs to know
  - S3 prefix name
  - extenion name
  - extension path
  - output file name
  - registry keys
  - other names or symbols that may be referenced
- provide a checklist for users of the extension
Place this summary under 'Key Requirements' in the plan doc