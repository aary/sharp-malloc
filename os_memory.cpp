#include <utility>
#include <cassert>
#include <algorithm>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "os_memory.hpp"

namespace eecs281 {

namespace {

    /**
     * Constant to determine the minimum memory that can be requested from the
     * operating system, this is done to batch requests to the OS since a
     * typical system call requires a lot of overhead.  Although this library
     * does not assert that the overhead is much larger than the overhead in
     * the actual malloc(3) call itself ¯\_(ツ)_/¯
     */
    const auto MINIMUM_BATCH = getpagesize();

    /**
     * Rounds up the value passed to be a multiple of the page size of the local
     * system, consult the manual pages for mmap(2) for more details about why
     * this could be important
     */
    int round_up_to_page_boundary(int amount);

    /**
     * Rounds up the amount of memory to request to a value such that it has
     * to be larger than the requested size by the maximum alignment of the
     * system, so that if the returned memory is not properly aligned then the
     * pointer can be incremented by 1 byte until the base pointer is aligned
     * properly
     */
    int round_up_account_for_misalignment(int amount);

} // namespace <anonymous>


std::pair<void*, int> extend_heap(int amount_of_memory) {
    assert(amount_of_memory);

    // The actual amount of memory that is going to be requested from the
    // operating system from the mmap call, this has to be a multiple of the
    // page boundary since we want to be good memory citizens (consult the
    // local manual pages for mmap(2) for more details)
    auto actual_amount = std::min(amount_of_memory, MINIMUM_BATCH);
    actual_amount = round_up_account_for_misalignment(actual_amount);
    actual_amount = round_up_to_page_boundary(actual_amount);
    assert(!(MINIMUM_BATCH % actual_amount));

    // then request the memory from the operating system, error check and
    // throw an exception with a string error message as fetched from the
    // errono thread local
    auto memory = mmap(nullptr, actual_amount, PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    // for some reason bad_alloc does not inherit from a runtime error
    // even though all POSIX systems set a readable string value via errno
    // to alert the user of the exact error that occured in the failed
    // allocation (presumably through mmap)
    if (memory == MAP_FAILED) {
        throw std::bad_alloc{};
    }

    assert(is_boundary_aligned(memory));
    return std::make_pair(memory, actual_amount);
}


namespace {

    /**
     * A very efficient implementation of rounding up to the nearest page
     * boundary, the amount of memory requested by mmap has to be a multiple of
     * the page size
     */
    int round_up_to_page_boundary(int amount) {
        return amount + (amount % getpagesize());
    }

    int round_up_account_for_misalignment(int amount) {
        // add the alignment to the returned value so that if the pointer is
        // not aligned properly then it can be incremented until the alignment
        // is ok, although most sytem mmap implementations should enforce this
        // by default
        //
        // an example to explain this would be that you need to allocate 10
        // bytes, the maximum alignment is 8 bytes and the pointer returned
        // was at address 1; in that case if you had allocated 18 bytes
        // instead, then you could just use the memory starting at location 8
        // and use the 10 bytes from there even if the actual memory that was
        // returned was at location 1, we would be wasting 7 bytes, but this
        // is the simplest solution
        return amount + alignof(std::max_align_t);
    }
} // namespace <anonymous>


} // namespace eecs281
