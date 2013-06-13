/*	$NetBSD: elf_machdep.h,v 1.7 2002/01/28 21:34:48 thorpej Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_ALTERA_NIOS2:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID	EM_ALTERA_NIOS2

/*
 * Machine-dependent ELF flags.  These are defined by the GNU tools.
 */
#define	EF_NIOS2	0x00810000

#define ARCH_ELFSIZE		32	/* MD native binary size */

/* NIOS2 relocation types */
#define R_NIOS2_NONE          0
#define R_NIOS2_S16           1
#define R_NIOS2_U16           2
#define R_NIOS2_PCREL16       3
#define R_NIOS2_CALL26        4
#define R_NIOS2_IMM5          5
#define R_NIOS2_CACHE_OPX     6
#define R_NIOS2_IMM6          7
#define R_NIOS2_IMM8          8
#define R_NIOS2_HI16          9
#define R_NIOS2_LO16          10
#define R_NIOS2_HIADJ16       11
#define R_NIOS2_BFD_RELOC_32  12
#define R_NIOS2_BFD_RELOC_16  13
#define R_NIOS2_BFD_RELOC_8   14
#define R_NIOS2_GPREL         15
#define R_NIOS2_GNU_VTINHERIT 16
#define R_NIOS2_GNU_VTENTRY   17
#define R_NIOS2_UJMP          18
#define R_NIOS2_CJMP          19
#define R_NIOS2_CALLR         20
#define R_NIOS2_ALIGN         21

#define	R_TYPE(name)	__CONCAT(R_NIOS2_,name)
