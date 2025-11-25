#pragma once

#include "core/core.h"
#include "render_api.h"

namespace sf::render {

    class IContext;

    // Command Queue Interface

    class ICommandQueue {
    public:
        virtual ~ICommandQueue() = default;

        // Execute command lists
        virtual void execute_command_lists(IContext** contexts, u32 count) = 0;
        virtual void execute_command_list(IContext* context) = 0;

        // Synchronization
        virtual u64 signal() = 0;
        virtual void wait_for_fence_value(u64 fence_value) = 0;
        virtual void wait_for_idle() = 0;
        virtual u64 get_last_completed_fence_value() const = 0;

        // Queue type
        virtual CommandQueueType get_type() const = 0;

        // Backend-specific handle (for advanced usage)
        virtual void* get_native_handle() = 0;
    };

} // namespace sf::render
