QEMU            		?= qemu-riscv32
export HOST_CC_PREFIX	?=
export TARGET_CC_PREFIX	?=	riscv32-unknown-elf-
export TARGET_CC_FLAGS_COMMON	?=	-mcmodel=medany -std=gnu99 -Wextra\
									-fno-builtin -Wall -O2 -nostdinc \
									-fno-stack-protector -ffunction-sections -fdata-sections
export TARGET_CC_FLAGS_BL		?=
export TARGET_CC_FLAGS_KERNEL	?=
export TARGET_CC_FLAGS_SV		?=
export TARGET_CC_FLAGS_USER		?=
export TARGET_LD_FLAGS			?=	-m elf32lriscv -nostdlib

qemu: all
	${QEMU} -m 512 \
	-hda ${T_OBJ}/kernel.img \
	-drive file=${T_OBJ}/swap.img,media=disk,cache=writeback \
	-drive file=${T_OBJ}/sfs.img,media=disk,cache=writeback \
	-s -S \
	-serial file:${T_OBJ}/serial.log -monitor stdio

debug: all
	${V}gdb -q -x gdbinit.${ARCH}