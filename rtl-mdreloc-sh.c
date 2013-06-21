#include <sys/cdefs.h>

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <rtl.h>
#include "rtl-elf.h"
#include "rtl-error.h"
#include <rtl-trace.h>

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
  Elf_Addr *where;
  Elf_Addr  tmp;

  where = (Elf_Addr *)(sect->base + rela->r_offset);

  if (rtems_rtl_trace (RTEMS_RTL_TRACE_RELOC)) {
      printf("rela relocation type is %ld\n", ELF_R_TYPE(rela->r_info));
      printf("relocated address 0x%08lx\n", (Elf_Addr)where);
  }

  switch (ELF_R_TYPE(rela->r_info)) {
    case R_TYPE(NONE):
      break;

    case R_TYPE(DIR32):
      tmp = load_ptr(where);
      tmp += symvalue;
      store_ptr(where, tmp);
      break;

    default:
      rtems_rtl_set_error (EINVAL, "rela type record not supported");
      printf("Unspported reloc type\n");
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
  rtems_rtl_set_error (EINVAL, "rel type record not supported");
  return false;
}
