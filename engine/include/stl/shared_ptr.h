#pragma once

#include <cassert>
#include <memory>
#include "core/core.h"
#include "memory/memory.h"

namespace sf::stl {

    template <typename T>
    using shared_ptr = std::shared_ptr<T>;

    template <typename T, typename... Args>
    shared_ptr<T> make_shared(mem::MemTag tag, Args&&... args) {
        auto* mm = mem::MemoryManager::get();
        assert(mm && "MemoryManager not initialized");

        // Use allocate_shared with custom allocator
        return std::allocate_shared<T>(mem::LinearTaggedAllocator<T>(&mm->arena(tag)), std::forward<Args>(args)...);
    }

} // namespace sf::stl
