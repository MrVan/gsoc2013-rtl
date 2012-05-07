/*
 *  COPYRIGHT (c) 2012 Chris Johns <chrisj@rtems.org>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */
/**
 * @file
 *
 * @ingroup rtems_rtl
 *
 * @brief RTEMS Run-Time Linker Allocator for the standard heap.
 */

#if !defined (_RTEMS_RTL_ALLOC_HEAP_H_)
#define _RTEMS_RTL_ALLOC_HEAP_H_

#include <rtl-allocator.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Allocator handler for the standard libc heap.
 *
 * @param allocation If true the request is to allocate memory else free.
 * @param tag The type of allocation request.
 * @param address Pointer to the memory address. If an allocation the value is
 *                unspecific on entry and the allocated address or NULL on
 *                exit. The NULL value means the allocation failed. If a delete
 *                or free request the memory address is the block to free. A
 *                free request of NULL is silently ignored.
 * @param size The size of the allocation if an allocation request and
 *             not used if deleting or freeing a previous allocation.
 */
void rtems_rtl_alloc_heap(bool                  allocate,
                          rtems_rtl_alloc_tag_t tag,
                          void**                address,
                          size_t                size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
