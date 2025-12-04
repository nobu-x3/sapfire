#pragma once

#include <span>
#include "core/core.h"
#include "render_api.h"

namespace sf::render {

    class IContext;
    class ISemaphore;
    class IFence;

    // Queue Submit Description

    struct QueueSubmitDesc {
        stl::span<ISemaphore*> wait_semaphores;
        stl::span<IContext*> command_contexts;
        stl::span<ISemaphore*> signal_semaphores;
        IFence* signal_fence = nullptr;
    };

    // Command Queue Interface

    class ICommandQueue {
    public:
        virtual ~ICommandQueue() = default;

        // Execute command lists
        virtual stl::result<> submit(const QueueSubmitDesc& submit_desc) = 0;

        // Helper for simple submits (no sync)
        virtual stl::result<> submit_immediate(IContext* context) = 0;

        // Synchronization
        virtual stl::result<> wait_for_idle() = 0;

        // Queue type
        virtual CommandQueueType get_type() const = 0;

        // Backend-specific handle (for advanced usage)
        virtual void* get_native_queue() = 0;
    };

} // namespace sf::render
