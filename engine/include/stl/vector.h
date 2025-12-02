#pragma once

#include <cassert>
#include <vector>
#include "core/core.h"
#include "memory/memory.h"

namespace sf::stl {

    template <class T>
    class vector : public std::vector<T, mem::LinearTaggedAllocator<T>> {
    public:
        using base_type = std::vector<T, mem::LinearTaggedAllocator<T>>;
        using allocator_type = mem::LinearTaggedAllocator<T>;

        explicit vector(mem::MemTag tag) : base_type(allocator_type(&mem::MemoryManager::get()->arena(tag))) {
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        }

        vector(mem::MemTag tag, typename base_type::size_type count) :
            base_type(count, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        }

        vector(mem::MemTag tag, typename base_type::size_type count, const T& value) :
            base_type(count, value, allocator_type(&mem::MemoryManager::get()->arena(tag))) {
            assert(mem::MemoryManager::get() && "MemoryManager not initialized");
        }

        vector(const vector& other) : base_type(other) {}

        vector(vector&& other) noexcept : base_type(std::move(other)) {}

        vector& operator=(const vector& other) {
            base_type::operator=(other);
            return *this;
        }

        vector& operator=(vector&& other) noexcept {
            base_type::operator=(std::move(other));
            return *this;
        }

        using base_type::base_type;
    };

    template <class T>
    [[nodiscard]] inline vector<T> make_vector(mem::MemTag tag) {
        auto* mm = mem::MemoryManager::get();
        assert(mm && "MemoryManager not initialized");
        return vector<T>(mem::LinearTaggedAllocator<T>(&mm->arena(tag)));
    }

} // namespace sf::stl
