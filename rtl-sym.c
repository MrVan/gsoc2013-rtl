/*
 *  COPYRIGHT (c) 2012 Chris Johns <chrisj@rtems.org>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */
/**
 * @file
 *
 * @ingroup rtems_rtl
 *
 * @brief RTEMS Run-Time Linker Object File Symbol Table.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include <rtl.h>
#include <rtl-error.h>
#include <rtl-sym.h>
#include <rtl-trace.h>

#include <sys/exec_elf.h>

/**
 * The single symbol forced into the global symbol table that is used to load a
 * symbol table from an object file.
 */
static rtems_rtl_obj_sym_t global_sym_add =
{
  .name  = "rtems_rtl_base_sym_global_add",
  .value = (void*) rtems_rtl_base_sym_global_add
};

static uint_fast32_t
rtems_rtl_symbol_hash (const char *s)
{
  uint_fast32_t h = 5381;
  unsigned char c;
  for (c = *s; c != '\0'; c = *++s)
    h = h * 33 + c;
  return h & 0xffffffff;
}

static void
rtems_rtl_symbol_global_insert (rtems_rtl_symbols_t* symbols,
                                rtems_rtl_obj_sym_t* symbol)
{
  uint_fast32_t hash = rtems_rtl_symbol_hash (symbol->name);
  rtems_chain_append (&symbols->buckets[hash % symbols->nbuckets],
                      &symbol->node);
}

bool
rtems_rtl_symbol_table_open (rtems_rtl_symbols_t* symbols,
                             size_t               buckets)
{
  symbols->buckets = rtems_rtl_alloc_new (RTEMS_RTL_ALLOC_SYMBOL,
                                          buckets * sizeof (rtems_chain_control),
                                          true);
  if (!symbols->buckets)
  {
    rtems_rtl_set_error (ENOMEM, "no memory for global symbol table");
    return false;
  }
  symbols->nbuckets = buckets;
  for (buckets = 0; buckets < symbols->nbuckets; ++buckets)
    rtems_chain_initialize_empty (&symbols->buckets[buckets]);
  rtems_rtl_symbol_global_insert (symbols, &global_sym_add);
  return true;
}

void
rtems_rtl_symbol_table_close (rtems_rtl_symbols_t* symbols)
{
  rtems_rtl_alloc_del (RTEMS_RTL_ALLOC_SYMBOL, symbols->buckets);
}

bool
rtems_rtl_symbol_global_add (rtems_rtl_obj_t*     obj,
                             const unsigned char* esyms,
                             unsigned int         size)
{
  rtems_rtl_symbols_t* symbols;
  rtems_rtl_obj_sym_t* sym;
  size_t               count;
  size_t               s;
  uint32_t             marker;

  count = 0;
  s = 0;
  while ((s < size) && (esyms[s] != 0))
  {
    int l = strlen ((char*) &esyms[s]);
    if ((esyms[s + l] != '\0') || ((s + l) > size))
    {
      rtems_rtl_set_error (EINVAL, "invalid exported symbol table");
      return false;
    }
    ++count;
#if defined(__align2__)
    s += ((l+1+3)&(~3)) + sizeof (unsigned long);
#else
    s += l + sizeof (unsigned long) + 1;
#endif
  }

  /*
   * Check this is the correct end of the table.
   */
#if defined(__align2__)
  s += 4;
  marker = esyms[s + 0];
  marker <<= 8;
  marker |= esyms[s + 1];
  marker <<= 8;
  marker |= esyms[s + 2];
  marker <<= 8;
  marker |= esyms[s + 3];
#else
  marker = esyms[s + 1];
  marker <<= 8;
  marker |= esyms[s + 2];
  marker <<= 8;
  marker |= esyms[s + 3];
  marker <<= 8;
  marker |= esyms[s + 4];
#endif

  if (marker != 0xdeadbeefUL)
  {
    rtems_rtl_set_error (ENOMEM, "invalid export symbol table");
    return false;
  }

  if (rtems_rtl_trace (RTEMS_RTL_TRACE_GLOBAL_SYM))
    printf ("rtl: global symbol add: %zi\n", count);

  obj->global_size = count * sizeof (rtems_rtl_obj_sym_t);
  obj->global_table = rtems_rtl_alloc_new (RTEMS_RTL_ALLOC_SYMBOL,
                                           obj->global_size, true);
  if (!obj->global_table)
  {
    obj->global_size = 0;
    rtems_rtl_set_error (ENOMEM, "no memory for global symbols");
    return false;
  }

  symbols = rtems_rtl_global_symbols ();

  s = 0;
  sym = obj->global_table;

  while ((s < size) && (esyms[s] != 0))
  {
    /*
     * Copy the void* using a union and memcpy to avoid any strict aliasing or
     * alignment issues. The variable length of the label and the packed nature
     * of the table means casting is not suitable.
     */
    union {
      uint8_t data[sizeof (void*)];
      void*   value;
    } copy_voidp;
    int b;

    sym->name = (const char*) &esyms[s];
#if defined(__align2__)
    s += ((strlen(sym->name) + 1 + 3) & (~3));
#else
    s += strlen (sym->name) + 1;
#endif
    for (b = 0; b < sizeof (void*); ++b, ++s)
      copy_voidp.data[b] = esyms[s];
    sym->value = copy_voidp.value;
    if (rtems_rtl_trace (RTEMS_RTL_TRACE_GLOBAL_SYM))
      printf ("rtl: esyms: %s -> %8p\n", sym->name, sym->value);
    if (rtems_rtl_symbol_global_find (sym->name) == NULL)
      rtems_rtl_symbol_global_insert (symbols, sym);
    ++sym;
  }

  obj->global_syms = count;

  return true;
}

