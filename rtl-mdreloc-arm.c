/*
 * Taken from NetBSD and stripped of the relocations not needed on RTEMS.
 */

/*  $NetBSD: mdreloc.c,v 1.33 2010/01/14 12:12:07 skrll Exp $  */

#include <sys/cdefs.h>

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <rtl.h>
#include "rtl-elf.h"
#include "rtl-error.h"
#include <rtl-trace.h>

/*
 * It is possible for the compiler to emit relocations for unaligned data.
 * We handle this situation with these inlines.
 */
#define	RELOC_ALIGNED_P(x) \
	(((uintptr_t)(x) & (sizeof(void *) - 1)) == 0)

static inline Elf_Addr
load_ptr(void *where)
{
	Elf_Addr res;

	memcpy(&res, where, sizeof(res));

	return (res);
}

static inline void
store_ptr(void *where, Elf_Addr val)
{

	memcpy(where, &val, sizeof(val));
}

/* Thumb symbol address is (the real address) + 1
 * Here the symtype is not considered, although
 * the elf abi for ARM tells that only the target symbol
 * type is STT_FUNC and the target symbol is a thumb
 * address, then T is 1.Because extern functions's 
 * symtype is NOTYPE, it's meaningless to care it.
 */
static inline int 
isThumb(Elf_Word symvalue)
{
  if ((symvalue & 0x1) == 0x1) 
    return true;
  else return false;
}

bool
rtems_rtl_elf_rel_resolve_sym (Elf_Word type)
{
  return true;
}

bool
rtems_rtl_elf_relocate_rela (const rtems_rtl_obj_t*      obj,
                             const Elf_Rela*             rela,
                             const rtems_rtl_obj_sect_t* sect,
                             const char*                 symname,
                             const Elf_Byte              syminfo,
                             const Elf_Word              symvalue)
{
  rtems_rtl_set_error (EINVAL, "rela type record not supported");
  return false;
}

