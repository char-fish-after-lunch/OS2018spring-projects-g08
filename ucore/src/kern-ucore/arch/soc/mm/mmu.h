#ifndef __KERN_MM_MMU_H__
#define __KERN_MM_MMU_H__

#ifndef __ASSEMBLER__
#include <types.h>
#endif

// A linear address 'la' has a four-part structure as follows:
//
// +--------9-------+-------9--------+-------9--------+---------12----------+
// | Page Directory | Page Directory |   Page Table   | Offset within Page  |
// |     Index 1    |    Index 2     |                |                     |
// +----------------+----------------+----------------+---------------------+
//  \-- PDX1(la) --/ \-- PDX0(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \-------------------PPN(la)----------------------/
//
// The PDX1, PDX0, PTX, PGOFF, and PPN macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).

// RISC-V uses 39-bit virtual address to access 56-bit physical address!
// Sv39 virtual address:
// +----9----+----9---+----9---+---12--+
// |  VPN[2] | VPN[1] | VPN[0] | PGOFF |
// +---------+----+---+--------+-------+
//
// Sv39 physical address:
// +----26---+----9---+----9---+---12--+
// |  PPN[2] | PPN[1] | PPN[0] | PGOFF |
// +---------+----+---+--------+-------+
//
// Sv39 page table entry:
// +----26---+----9---+----9---+---2----+-------8-------+
// |  PPN[2] | PPN[1] | PPN[0] |Reserved|D|A|G|U|X|W|R|V|
// +---------+----+---+--------+--------+---------------+

#define SATP_SV39 0x8000000000000000 // Sv39

#define PTE_SIZE 8 // One PTE is 8 bytes

#ifndef __ASSEMBLER__

// page directory index 0
#define PDX0(la) ((((uintptr_t)(la)) >> PDX0SHIFT) & 0x1FF)

// page directory index 1
#define PDX1(la) ((((uintptr_t)(la))>>PDX1SHIFT) & 0x1FF)

// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x1FF)

// page number field of address
#define PPN(la) (((uintptr_t)(la)) >> PTXSHIFT)

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d1, d0, t, o) ((uintptr_t)((d1) << PDX1SHIFT | (d0) << PDX0SHIFT | (t) << PTXSHIFT | (o)))

#else

// page directory index 0
#define PDX0(la) ((((la)) >> PDX0SHIFT) & 0x1FF)

// page directory index 1
#define PDX1(la) ((((la))>>PDX1SHIFT) & 0x1FF)

// page table index
#define PTX(la) ((((la)) >> PTXSHIFT) & 0x1FF)

// page number field of address
#define PPN(la) (((la)) >> PTXSHIFT)

// offset in page
#define PGOFF(la) (((la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d1, d0, t, o) (((d1) << PDX1SHIFT | (d0) << PDX0SHIFT | (t) << PTXSHIFT | (o)))

#endif

// address in page table or page directory entry
#define PTE_ADDR(pte)   (((uintptr_t)(pte) & ~0x3FF) << (PTXSHIFT - PTE_PPN_SHIFT))
#define PDE_ADDR(pde)   PTE_ADDR(pde)

/* page directory and page table constants */
#define NPDEENTRY       512                    // page directory entries per page directory
#define NPTEENTRY       512                    // page table entries per page table

#define PGSIZE          4096                    // bytes mapped by a page
#define PGSHIFT         12                      // log2(PGSIZE)
#define PTSIZE          (PGSIZE * NPTEENTRY)    // bytes mapped by a page directory entry
#define PTSHIFT			21						// log2(PTSIZE)

#define PTXSHIFT        12                      // offset of PTX in a linear address
#define PDX0SHIFT       21                      // offset of PDX[0] in a linear address
#define PDX1SHIFT		30						// offset of PDX[1] in a linear address
#define PTE_PPN_SHIFT   10                      // offset of PPN in a physical address


// for ucore+ kern-ucore/mm/pmm.c
// #define PTXSHIFT        12     // has been defined above
#define PMXSHIFT		PDX0SHIFT
#define PUXSHIFT		PDX1SHIFT
#define PGXSHIFT		PDX1SHIFT

// page directory index
#define PMX(la) PDX0(la)
#define PUX(la) PDX1(la)
#define PGX(la) PDX1(la)

// address in page table or page directory entry
#define PMD_ADDR(pmd)   PTE_ADDR(pmd)
#define PUD_ADDR(pud)   PTE_ADDR(pud)
#define PGD_ADDR(pgd)   PTE_ADDR(pgd)

/* page directory and page table constants */
// in ucore+ PTSIZE means bytes mapped by a pmd entry
#define PMSIZE			(1LLU * NPDEENTRY * PTSIZE) // bytes mapped by a pud entry
#define PUSIZE			PMSIZE // bytes mapped by a pgd entry