rtems_rtl_obj_sym_t*
rtems_rtl_symbol_obj_find_internal (rtems_rtl_obj_t* obj, const char* name,
    uint32_t index, uint32_t symbinding)
{
  rtems_rtl_obj_sym_t* sym;
	size_t               s;
  /*
	 * Check the object file's symbols first. If not found search the
   * * global symbol table.
	 */
  for (s = 0, sym = obj->global_table; s < obj->global_syms; ++s, ++sym) {
    if (symbinding == STB_LOCAL) {
      if ((strcmp (name, sym->name) == 0) && (sym->index == index))	
        return sym;
    } else if (strcmp (name, sym->name) == 0)
      return sym;
  }

  if (symbinding == STB_LOCAL)
    return NULL;
  return rtems_rtl_symbol_global_find (name);
}

rtems_rtl_obj_sym_t*
rtems_rtl_symbol_global_find (const char* name)
{
  rtems_rtl_symbols_t* symbols;
  uint_fast32_t        hash;
  rtems_chain_control* bucket;
  rtems_chain_node*    node;

  symbols = rtems_rtl_global_symbols ();

  hash = rtems_rtl_symbol_hash (name);
  bucket = &symbols->buckets[hash % symbols->nbuckets];
  node = rtems_chain_first (bucket);

  while (!rtems_chain_is_tail (bucket, node))
  {
    rtems_rtl_obj_sym_t* sym = (rtems_rtl_obj_sym_t*) node;
    /*
     * Use the hash. I could add this to the symbol but it uses more memory.
     */
    if (strcmp (name, sym->name) == 0)
      return sym;
    node = rtems_chain_next (node);
  }

  return NULL;
}

rtems_rtl_obj_sym_t*
rtems_rtl_symbol_obj_find (rtems_rtl_obj_t* obj, const char* name)
{
  rtems_rtl_obj_sym_t* sym;
  size_t               s;
  /*
   * Check the object file's symbols first. If not found search the
   * global symbol table.
   */
  for (s = 0, sym = obj->global_table; s < obj->global_syms; ++s, ++sym)
    if (strcmp (name, sym->name) == 0)
      return sym;
  return rtems_rtl_symbol_global_find (name);
}

void
rtems_rtl_symbol_obj_add (rtems_rtl_obj_t* obj)
{
  rtems_rtl_symbols_t* symbols;
  rtems_rtl_obj_sym_t* sym;
  size_t               s;

  symbols = rtems_rtl_global_symbols ();

  for (s = 0, sym = obj->global_table; s < obj->global_syms; ++s, ++sym)
    rtems_rtl_symbol_global_insert (symbols, sym);
}

void
rtems_rtl_symbol_obj_erase (rtems_rtl_obj_t* obj)
{
  if (obj->global_table)
  {
    rtems_rtl_obj_sym_t* sym;
    size_t               s;
    for (s = 0, sym = obj->global_table; s < obj->global_syms; ++s, ++sym)
        if (!rtems_chain_is_node_off_chain (&sym->node))
          rtems_chain_extract (&sym->node);
    rtems_rtl_alloc_del (RTEMS_RTL_ALLOC_SYMBOL, obj->global_table);
    obj->global_table = NULL;
    obj->global_size = 0;
    obj->global_syms = 0;
  }
}
