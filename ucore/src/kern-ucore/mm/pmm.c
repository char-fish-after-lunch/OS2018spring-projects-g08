#include <pmm.h>
#include <string.h>
#include <error.h>
#include <memlayout.h>
#include <swap.h>
#if defined(ARCH_RISCV64) || defined(ARCH_SOC)
#include <smp.h>
#else
#include <mp.h>
#endif

/**************************************************
 * Page table operations
 **************************************************/

pgd_t *get_pgd(pgd_t * pgdir, uintptr_t la, bool create)
{
	return &pgdir[PGX(la)];
}

pud_t *get_pud(pgd_t * pgdir, uintptr_t la, bool create)
{
#if PUXSHIFT == PGXSHIFT
	return get_pgd(pgdir, la, create);
#else /* PUXSHIFT == PGXSHIFT */
	pgd_t *pgdp;
	if ((pgdp = get_pgd(pgdir, la, create)) == NULL) {
		return NULL;
	}
	if (!ptep_present(pgdp)) {
		struct Page *page;
		if (!create || (page = alloc_page()) == NULL) {
			return NULL;
		}
		set_page_ref(page, 1);
		uintptr_t pa = page2pa(page);
		memset(KADDR(pa), 0, PGSIZE);
		ptep_map(pgdp, pa);
		#ifndef ARCH_SOC
		ptep_set_u_write(pgdp);
		ptep_set_accessed(pgdp);
		ptep_set_dirty(pgdp);
		#endif
	}
	return &((pud_t *) KADDR(PGD_ADDR(*pgdp)))[PUX(la)];
#endif /* PUXSHIFT == PGXSHIFT */
}

pmd_t *get_pmd(pgd_t * pgdir, uintptr_t la, bool create)
{
#if PMXSHIFT == PUXSHIFT
	return get_pud(pgdir, la, create);
#else /* PMXSHIFT == PUXSHIFT */
	pud_t *pudp;
	if ((pudp = get_pud(pgdir, la, create)) == NULL) {
		return NULL;
	}
	if (!ptep_present(pudp)) {
		struct Page *page;
		if (!create || (page = alloc_page()) == NULL) {
			return NULL;
		}
		set_page_ref(page, 1);
		uintptr_t pa = page2pa(page);
		memset(KADDR(pa), 0, PGSIZE);
		ptep_map(pudp, pa);
#if !defined(ARCH_RISCV64) && !defined(ARCH_SOC)
		ptep_set_u_write(pudp);
		ptep_set_accessed(pudp);
		ptep_set_dirty(pudp);
#endif
	}
	return &((pmd_t *) KADDR(PUD_ADDR(*pudp)))[PMX(la)];
#endif /* PMXSHIFT == PUXSHIFT */
}

pte_t *get_pte(pgd_t * pgdir, uintptr_t la, bool create)
{
#if PTXSHIFT == PMXSHIFT
	return get_pmd(pgdir, la, create);
#else /* PTXSHIFT == PMXSHIFT */
	pmd_t *pmdp;
	if ((pmdp = get_pmd(pgdir, la, create)) == NULL) {
		return NULL;
	}
	if (!ptep_present(pmdp)) {
		struct Page *page;
		if (!create || (page = alloc_page()) == NULL) {
			return NULL;
		}
		set_page_ref(page, 1);
		uintptr_t pa = page2pa(page);
		memset(KADDR(pa), 0, PGSIZE);
#ifdef ARCH_ARM
		pdep_map(pmdp, pa);
#else
		ptep_map(pmdp, pa);
#endif
		/* ARM9 PDE does not have access field */
#ifndef ARCH_ARM
#if !defined(ARCH_RISCV64) && !defined(ARCH_SOC)
		ptep_set_u_write(pmdp);
		ptep_set_accessed(pmdp);
		ptep_set_dirty(pmdp);
#endif
#endif
	}
	return &((pte_t *) KADDR(PMD_ADDR(*pmdp)))[PTX(la)];
#endif /* PTXSHIFT == PMXSHIFT */
}

/**
 * get related Page struct for linear address la using PDT pgdir
 * @param pgdir page directory
 * @param la linear address
 * @param ptep_store table entry stored if not NULL
 * @return @la's corresponding page descriptor
 */
struct Page *get_page(pgd_t * pgdir, uintptr_t la, pte_t ** ptep_store)
{
	pte_t *ptep = get_pte(pgdir, la, 0);
	if (ptep_store != NULL) {
		*ptep_store = ptep;
	}
	if (ptep != NULL && ptep_present(ptep)) {
		return pa2page(*ptep);
	}
	return NULL;
}

/**
 * page_remove_pte - free an Page sturct which is related linear address la
 *                 - and clean(invalidate) pte which is related linear address la
 * @param pgdir page directory (not used)
 * @param la logical address of the page to be removed
 * @param page table entry of the page to be removed
 * note: PT is changed, so the TLB need to be invalidate 
 */
