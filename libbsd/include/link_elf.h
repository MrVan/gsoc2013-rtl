/*	$NetBSD: link_elf.h,v 1.8 2009/11/04 19:28:03 pooka Exp $	*/

/*
 * This only exists for GDB.
 */

#ifndef _LINK_ELF_H_
#define	_LINK_ELF_H_

#include <sys/types.h>

#include <machine/elf_machdep.h>
#include <stdint.h>

enum sections
{
  rap_text = 0,
  rap_const = 1,
  rap_ctor = 2,
  rap_dtor = 3,
  rap_data = 4,
  rap_bss = 5,
  rap_secs = 6
};

/**
 * Object details.
 */
typedef struct
{
  char* name;
  uint32_t offset;
  uint32_t size;
  uint32_t rap_id;
}section_detail;

struct link_map {
  char*             name;                 /**< Name of the obj. */
  uint32_t          sec_num;              /**< The count of section. */
  section_detail*   sec_detail;           /**< The section details. */
  uint32_t*         sec_addr[rap_secs];   /**< The RAP section addr. */
  uint32_t          rpathlen;             /**< The length of the path. */
  char*             rpath;                /**< The path of object files. */
  struct link_map*  l_next;               /**< Linked list of mapped libs. */
  struct link_map*  l_prev;
};

struct r_debug {
	int r_version;			      /* not used */
	struct link_map *r_map;		/* list of loaded images */
	enum {
		RT_CONSISTENT,		      /* things are stable */
		RT_ADD,			            /* adding a shared library */
		RT_DELETE		            /* removing a shared library */
	} r_state;
};

#endif	/* _LINK_ELF_H_ */
