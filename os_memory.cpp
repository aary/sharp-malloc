#include <utility>
#include <cassert>
#include <algorithm>
#include <cstddef>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "os_memory.hpp"

using std::max_align_t;

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

} // namespace <anonymous>


std::pair<void*, int> extend_heap(int amount_of_memory) {
    // assert that the amount of memory requested is aligned on the maximum
    // alignment restriction of the system
    assert(amount_of_memory);
    assert(!(amount_of_memory % alignof(max_align_t)));

    // The actual amount of memory that is going to be requested from the
    // operating system from the mmap call, this has to be a multiple of the
    // page boundary since we want to be good memory citizens (consult the
    // local manual pages for mmap(2) for more details)
    auto actual_amount = std::max(amount_of_memory, MINIMUM_BATCH);
    actual_amount += actual_amount % MINIMUM_BATCH;

    // then request the memory from the operating system, error check and
    // throw an exception with a string error message as fetched from the
    // errono thread local
    auto memory = mmap(nullptr, actual_amount, PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    // for some reason bad_alloc does not inherit from a runtime error
    // even though all POSIX systems set a readable string value via errno
    // to alert the user of the exact error that occured in the failed
    // allocation (presumably through mmap)
    //
    // tl;dr I think that the design of bad_alloc should be extended to
    // inherit from std::runtime_error rather than directly from
    // std::exception, since the system can include error messages
    if (memory == MAP_FAILED) {
        throw std::bad_alloc{};
    }

    // the c++ standard does not say anything about the resulting value of
    // reinterpret_cast<uintptr_t(memory) but cpprefernece does and at one
    // point the compiler needs to be trusted for bitwise conversions like
    // this so ¯\_(ツ)_/¯
    assert(memory);
    assert(!(reinterpret_cast<uintptr_t>(memory) % alignof(max_align_t)));
    return std::make_pair(memory, actual_amount);
}

} // namespace eecs281
