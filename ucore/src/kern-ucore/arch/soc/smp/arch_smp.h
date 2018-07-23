#ifndef UCORE_SMP_ARCH_H
#define UCORE_SMP_ARCH_H

inline struct cpu* mycpu(){
  struct cpu* cpu;
  asm("mv %0, tp;" : "=r"(cpu));
  return cpu;
}

#endif

