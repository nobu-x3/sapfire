#pragma once

#include <cassert>
#include <functional>
#include <map>
#include "memory/memory.h"
#include "core/core.h"

namespace sf::stl {

    template <class Key, class T, class Compare = std::less<Key>>
    class map : public std::map<Key, T, Compare, mem::PoolTaggedAllocator<std::pair<const Key, T>>> {
    public:
        using base_type = std::map<Key, T, Compare, mem::PoolTaggedAllocator<std::pair<const Key, T>>>;
        using allocator_type = mem::PoolTaggedAllocator<std::pair<const Key, T>>;

        explicit map(mem::MemTag tag) : base_type(Compare{}, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        }

        map(const map& other) : base_type(other) {}

        map(map&& other) noexcept : base_type(std::move(other)) {}

        map& operator=(const map& other) {
            base_type::operator=(other);
            return *this;
        }

        map& operator=(map&& other) noexcept {
            base_type::operator=(std::move(other));
            return *this;
        }

        using base_type::base_type;
    };

    template <class Key, class T, class Compare = std::less<Key>>
    [[nodiscard]] inline map<Key, T, Compare> make_map(mem::MemTag tag) {
        auto& mm = *mem::MemoryManager::get();
        assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        return map<Key, T, Compare>(Compare{}, mem::PoolTaggedAllocator<std::pair<const Key, T>>(&mm.arena(tag)));
    }

} // namespace sf::stl
