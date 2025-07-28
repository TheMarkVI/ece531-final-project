#!/bin/sh

# Please ensure you have set the correct path to your buildroot output directory.
export PATH=$PATH:/home/art/ece531/buildroot/buildroot-2025.05/output/host/bin:${PATH}

qemu-system-arm \
  -M versatilepb \
  -kernel zImage \
  -dtb versatile-pb.dtb \
  -drive file=rootfs.ext2,if=scsi,format=raw \
  -append "root=/dev/sda console=ttyAMA0,115200" \
  -serial stdio \
  -netdev user,id=ether0,hostfwd=tcp:127.0.0.1:2222-:22 \
  -device rtl8139,netdev=ether0 \

# exec qemu-system-arm -M versatilepb -kernel zImage -dtb versatile-pb.dtb -drive file=rootfs.ext2,if=scsi,format=raw -append "rootwait root=/dev/sda console=ttyAMA0,115200"  -net nic,model=rtl8139 -net user  ${EXTRA_ARGS} "$@"
