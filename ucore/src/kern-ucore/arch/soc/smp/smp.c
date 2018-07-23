#include <smp.h>
#include <mmu.h>
#include <pmm.h>
#include <vmm.h>

struct cpu cpus[NCPU];

// int myid(){
//     return mycpu()->id;
// }

extern uintptr_t boot_cr3;

void smp_init(){
    int i;
    for(i = 0; i < NCPU; i ++){
        cpus[i].id = i;
    }
}

void mp_tlb_invalidate(pgd_t* pgdir, uintptr_t la){
    tlb_invalidate(pgdir, la);
}

void mp_tlb_update(pgd_t* pgdir, uintptr_t la){
    tlb_update(pgdir, la);
}

void mp_set_mm_pagetable(struct mm_struct* mm){
    mp_tlb_flush();
    if(mm == NULL)
        lcr3(boot_cr3);
    else
        lcr3(PADDR(mm->pgdir));
}

void mp_tlb_flush() {
    asm volatile ("sfence.vma");
}