// page table entry (PTE) fields
#define PTE_V     0x001 // Valid
#define PTE_R     0x002 // Read
#define PTE_W     0x004 // Write
#define PTE_X     0x008 // Execute
#define PTE_U     0x010 // User
#define PTE_G     0x020 // Global
#define PTE_A     0x040 // Accessed
#define PTE_D     0x080 // Dirty
#define PTE_SOFT  0x300 // Reserved for Software

#define PAGE_TABLE_DIR (PTE_V)
#define READ_ONLY (PTE_R | PTE_V)
#define READ_WRITE (PTE_R | PTE_W | PTE_V)
#define EXEC_ONLY (PTE_X | PTE_V)
#define READ_EXEC (PTE_R | PTE_X | PTE_V)
#define READ_WRITE_EXEC (PTE_R | PTE_W | PTE_X | PTE_V)

#define PTE_USER (PTE_R | PTE_W | PTE_X | PTE_U | PTE_V)


// for ucore+ kern-ucore/mm/pmm.c
#define PTE_SWAP        (PTE_A | PTE_D)
#define PTE_P           PTE_V


#ifndef __ASSEMBLER__

typedef uintptr_t pgd_t;
typedef uintptr_t pud_t;
typedef uintptr_t pmd_t;
typedef uintptr_t pte_t;
typedef pte_t swap_entry_t;
typedef pte_t pte_perm_t;

static inline void ptep_map(pte_t * ptep, uintptr_t pa)
{
	*ptep = (pa >> (PGSHIFT - PTE_PPN_SHIFT) | PTE_V);
}

static inline void ptep_unmap(pte_t * ptep)
{
	*ptep = 0;
}

static inline int ptep_invalid(pte_t * ptep)
{
	return (*ptep == 0);
}

static inline int ptep_present(pte_t * ptep)
{
	return (*ptep & PTE_V);
}

static inline int ptep_s_read(pte_t * ptep)
{
	return (*ptep & PTE_R);
}

static inline int ptep_s_write(pte_t * ptep)
{
	return (*ptep & PTE_W);
}

static inline int ptep_u_read(pte_t * ptep)
{
	return ((*ptep & PTE_U) & (*ptep & PTE_R));
}

static inline int ptep_u_write(pte_t * ptep)
{
	return ((*ptep & PTE_U) && (*ptep & PTE_W));
}

static inline void ptep_set_s_read(pte_t * ptep)
{
    *ptep |= PTE_R;
}

static inline void ptep_set_s_write(pte_t * ptep)
{
    *ptep |= PTE_W | PTE_R;
}

static inline void ptep_set_u_read(pte_t * ptep)
{
	*ptep |= PTE_U | PTE_R;
}

static inline void ptep_set_u_write(pte_t * ptep)
{
	*ptep |= PTE_W | PTE_R | PTE_U;
}

static inline void ptep_set_u(pte_t * ptep)
{
	*ptep |= PTE_U;
}

static inline void ptep_unset_u(pte_t * ptep)
{
	*ptep &= ~PTE_U;
}

static inline void ptep_unset_s_read(pte_t * ptep)
{
    *ptep &= (~(PTE_R | PTE_W));
}

static inline void ptep_unset_s_write(pte_t * ptep)
{
	*ptep &= (~PTE_W);
}

static inline void ptep_unset_u_read(pte_t * ptep)
{
    *ptep &= (~(PTE_R | PTE_W | PTE_U));
}

static inline void ptep_unset_u_write(pte_t * ptep)
{
	*ptep &= (~(PTE_W | PTE_U));
}

static inline pte_perm_t ptep_get_perm(pte_t * ptep, pte_perm_t perm)
{
	return (*ptep) & perm;
}

static inline void ptep_set_perm(pte_t * ptep, pte_perm_t perm)
{
	if (perm & PTE_W) {
		perm |= PTE_R;
	}
	*ptep |= perm;
}

static inline void ptep_copy(pte_t * to, pte_t * from)
{
	*to = *from;
}

static inline void ptep_unset_perm(pte_t * ptep, pte_perm_t perm)
{
	if (perm & PTE_R) {
		perm |= PTE_W;
	}
	*ptep &= (~perm);
}

static inline int ptep_accessed(pte_t * ptep)
{
	return *ptep & PTE_A;
}

static inline int ptep_dirty(pte_t * ptep)
{
	return *ptep & PTE_D;
}

static inline void ptep_set_accessed(pte_t * ptep)
{
	*ptep |= PTE_A;
}

static inline void ptep_set_dirty(pte_t * ptep)
{
	*ptep |= PTE_D;
}

static inline void ptep_unset_accessed(pte_t * ptep)
{
	*ptep &= (~PTE_A);
}

static inline void ptep_unset_dirty(pte_t * ptep)
{
	*ptep &= (~PTE_D);
}

#endif


#endif /* !__KERN_MM_MMU_H__ */