void page_remove_pte(pgd_t * pgdir, uintptr_t la, pte_t * ptep)
{
	if (ptep_present(ptep)) {
		struct Page *page = pte2page(*ptep);
		if (!PageSwap(page)) {
			if (page_ref_dec(page) == 0) {
				//Don't free dma pages
				if (!PageIO(page))
					free_page(page);
			}
		} else {
			if (ptep_dirty(ptep)) {
				SetPageDirty(page);
			}
			page_ref_dec(page);
		}
		ptep_unmap(ptep);
		mp_tlb_invalidate(pgdir, la);
	} else if (!ptep_invalid(ptep)) {
#ifdef UCONFIG_SWAP
		swap_remove_entry(*ptep);
#endif
		ptep_unmap(ptep);
	}
}

/**
 * page_insert - build the map of phy addr of an Page with the linear addr @la
 * @param pgdir page directory
 * @param page the page descriptor of the page to be inserted
 * @param la logical address of the page
 * @param perm permission of the page
 * @return 0 on success and error code when failed
 */
int page_insert(pgd_t * pgdir, struct Page *page, uintptr_t la, pte_perm_t perm)
{
	pte_t *ptep = get_pte(pgdir, la, 1);
	if (ptep == NULL) {
		return -E_NO_MEM;
	}
	page_ref_inc(page);
	if (*ptep != 0) {
		if (ptep_present(ptep) && pte2page(*ptep) == page) {
			page_ref_dec(page);
			goto out;
		}
		page_remove_pte(pgdir, la, ptep);
	}

out:
	ptep_map(ptep, page2pa(page));
	ptep_set_perm(ptep, perm);
	mp_tlb_update(pgdir, la);
	return 0;
}

#ifdef UCONFIG_BIONIC_LIBC
void
page_insert_pte(pgd_t * pgdir, struct Page *page, pte_t * ptep, uintptr_t la,
		pte_perm_t perm)
{
	page_ref_inc(page);
	if (*ptep != 0) {
		if (ptep_present(ptep) && pte2page(*ptep) == page) {
			page_ref_dec(page);
		} else {
			page_remove_pte(pgdir, la, ptep);
		}
	}
	ptep_map(ptep, page2pa(page));
	ptep_set_perm(ptep, perm);
	mp_tlb_update(pgdir, la);
}
#endif //UCONFIG_BIONIC_LIBC

/**
 * page_remove - free an Page which is related linear address la and has an validated pte
 * @param pgdir page directory
 * @param la logical address of the page to be removed
 */
void page_remove(pgd_t * pgdir, uintptr_t la)
{
	pte_t *ptep = get_pte(pgdir, la, 0);
	if (ptep != NULL) {
		page_remove_pte(pgdir, la, ptep);
	}
}

/**
 * pgdir_alloc_page - call alloc_page & page_insert functions to 
 *                  - allocate a page size memory & setup an addr map
 *                  - pa<->la with linear address la and the PDT pgdir
 * @param pgdir    page directory
 * @param la       logical address for the page to be allocated
 * @param perm     permission of the page
 * @return         the page descriptor of the allocated
 */
struct Page *pgdir_alloc_page(pgd_t * pgdir, uintptr_t la, uint32_t perm)
{
	struct Page *page = alloc_page();
	if (page != NULL) {
		//zero it!
		memset(page2kva(page), 0, PGSIZE);
		if (page_insert(pgdir, page, la, perm) != 0) {
			free_page(page);
			return NULL;
		}
	}
	return page;
}

/**************************************************
 * memory management for users
 **************************************************/

static void
unmap_range_pte(pgd_t * pgdir, pte_t * pte, uintptr_t base, uintptr_t start,
		uintptr_t end)
{
	assert(start >= 0 && start < end && end <= PTSIZE);
	assert(start % PGSIZE == 0 && end % PGSIZE == 0);
	do {
		pte_t *ptep = &pte[PTX(start)];
		if (*ptep != 0) {
			page_remove_pte(pgdir, base + start, ptep);
		}
		start += PGSIZE;
	} while (start != 0 && start < end);
}

static void
unmap_range_pmd(pgd_t * pgdir, pmd_t * pmd, uintptr_t base, uintptr_t start,
		uintptr_t end)
{
#if PMXSHIFT == PUXSHIFT
	unmap_range_pte(pgdir, pmd, base, start, end);
#else
	assert(start >= 0 && start < end && end <= PMSIZE);
	size_t off, size;
	uintptr_t la = ROUNDDOWN(start, PTSIZE);
	do {
		off = start - la, size = PTSIZE - off;
		if (size > end - start) {
			size = end - start;
		}
		pmd_t *pmdp = &pmd[PMX(la)];
		if (ptep_present(pmdp)) {
			unmap_range_pte(pgdir, KADDR(PMD_ADDR(*pmdp)),
					base + la, off, off + size);
		}
		start += size, la += PTSIZE;
	} while (start != 0 && start < end);
#endif
}

