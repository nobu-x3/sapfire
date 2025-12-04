#pragma once

#include "core/core.h"

namespace sf::render {

    // Fence Interface
    // Used for CPU-GPU synchronization

    class IFence {
    public:
        virtual ~IFence() = default;

        // Wait for fence to be signaled
        virtual stl::result<> wait(u64 timeout_ns = UINT64_MAX) = 0;

        // Reset fence to unsignaled state
        virtual stl::result<> reset() = 0;

        // Check if fence is signaled
        virtual bool is_signaled() const = 0;

        // Backend-specific handle
        virtual void* get_native_fence() = 0;
    };

    // Semaphore Interface
    // Used for GPU-GPU synchronization (between queues, swapchain)

    class ISemaphore {
    public:
        virtual ~ISemaphore() = default;

        // Backend-specific handle
        virtual void* get_native_semaphore() = 0;
    };

} // namespace sf::render
