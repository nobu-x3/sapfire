#pragma once

#include <cassert>
#include <memory>
#include "memory/memory.h"
#include "core/core.h"

namespace sf::stl {

    template <typename T>
    using unique_ptr = std::unique_ptr<T, TaggedDeleter<T>>;

    template <typename T, typename... Args>
    unique_ptr<T> make_unique(mem::MemTag tag, Args&&... args) {
        auto* mm = mem::MemoryManager::get();
        assert(mm && "MemoryManager not initialized");

        auto& arena = mm->arena(tag);
        void* mem = arena.allocate(sizeof(T), alignof(T));
        T* ptr = new (mem) T(std::forward<Args>(args)...);

        return unique_ptr<T>(ptr, TaggedDeleter<T>(tag));
    }

} // namespace sf::stl