bool
rtems_rtl_elf_relocate_rel (const rtems_rtl_obj_t*      obj,
                            const Elf_Rel*              rel,
                            const rtems_rtl_obj_sect_t* sect,
                            const char*                 symname,
                            const Elf_Byte              syminfo,
                            const Elf_Word              symvalue)
{
  Elf_Addr *where;
  Elf_Addr  tmp;
  Elf_Sword addend;
  Elf_Word val;
  Elf_Word symvalue_b;
  uint32_t sign, i1, i2;
  uint16_t lower_insn, upper_insn;
  uint16_t upper, lower;
  uint32_t j1,j2;

  where = (Elf_Addr *)(sect->base + rel->r_offset);

  switch (ELF_R_TYPE(rel->r_info)) {
    case R_TYPE(NONE):
      break;

    case R_TYPE(PREL31):
      tmp = *where;
      addend = tmp & 0x7fffffff;
      if (addend & 0x40000000)
        addend |= 0x80000000;
      if ((addend > 0x7fffffff) || ((int32_t)addend < -0x3fffffff)) {
        rtems_rtl_set_error (EINVAL, "%s: Overflow %ld "
            "PREL31 relocations",
            sect->name, (uint32_t) ELF_R_TYPE(rel->r_info));
        return false;
      }

      tmp = symvalue + addend;
      if (isThumb(symvalue))
        tmp |= 1;
      *where = tmp - (Elf_Addr)where;

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: PREL31 %p @ %p in %s\n",
            (void *)tmp, where, rtems_rtl_obj_oname (obj));
      break;

    case R_TYPE(PC24): /* Deprecated */
    case R_TYPE(CALL): /* BL/BLX */
    case R_TYPE(JUMP24): /* B/BL<cond> */
      addend = *where;
      if (addend & 0x00800000)
        addend |= 0xff000000;

      symvalue_b = symvalue;
      if (isThumb(symvalue)) {
        if ((*where & 0xfe000000) == 0xfa000000); /* Already blx; do not care */
        else {
          Elf_Word insn;
          insn = *where;
          if ((insn & 0xff000000) == 0xeb000000) { /* CALL */
            insn = (0xfa000000 | (insn & 0x00ffffff));
            *where = insn;
          } else { /* JUMP24 */

            /*
             * b/bl<cond> relocation type is R_ARM_JUMP24
             * If the target address is thumb address, how
             * to handle the arm/thumb switch.
             * It means how to convert b/bl<cond> to bx/blx.
             */
            printf("JUMP24 arm to thumb jump failed\n");
            return false;
          }
        }
      }

      tmp = symvalue_b + (addend << 2);
      tmp -= (Elf_Addr)where;
      tmp >>= 2;

      *where = (*where & 0xff000000) | (tmp & 0x00ffffff);

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: JUMP24/PC24/CALL %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;

    case R_TYPE(V4BX): /* Miscellaneous */
#if 0
      tmp = *where;
      tmp &= 0xf000000f; /* Keep cond and Rm*/
      tmp |= 0x01a0f000; /* Mov PC, Rm */
#endif
      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC)) {
        printf ("rtl: V4BX %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      }
      break;

    case R_TYPE(MOVT_ABS):
    case R_TYPE(MOVW_ABS_NC):
      tmp = *where;
      addend = ((tmp >> 4) & 0xf000) | (tmp & 0xfff);
      if (addend & 0x8000)
        addend |= 0xffff0000;
      tmp = symvalue + addend;
      if (ELF_R_TYPE(rel->r_info) == R_ARM_MOVW_ABS_NC) {
        tmp &= 0xffff;	
      } else if (ELF_R_TYPE(rel->r_info) == R_ARM_MOVT_ABS) {
        tmp >>= 16;	
        if (((int)tmp >= 0x8000) || ((int)tmp < -0x8000)) {
          rtems_rtl_set_error (EINVAL, "%s: Overflow %ld "
              "MOVT_ABS relocations",
              sect->name, (uint32_t) ELF_R_TYPE(rel->r_info));
          return false;
        }
      }

      *where = (*where & 0xfff0f000) | ((tmp & 0xf000) << 4) | (tmp & 0xfff);

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: MOVT_ABS/MOVW_ABS_NC %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;

    case R_TYPE(REL32): /* word32 (S + A) | T - P */
    case R_TYPE(ABS32):	/* word32 (S + A) | T */
    case R_TYPE(GLOB_DAT):	/* word32 (S + A) | T */
      if (__predict_true(RELOC_ALIGNED_P(where))) {
        tmp = *where + symvalue;
        /* Set the Thumb bit, if needed.  */
        if (isThumb(symvalue))
          tmp |= 1;
        if (ELF_R_TYPE(rel->r_info) == R_TYPE(REL32))
          tmp -= (Elf_Addr)where;
        *where = tmp;
      } else {
        tmp = load_ptr(where) + symvalue;
        /* Set the Thumb bit, if needed.  */
        if (isThumb(symvalue))
          tmp |= 1;
        if (ELF_R_TYPE(rel->r_info) == R_TYPE(REL32))
          tmp -= (Elf_Addr)where;
        store_ptr(where, tmp);
      }
      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: REL32/ABS32/GLOB_DAT %p @ %p in %s",
            (void *)tmp, where, rtems_rtl_obj_oname (obj));
      break;


    case R_TYPE(THM_MOVT_ABS): /* word32 S + A*/
    case R_TYPE(THM_MOVW_ABS_NC): /* word32 (S + A) | T */
      upper_insn = *(uint16_t *)where;
      lower_insn = *((uint16_t *)where + 1);

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf("upper_insn 0x%02x lower_insn 02%02x\n", upper_insn, lower_insn);
      addend = ((upper_insn & 0x000f) << 12) |
        ((upper_insn & 0x0400) << 1) |
        ((lower_insn & 0x7000) >> 4) | (lower_insn & 0x00ff);
      addend = (addend ^ 0x8000) - 0x8000;
      tmp = addend + symvalue;

      if (ELF32_R_TYPE(rel->r_info) == R_ARM_THM_MOVT_ABS)
        tmp >>= 16;

      *(uint16_t *)where = (uint16_t)((upper_insn & 0xfbf0) |
          ((tmp & 0xf000) >> 12) |
          ((tmp & 0x0800) >> 1));
      *((uint16_t *)where + 1) = (uint16_t)((lower_insn & 0x8f00) |
          ((tmp & 0x0700) << 4) |
          (tmp & 0x00ff));

      upper_insn = *(uint16_t *)where;
      lower_insn = *((uint16_t *)where + 1);

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC)) {
        printf("upper_insn 0x%02x lower_insn 0x%02x\n", upper_insn, lower_insn);
        printf ("rtl: THM_MOVT_ABS/THM_MOVW_ABS_NC %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      }

      break;

    case R_TYPE(THM_JUMP24):
      /* same to THM_CALL; insn b.w */
    case R_TYPE(THM_CALL): 
      upper_insn = *(uint16_t *)where;
      lower_insn = *((uint16_t *)where + 1);
      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf("upper_insn 0x%04x lower_insn 0x%04x\n", upper_insn, lower_insn);
      sign = (upper_insn & (1 << 10)) >> 10;
      i1 = ((lower_insn >> 13) & 1) ^ sign ? 0 : 1;
      i2 = ((lower_insn >> 11) & 1) ^ sign ? 0 : 1;
      tmp = (i1 << 23) | (i2 << 22) | ((upper_insn & 0x3ff) << 12) | ((lower_insn & 0x7ff) << 1);
      addend = (tmp | ((sign ? 0 : 1) << 24)) - (1 << 24);

      symvalue_b = symvalue;
      if (isThumb(symvalue)) ;/*Thumb to Thumb call, nothing to care */
      else {
        if (ELF_R_TYPE(rel->r_info) == R_TYPE(THM_JUMP24)) {
          tmp = (tmp + 2) & ~3; /* aligned to 4 bytes only for JUMP24 */
          printf("THM_JUMP24 to arm not supported\n");
          return false;
        }
        else {
          /* THM_CALL bl-->blx */
          lower_insn &=~(1<<12);
        }
      }

      tmp = symvalue_b + addend;
      tmp = tmp - (Elf_Addr)where;

      if (((int32_t)tmp > (int32_t)(1<<24)) || ((int32_t)tmp < (int32_t)(0xff<<24))) {
        rtems_rtl_set_error (EINVAL, "%s: Overflow %ld "
            "THM_CALL/THM_JUMP24 relocations",
            sect->name, (uint32_t) ELF_R_TYPE(rel->r_info));
        return false;
      }

      sign = (tmp >> 24) & 1;
      *(uint16_t *)where = (uint16_t)((upper_insn & 0xf800) | (sign << 10) |
          ((tmp >> 12) & 0x3ff));

      *((uint16_t *)where + 1) = (uint16_t)((lower_insn & 0xd000)|
          ((sign ^ (~(tmp >> 23) & 1)) << 13) |
          ((sign ^ (~(tmp >> 22) & 1)) << 11) |
          ((tmp >> 1) & 0x7ff));

      upper_insn = *(uint16_t *)where;
      lower_insn = *((uint16_t *)where + 1);
      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC)){
        printf("modified upper_insn 0x%04x lower_insn 0x%04x\n", upper_insn, lower_insn);
        printf ("rtl: THM_CALL/JUMP24 %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      }

      break;

    case R_TYPE(THM_JUMP19):
      /*
       * operation ((S + A) | T) - P
       * overflow: yes;class:Thumb32; insn: B<cond>.W/beq.w; result mask: X & 0x001ffffe
       */												 
      if (!isThumb(symvalue)) {
        printf("THM_JUMP19 to arm not supported\n");	
        return false;
      }
      upper_insn = *(uint16_t *)where;
      lower_insn = *((uint16_t *)where + 1);
      printf("upper_insn 0x%04x lower_insn 0x%04x\n", upper_insn, lower_insn);
      sign = (upper_insn >> 10) & 0x1;
      upper = upper_insn & 0x3f;
      if (((upper >> 7) & 0x7) == 0x7) {
        printf("THM_JUMP19 failed\n");
        return false; /*if cond <3:1> == '111', see Related codings in armv7a manual */
      }
      j1 = (lower_insn >> 13) & 0x1;
      j2 = (lower_insn >> 11) & 0x1;
      lower = lower_insn & 0x7ff;

      tmp = ((j2 << 19) | (j1 << 18) | (upper << 12) | (lower << 1));
      addend = (tmp | ((sign ? 0 : 1) << 20)) - (1 << 20);
      tmp = symvalue + addend;

      tmp = tmp - (Elf_Addr)where;

      if (((int32_t)tmp > (int32_t)(0xffffe)) || ((int32_t)tmp < (int32_t)(0xfff<<20))) {
        rtems_rtl_set_error (EINVAL, "%s: Overflow %ld "
            "THM_JUMP19 relocations",
            sect->name, (uint32_t) ELF_R_TYPE(rel->r_info));
        return false;
      }
      sign = (tmp >> 20) & 0x1;
      j2 = (tmp >> 19) & 0x1;
      j1 = (tmp >> 18) & 0x1;
      *(uint16_t*)where = (upper_insn & 0xfbc0) | (sign << 10) | ((tmp >> 12) & 0x3f);
      *((uint16_t*)where + 1) = (lower_insn & 0xd000) | (j1 << 13) | 
        (j2 << 11) | ((tmp >> 1) & 0x7ff);

      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: THM_JUMP19 %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;

#if 1
    case R_TYPE(THM_ABS5):
      addend = (*(uint16_t *)where & 0x7e0) >> 6;
      tmp = symvalue + addend;
      if (((int)tmp > 0x1f) || ((int)tmp < -0x10)) {
        printf("THM_ABS5 overflow\n");
        return false;	
      }
      *(uint16_t *)where = (*(uint16_t *)where & 0xf83f) | tmp;
      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: THM_ABS5 %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;
#endif
      int bit,mask;
      int min,max;
    case R_TYPE(THM_JUMP6):
      bit = 6;
      mask = 0x2f8;
    case R_TYPE(THM_JUMP11): 
      bit = 11;
      mask = 0x7ff;
    case R_TYPE(THM_JUMP8):
      bit = 8;
      mask = 0xff;
      max = (1 << (bit - 1)) - 1;
      min = ~max;
      if (ELF_R_TYPE(rel->r_info) == R_TYPE(THM_JUMP6))
        min = 0;

      tmp = *(uint16_t *)where;
      addend = tmp & mask;
      if ((addend & (1 << (bit - 1))) == (1 << (bit - 1)))
        addend |= ~((1 << bit) - 1);
      addend <<= 1;
      tmp = symvalue + addend - (Elf_Addr)where;
      tmp >>= 1;
      if (ELF_R_TYPE(rel->r_info) == R_TYPE(THM_JUMP6)) {
        tmp = ((tmp & 0x0020) << 4) | ((tmp & 0x001f) << 3);
      }
      else tmp &= mask;

      /* Why comment this? Because objdump 0xd0eb  0xeb > max:8bit*/
      /* Do not care this */
#if 0
      if (((int32_t)tmp > max) || (int32_t)tmp < min) {
        printf("THM_JUMP11/JUMP8 overflow\n");	
        return false;
      }
#endif

      *(uint16_t *)where = (*(uint16_t *)where & ~((1<<bit)-1)) | tmp;
      if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC))
        printf ("rtl: THM_JUMP8/JUMP11/JUMP6 %p @ %p in %s\n",
            (void *)*where, where, rtems_rtl_obj_oname (obj));
      break;

    default:
      printf ("rtl: reloc unknown: sym = %lu, type = %lu, offset = %p, "
          "contents = %p\n",
          ELF_R_SYM(rel->r_info), (uint32_t) ELF_R_TYPE(rel->r_info),
          (void *)rel->r_offset, (void *)*where);
      rtems_rtl_set_error (EINVAL,
          "%s: Unsupported relocation type %ld "
          "in non-PLT relocations",
          sect->name, (uint32_t) ELF_R_TYPE(rel->r_info));
      return false;
  }

  return true;
}

