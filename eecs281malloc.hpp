/**
 * @file eecs281malloc.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a simple malloc implementation similar to the one
 * described in "The C Programming Language" by Kernighan and Ritche.  Key
 * differences are in the langague of implementation and in the way the
 * features are implemented.  As opposed to following a C based approach with
 * manual implementations.  This malloc implementation uses a simple
 * transparent linked list abstraction to simplify the implementation.  C++
 * features like iterators and generic algorithms are used here to make this
 * process simpler
 *
 * The implemetation follows a simple first fit algorithm, the runtime looks
 * through the available memory on the heap to find the first fragmented data
 * block that is large enough to fit the user's request.  Subsequently after
 * the memory is returned to the user the implementation inserts the unused
 * portion of that memory in order of increasing address values, so that
 * freed coalescing blocks into one block can be easier and faster (O(log(n))
 * time).  In contrast a best fit algorithm would have faster allocations but
 * slower deletions and frees.  Unless a garbage collection strategy is
 * employed to periodically coalesce blocks where possible
 *
 * This allocator is meant to be simple, as such it does not maintain any
 * metadata more than the bare minimum that is required without sacrificing
 * code redability.  It does not guarantee things like thread safety and does
 * not try and optimize performance across cores by utilizing thread local
 * arenas.  If you are curious and want more information on the implementation
 * of Industry level allocators, then look into libraies such as jemalloc
 * located at https://github.com/jemalloc/jemalloc
 *
 * There are no security guarantees in place with this allocator either, as a
 * result it will be extremely easy for an attacker to exploit a buffer
 * overflow vulnerability in the application code.  This library does not
 * provide things like canaries at the end of the allocated region to shield
 * against that.  Nor does it fiddle with the protection bits of the memory
 * allocated from the operating system beyond the minimally required amount.
 *
 * This library uses assertions copiously to flag misalignments before they
 * can actually cause any problems, so if there is an assertion failure in the
 * library, it probably means that a pointer is either null when it should not
 * be or that it is not aligned to the maximum alignment set by the system
 * hardware
 */

#pragma once

#include <cstdint>
#include <algorithm>

namespace eecs281 {

/**
 * The allocator part of the drop in replacement for malloc(3) and free(3),
 * this can be wrapped around in an allocator class to make it compatible with
 * no weight for STL components.
 *
 * The pointer returned by malloc is aligned on the widest byte boundary that
 * is required by any fundamental type and any custom type that is built from
 * fundamental types.  Extended alignment (for example with alignas) is not
 * supported by this allocator alone.  The maximum alignment requirement (or
 * the minumum alignment requirement that will not cause a fault) is
 * determined by the alignment of std::max_align_t
 *
 * @param amount_of_memory the amount of memory in bytes that have to be
 *        allocated, for example, if you want to allocate 4 integers each 4
 *        bytes wide, then you would call malloc with 4*4 as the
 *        amount_of_memory parameter.  Note that the type of the parameter is
 *        an unsigned integer because of compatibility with regular malloc(3)
 *        implemetations
 */
void* malloc(int amount);

/**
 * The free function associated with the malloc function above, this works
 * only with the malloc above, using it with any other memory allocator is
 * *undefined behavior*, use with caution
 */
void free(void* pointer_to_free);

} // namespace eecs281