static void
unmap_range_pud(pgd_t * pgdir, pud_t * pud, uintptr_t base, uintptr_t start,
		uintptr_t end)
{
#if PUXSHIFT == PGXSHIFT
	unmap_range_pmd(pgdir, pud, base, start, end);
#else
	assert(start >= 0 && start < end && end <= PUSIZE);
	size_t off, size;
	uintptr_t la = ROUNDDOWN(start, PMSIZE);
	do {
		off = start - la, size = PMSIZE - off;
		if (size > end - start) {
			size = end - start;
		}
		pud_t *pudp = &pud[PUX(la)];
		if (ptep_present(pudp)) {
			unmap_range_pmd(pgdir, KADDR(PUD_ADDR(*pudp)),
					base + la, off, off + size);
		}
		start += size, la += PMSIZE;
	} while (start != 0 && start < end);
#endif
}

static void unmap_range_pgd(pgd_t * pgd, uintptr_t start, uintptr_t end)
{
	size_t off, size;
	uintptr_t la = ROUNDDOWN(start, PUSIZE);
	do {
		off = start - la, size = PUSIZE - off;
		if (size > end - start) {
			size = end - start;
		}
		pgd_t *pgdp = &pgd[PGX(la)];
		if (ptep_present(pgdp)) {
			unmap_range_pud(pgd, KADDR(PGD_ADDR(*pgdp)), la, off,
					off + size);
		}
		start += size, la += PUSIZE;
	} while (start != 0 && start < end);
}

void unmap_range(pgd_t * pgdir, uintptr_t start, uintptr_t end)
{
	assert(start % PGSIZE == 0 && end % PGSIZE == 0);
	assert(USER_ACCESS(start, end));
	unmap_range_pgd(pgdir, start, end);
}

static void exit_range_pmd(pmd_t * pmd)
{
#if PMXSHIFT == PUXSHIFT
	/* do nothing */
#else
	uintptr_t la = 0;
	do {
		pmd_t *pmdp = &pmd[PMX(la)];
		if (ptep_present(pmdp)) {
			free_page(pmd2page(*pmdp)), *pmdp = 0;
		}
		la += PTSIZE;
	} while (la != PMSIZE);
#endif
}

static void exit_range_pud(pud_t * pud)
{
#if PUXSHIFT == PGXSHIFT
	exit_range_pmd(pud);
#else
	uintptr_t la = 0;
	do {
		pud_t *pudp = &pud[PUX(la)];
		if (ptep_present(pudp)) {
			exit_range_pmd(KADDR(PUD_ADDR(*pudp)));
			free_page(pud2page(*pudp)), *pudp = 0;
		}
		la += PMSIZE;
	} while (la != PUSIZE);
#endif
}

static void exit_range_pgd(pgd_t * pgd, uintptr_t start, uintptr_t end)
{
	start = ROUNDDOWN(start, PUSIZE);
	do {
		pgd_t *pgdp = &pgd[PGX(start)];
		if (ptep_present(pgdp)) {
			exit_range_pud(KADDR(PGD_ADDR(*pgdp)));
			free_page(pgd2page(*pgdp)), *pgdp = 0;
		}
		start += PUSIZE;
	} while (start != 0 && start < end);
}

void exit_range(pgd_t * pgdir, uintptr_t start, uintptr_t end)
{
	assert(start % PGSIZE == 0 && end % PGSIZE == 0);
	assert(USER_ACCESS(start, end));
	exit_range_pgd(pgdir, start, end);
}


/* copy_range - copy content of memory (start, end) of one process A to another process B
 * @to:    the addr of process B's Page Directory
 * @from:  the addr of process A's Page Directory
 * @share: flags to indicate to dup OR share. We just use dup method, so it didn't be used.
 *
 * CALL GRAPH: copy_mm-->dup_mmap-->copy_range
 */
int
copy_range(pte_t *to, pte_t *from, uintptr_t start, uintptr_t end, bool share) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));
    // copy content by page unit.
    do {
        //call get_pte to find process A's pte according to the addr start
        pte_t *ptep = get_pte(from, start, 0), *nptep;
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            continue ;
        }
        //call get_pte to find process B's pte according to the addr start. If pte is NULL, just alloc a PT
        if (*ptep & PTE_P) {
			if ((nptep = get_pte(to, start, 1)) == NULL) {
				return -E_NO_MEM;
			}
			uint32_t perm = (*ptep & PTE_USER);
			struct Page *page = pte2page(*ptep);
			assert(page!=NULL);

			if(share){
				int ret = page_insert(to, page, start, perm);
				assert(ret == 0);
			} else{
				//get page from ptep
				// alloc a page for process B
				struct Page *npage=alloc_page();
				assert(npage!=NULL);
				int ret=0;
				void * kva_src = page2kva(page);
				void * kva_dst = page2kva(npage);
			
				memcpy(kva_dst, kva_src, PGSIZE);

				ret = page_insert(to, npage, start, perm);
				assert(ret == 0);
			}
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
    return 0;
}



