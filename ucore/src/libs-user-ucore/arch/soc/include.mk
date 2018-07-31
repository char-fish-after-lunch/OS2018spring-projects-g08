ARCH_CFLAGS := -march=rv32ia -DARCH_SOC\
				-mcmodel=medany -std=gnu99 -Wextra\
				-fno-builtin -Wall -O2 -nostdinc \
				-fno-stack-protector -ffunction-sections -fdata-sections
ARCH_LDFLAGS := -m elf32lriscv -nostdlib
ARCH_OBJS := clone.o syscall.o udivmodsi4.o udivmod.o divmod.o mult.o
ARCH_INITCODE_OBJ := initcode.o
