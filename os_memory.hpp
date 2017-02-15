/**
 * @file os_memory.hpp
 * @author Aaryaman Sagar
 *
 * This file contains functions that interact with the underlying operating
 * system interface to allocate memory.
 *
 * Since the memory returned by the OS may or may not be aligned (mmap(2) does
 * not mention anything to that effect) this module can also be used to align
 * memory and check for alignment
 */

#pragma once

#include <utility>

namespace eecs281 {

/**
 * Allocates a chunk of memory from the operating system.  The returned
 * memory contains either as much usable memory that was requested or more
 * than the requested memory, memory returned will never be less than the
 * amount that was returned.  However this means that the memory returned
 * will have to be defragmented by the application, as the amount of
 * memory returned can and often times is going to be larger than what was
 * requested.
 *
 * On error from the OS this function throws a std::bad_alloc exception to
 * alert the user
 *
 * The naming convention is made because of the logical similarities of this
 * fucntion with the OS memory interface employed in kernighan and ritche
 * (sbrk(2))
 *
 * @param amount_of_memory the amount of memory that is to be requested from
 *        the operating system in bytes.
 */
std::pair<void*, int> extend_heap(int amount_of_memory);

} // namespace eecs281

