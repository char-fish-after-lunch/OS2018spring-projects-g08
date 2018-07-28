#include <elf.h>
#include <mod.h>
#include <bitman.h>
#include <assert.h>

int apply_relocate(struct secthdr *sechdrs,
				   const char *strtab,
				   unsigned int symindex,
				   unsigned int relsec, struct module *mod)
{
	kprintf("REL type relocation tables not supported!\n");
	return -1;
}

/* Apply the given add relocation to the (simplified) ELF.  Return
	-error or 0 */
int apply_relocate_add(struct secthdr *sechdrs,
					   const char *strtab,
					   unsigned int symindex,
					   unsigned int relsec, struct module *mod)
{
	static_assert(sizeof(struct secthdr) == 0x28);
	static_assert(sizeof(struct symtab_s) == 16);
	static_assert(sizeof(struct reloc_a_s) == 12);

	unsigned int i;
	struct reloc_a_s *rel = (struct reloc_a_s *)sechdrs[relsec].sh_addr;
	struct symtab_s *sym;
	uint32_t *location;
	uint32_t o_val, hi, lo;

	kprintf("Applying relocate section %u to %u\n", relsec,
			sechdrs[relsec].sh_info);
	for (i = 0; i < sechdrs[relsec].sh_size / sizeof(struct reloc_a_s); i++)
	{
		/* This is where to make the change */
		location = (void *)(sechdrs[sechdrs[relsec].sh_info].sh_addr + rel[i].r_offset);
		// kprintf("Offset %04lx : ", rel[i].r_offset);
		o_val = *location;
		// kprintf("RELOC %016lx ... ", (unsigned long)location);
		/* This is the symbol it is referring to.  Note that all
		   undefined symbols have been resolved.  */
		sym = (struct symtab_s *)sechdrs[symindex].sh_addr + GET_RELOC_SYM(rel[i].r_info);
		uint32_t val = sym->st_value + rel[i].r_addend;
		// kprintf("VAL = %016lx ", val);
		uint32_t tmp;

		switch (GET_RELOC_TYPE(rel[i].r_info))
		{
		case R_RISCV_32:
			// kprintf(" R_RISCV_64 ");
			*location = val;
			break;
		case R_RISCV_LO12_I:
			// kprintf(" R_RISCV_LO12_I ");
			*location = SET_BITS(*location, 20, 12, PICK_BITS(lo, 0, 12));
			break;
		case R_RISCV_LO12_S:
			// kprintf(" R_RISCV_LO12_S ");
			*location = SET_BITS(*location, 7, 5, PICK_BITS(lo, 0, 5));
			*location = SET_BITS(*location, 25, 7, PICK_BITS(lo, 5, 7));
			break;
		case R_RISCV_HI20:
			// kprintf(" R_RISCV_HI20 ");
			tmp = (void*)val - (void*)location;
			hi = PICK_BITS(tmp + 0x800, 12, 20);
			lo = PICK_BITS(tmp, 0, 12);
			*location = SET_BITS(*location, 12, 20, hi);
			*location = SET_BITS(*location, 0, 7, 0x17); // replace lui with auipc
			break;
		case R_RISCV_RELAX:
		case R_RISCV_ALIGN:
			// kprintf(" R_RISCV_RELAX ");
			// Linker relaxation not implemented
			break;
		case R_RISCV_CALL:
			// kprintf(" R_RISCV_CALL ");

			tmp = (void*)val - (void*)location;
			// kprintf("Jumping offset: %016x\n", tmp);
			hi = PICK_BITS(tmp + 0x800, 12, 20);
			lo = PICK_BITS(tmp, 0, 12);
			*location = SET_BITS(*location, 12, 20, hi);
			location = (void*)location + 4;
			*location = SET_BITS(*location, 20, 12, lo);
			break;
		case R_RISCV_RVC_JUMP:
			// kprintf(" R_RISCV_RVC_JUMP ");
			tmp = (void*)val - (void*)location;
			*location = SET_BITS(*location, 2, 11, (PICK_BITS(tmp, 11, 1) << 10) |
				(PICK_BITS(tmp, 4, 1) << 9) | (PICK_BITS(tmp, 8, 2) << 7) | 
				(PICK_BITS(tmp, 10, 1) << 6) | (PICK_BITS(tmp, 6, 1) << 5) |
				(PICK_BITS(tmp, 7, 1) << 4) | (PICK_BITS(tmp, 1, 3) << 1) |
				PICK_BITS(tmp, 5, 1));
			break;
		case R_RISCV_RVC_BRANCH:
			// kprintf(" R_RISCV_RVC_BRANCH ");
			tmp = (void*)val - (void*)location;
			*location = SET_BITS(*location, 12, 1, PICK_BITS(tmp, 8, 1));
			*location = SET_BITS(*location, 10, 2, PICK_BITS(tmp, 3, 2));
			*location = SET_BITS(*location, 5, 2, PICK_BITS(tmp, 6, 2));
			*location = SET_BITS(*location, 3, 2, PICK_BITS(tmp, 1, 2));
			*location = SET_BITS(*location, 2, 1, PICK_BITS(tmp, 5, 1));
			break;
		case R_RISCV_BRANCH:
			// kprintf(" R_RISCV_BRANCH ");
			tmp = (void*)val - (void*)location;
			*location = SET_BITS(*location, 25, 7, (PICK_BITS(tmp, 12, 1) << 6) |
				PICK_BITS(tmp, 5, 6));
			*location = SET_BITS(*location, 7, 5, (PICK_BITS(tmp, 1, 4) << 1) |
				PICK_BITS(tmp, 11, 1));
			break;
		case R_RISCV_JAL:
			// kprintf(" R_RISCV_JAL ");
			tmp = (void*)val - (void*)location;
			*location = SET_BITS(*location, 12, 20, (PICK_BITS(tmp, 20, 1) << 19) |
				(PICK_BITS(tmp, 1, 10) << 9) | (PICK_BITS(tmp, 11, 1) << 8) |
				PICK_BITS(tmp, 12, 8));
			break;
		default:
			kprintf("apply_relocate: module %s: Unknown relocation: %u\n",
					mod->name, GET_RELOC_TYPE(rel[i].r_info));
			return -1;
		}
		// kprintf(" %016lx -> %016lx\n", o_val, *location);
	}
	return 0;
}
