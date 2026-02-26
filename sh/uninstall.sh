#!/bin/bash

# stop the extension
/var/volatile/bsext/ext_npu_obj/bsext_init stop

# check that all the processes are stopped
# ps | grep bsext_npu_obj

# unmount the extension
umount /var/volatile/bsext/ext_npu_obj
# remove the extension
rm -rf /var/volatile/bsext/ext_npu_obj

# remove the extension from the system
# lvremove --yes /dev/mapper/ext_npu_obj
# if that path does not exist, you can try
lvremove --yes /dev/mapper/bsos-ext_npu_obj

# rm -rf /dev/mapper/bsext_npu_obj
rm -rf /dev/mapper/bsos-ext_npu_obj

# reboot
echo "Uninstallation complete. Please reboot your device to finalize the changes."