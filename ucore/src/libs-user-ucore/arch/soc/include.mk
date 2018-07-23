ARCH_CFLAGS := -DARCH_SOC\
				-mcmodel=kernel -std=gnu99 -Wextra\
				-fno-builtin -Wall -O2 -nostdinc \
				-fno-stack-protector -ffunction-sections -fdata-sections
ARCH_LDFLAGS := -m elf32lriscv -nostdlib
ARCH_OBJS := clone.o syscall.o
ARCH_INITCODE_OBJ := initcode.o
