#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <limits>
#include <utility>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "TransparentList.hpp"
#include "eecs281malloc.hpp"
#include "os_memory.hpp"

using std::uintptr_t;
using std::max_align_t;

namespace eecs281 {

namespace {

    /**
     * Typedefs for the list and header
     */
    using FreeList_t = TransparentList<int>;
    using Header_t = TransparentNode<int>;

    /**
     * assert that the alignment of the header is the maximum alignment of
     * the system
     */
    static_assert(alignof(Header_t) == alignof(max_align_t),
            "Cannot work with a header class that is not aligned to the right "
            "system boundary");

    /**
     * A concept that enables an overload only if the type is an integral
     * type, this has to be used in a SFINAE context with it set either as a
     * template parameter value pointer, a template parameter value, a
     * function call parameter value or a return type
     */
    template <typename Type>
    using EnableIfIntegral = std::enable_if_t<std::is_integral<Type>::value>;

    /**
     * A singleton that contains the state required by the memory allocator
     */
    FreeList_t free_list;

    /**
     * Constructs a header starting at address address and extending till the
     * location as specified by amount and returns the aligned pointer to the
     * header
     *
     * Fails with an abort if either address or amount are not divisible by
     * the maximum alignment on the system
     *
     * @param address The address at which to construct the header
     * @param amount The amount of memory that will be taken up by the block
     *        following the header
     *
     * @return returns a pointer to the formed header in the memory address
     *         specified.  If the header can not fit in the memory location
     *         given then the function returns a nullptr
     */
    Header_t* make_header(void* address, int amount);

    /**
     * Removes the requested memory from the header and returns a pointer to
     * whatever was left in the new header, if nothing is left, then this
     * returns a nullptr
     *
     * Returns the same pointer if it has enough memory to fit the amount but
     * not more than the amount requested.  Returns a pointer that is not
     * equal to the original pointer if there is enough space in the header to
     * accomodate another header if possible
     *
     * @param header_ptr a pointer to the header from which you want to remove
     *        memory, another header that is amount + sizeof(Header_t) bytes
     *        from the passed header will be returned if there is memory for
     *        it
     *
     * @return If the header cannot be used for the amount of bytes given in
     *         the second parameter then a nullptr will be returned.  If there
     *         is just enough memory in the header then the same pointer will
     *         be returned
     */
    Header_t* remove_memory(Header_t* header_ptr, int amount);

    /**
     * Coalesces two blocks and then returns the coalesced block to the user,
     * if they cannot be coalesced then this function returns a nullptr
     *
     * @param header_one the first header to be coalesced
     * @param header_two the second header to be coalesced
     *
     * @return returns a pointer to the header formed by coalescing the two
     *         headers in the parameter pack, if they cannot be coalesced then
     *         it returns a nullptr
     */
    Header_t* coalesce(Header_t* header_one, Header_t* header_two);

    /**
     * Asserts the alignment of the passed in pointer value.  The maximum
     * alignment is determined by the alignment of the library type provided
     * with the maximum primitive alignment value (std::max_align_t)
     *
     * This function can be used both with pointers as well as integral
     * values.  The two overloads take care of seeing whether the pointer or
     * the integer are divisible by the maximum alignment on the system, there
     * is no good way in C++ to cast both a pointer or a signed integer to an
     * unsigned value, reinterpret_cast is only used for bit-wise conversions
     * and static_cast does not work on pointers
     */
    template <typename Type>
    bool boundary_aligned(Type* pointer) {
        return !(reinterpret_cast<uintptr_t>(pointer) % alignof(max_align_t));
    }

    template <typename IntegralType, EnableIfIntegral<IntegralType>* = nullptr>
    bool boundary_aligned(IntegralType integer) {
        return !(integer % alignof(max_align_t));
    }

} // namespace <anonymous>


void* malloc(std::size_t amount_in) {
    // assert that the amount of memory is not greater than the maximum
    // integer on the system, this is done for safety because apart from this
    // function no other function in this module uses unsigned integers
    assert(amount_in <= std::numeric_limits<int>::max());
    // auto amount = static_cast<int>(amount_in);
    remove_memory(nullptr, 0);
    coalesce(nullptr, nullptr);
    return nullptr;
}

void free(void*) {
}


namespace {

    Header_t* make_header(void* address, int amount) {
        assert(boundary_aligned(address));
        assert(boundary_aligned(amount));
        assert(boundary_aligned(sizeof(Header_t)));

        // if the header cannot serve any memory request then return a nullptr
        // to indicate that the header is not suitable for usage
        if (amount <= static_cast<int>(sizeof(Header_t))) {
            return nullptr;
        }

        // set the size variable in the header to be the previous size minus
        // the amount that is needed for the header, this will still result in
        // the address range being aligned since the node type's alignment has
        // been set to the maximum alignment on the system (i.e.
        // alignof(std::max_align_t)
        auto new_size = static_cast<int>(amount - sizeof(Header_t));
        auto header_ptr = new(address) Header_t{new_size};
        return header_ptr;;
    }

    Header_t* remove_memory(Header_t* header_ptr, int amount) {
        assert(header_ptr);
        assert(boundary_aligned(header_ptr));
        assert(boundary_aligned(amount));
        assert(boundary_aligned(header_ptr->datum));

        // if the header does not contain enough memory for the amount to be
        // reduced then return nullptr
        if (header_ptr->datum < amount) {
            return nullptr;
        }

        // attempt to create another header from the current header, if there
        // is enough memory for another header then one will be created
        auto new_header = make_header(reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(header_ptr + 1) + amount),
                header_ptr->datum - amount);
        if (new_header) {
            assert(boundary_aligned(new_header));
            assert(boundary_aligned(new_header->datum));
            return new_header;
        }

        // else return the same pointer to indicate that it can serve the
        // request but not more than it
        return header_ptr;
    }

    Header_t* coalesce(Header_t* header_one, Header_t* header_two) {
        // assert a bunch of things
        assert(header_one);
        assert(header_two);
        assert(boundary_aligned(header_one));
        assert(boundary_aligned(header_two));
        assert(header_one != header_two);
        assert(boundary_aligned(header_one->datum));
        assert(boundary_aligned(header_two->datum));

        // assign the lesser of the two to be the min_header, since we need to
        // coalesce the blocks, and we don't care about the order in which the
        // blocks are given
        auto min_header = std::min(header_one, header_two);
        auto max_header = std::max(header_one, header_two);

        // if the max one is immediately after the lesser one, then coalesce
        // them and return the pointer to the coalesced block
        if (reinterpret_cast<uintptr_t>(min_header + 1) + min_header->datum
                == reinterpret_cast<uintptr_t>(max_header)) {
            min_header->datum += sizeof(Header_t);
            min_header->datum += max_header->datum;
            return min_header;
        }

        return nullptr;
    }

} // namespace <anonymous>

} // namespace eecs281

