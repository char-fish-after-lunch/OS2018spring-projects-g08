ARCH_INLUCDES := . debug driver include libs mm numa process sync trap syscall smp kmodule driver/acpica/source/include
ARCH_CFLAGS :=  -march=rv32ia -DARCH_SOC -mcmodel=medany -std=gnu99 -Wextra\
				-fno-builtin -Wall -O2 -nostdinc \
				-fno-stack-protector -ffunction-sections -fdata-sections \
				-fno-omit-frame-pointer -fno-optimize-sibling-calls
ARCH_LDFLAGS := -m elf32lriscv -nostdlib
