#pragma once

#include "../containers/no_init_vector.hpp"
#include "../types/concepts.hpp"
#include <cstddef>
#include <cstring>
#include <ranges>
#include <utility>

namespace cpp_utils::io
{

/** Owning counterpart to buffer_view: holds a no_init_vector<char> by value and
 * exposes the same random_access_buffer shape (read/view/size/is_valid), for buffers
 * whose lifetime must outlive whatever produced them — e.g. a freshly-decompressed
 * payload that has nowhere else to live. */
struct owned_buffer
{
    containers::no_init_vector<char> storage;

    owned_buffer() = default;
    explicit owned_buffer(containers::no_init_vector<char>&& storage)
            : storage { std::move(storage) }
    {
    }

    auto begin() const { return storage.data(); }
    auto end() const { return storage.data() + storage.size(); }

    auto view(const std::size_t offset) const { return storage.data() + offset; }

    void read(char* dest, const std::size_t offset, const std::size_t size) const
    {
        std::memcpy(dest, storage.data() + offset, size);
    }

    auto data() const { return storage.data(); }
    std::size_t size() const { return storage.size(); }

    bool is_valid() const { return storage.data() != nullptr; }
};

static_assert(std::ranges::contiguous_range<owned_buffer>);
static_assert(types::concepts::random_access_buffer<owned_buffer>);

}
