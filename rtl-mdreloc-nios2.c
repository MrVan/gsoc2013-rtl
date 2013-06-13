/*
 * Taken from NetBSD and stripped of the relocations not needed on RTEMS.
 */

/*	$NetBSD: mdreloc.c,v 1.26 2010/01/14 11:58:32 skrll Exp $	*/

#include <sys/cdefs.h>

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <rtl.h>
#include "rtl-elf.h"
#include "rtl-error.h"
#include <rtl-trace.h>

bool
rtems_rtl_elf_rel_resolve_sym (Elf_Word type)
{
  return true;
}


/*
 * R is the relocated address
 * B is the bit shift
 * M is the bit mask
 * X is the original instruction
 * Xr is the relocated instruction
 *
 */
#define Adj(ADDR) (((((ADDR) >> 16) & 0xffff) +  (((ADDR) >> 15) & 0x1)) & 0xffff)
#define S (symvalue)
#define A (rela->r_addend)
#define Xr ((((R) << (B)) & (M)) | (((X) & ~(M))))

bool
rtems_rtl_elf_relocate_rela (const rtems_rtl_obj_t*      obj,
                             const Elf_Rela*             rela,
                             const rtems_rtl_obj_sect_t* sect,
                             const char*                 symname,
                             const Elf_Byte              syminfo,
                             const Elf_Word              symvalue)
{
  Elf_Addr *where;
  Elf_Word R,B,M,X;

  where = (Elf_Addr *)(sect->base + rela->r_offset);

  switch (ELF_R_TYPE(rela->r_offset)) {
    case R_TYPE(NONE):
      break;
    
    case R_TYPE(HIADJ16):
    case R_TYPE(HI16):
    case R_TYPE(LO16):
      if (ELF_R_TYPE(rela->r_offset) == R_TYPE(HIADJ16))
        R = Adj(S + A);
      else if (ELF_R_TYPE(rela->r_offset) == R_TYPE(HI16))
        R = ((S + A) >> 16) & 0xffff;
      else  R = (S + A) & 0xffff;

      X = *where;
      B = 6;
      M = 0x003fffc0;

      *where = Xr;

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: HIADJ16/HI16/LO16 %p @ %p in %s\n",
                (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;

    case R_TYPE(CALL26):

      X = *where;
      R = (S + A) >> 2;
      B = 6;
      M = 0xffffffc0;

      *where = Xr;

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: CALL26 %p @ %p in %s\n",
                (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;

    case R_TYPE(BFD_RELOC_32):

      X = *where;
      R = S + A;
      B = 0;
      M = 0xffffffff;

      *where = Xr;

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: BFD_RELOC_32 %p @ %p in %s\n",
                (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;

    case R_TYPE(PCREL16):

      X = *where;
      R = S + A - 4 - (Elf_Addr)where;
      if ((R & 0xffff0000) && (~R & 0xffff8000)) {
        printf("PCREL16 overflow\n"); 
        return false;
      }
      B = 6;
      M = 0x003fffc0;

      *where = Xr;

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: PCREL16 %p @ %p in %s\n",
                (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;
    
    default:
      printf ("rtl: reloc unknown: sym = %lu, type = %lu, offset = %p, "
              "contents = %p\n",
              ELF_R_SYM(rela->r_info), (uint32_t) ELF_R_TYPE(rela->r_info),
              (void *)rela->r_offset, (void *)*where);
      return false;
  
  }
   return true;
}

bool
rtems_rtl_elf_relocate_rel (const rtems_rtl_obj_t*      obj,
                            const Elf_Rel*              rel,
                            const rtems_rtl_obj_sect_t* sect,
                            const char*                 symname,
                            const Elf_Byte              syminfo,
                            const Elf_Word              symvalue)
{
  printf ("rtl: rel type record not supported; please report\n");
  return false;
}
