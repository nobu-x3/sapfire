#pragma once

#include <vulkan/vulkan.h>
#include "render/i_fence.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;

    class VulkanFence final : public IFence {
    public:
        VulkanFence(VulkanGraphicsDevice* device, bool signaled, const char* name);
        ~VulkanFence() override;

        // IFence interface
        stl::result<> wait(u64 timeout_ns = UINT64_MAX) override;
        stl::result<> reset() override;
        bool is_signaled() const override;
        void* get_native_fence() override { return reinterpret_cast<void*>(m_Fence); }

        // Vulkan-specific accessors
        VkFence get_vk_fence() const { return m_Fence; }

    private:
        VulkanGraphicsDevice* m_Device;
        VkFence m_Fence;
        const char* m_Name;
    };

    class VulkanSemaphore final : public ISemaphore {
    public:
        VulkanSemaphore(VulkanGraphicsDevice* device, const char* name);
        ~VulkanSemaphore() override;

        // ISemaphore interface
        void* get_native_semaphore() override { return reinterpret_cast<void*>(m_Semaphore); }

        // Vulkan-specific accessors
        VkSemaphore get_vk_semaphore() const { return m_Semaphore; }

    private:
        VulkanGraphicsDevice* m_Device;
        VkSemaphore m_Semaphore; // Binary semaphore
        const char* m_Name;
    };

} // namespace sf::render::vk
