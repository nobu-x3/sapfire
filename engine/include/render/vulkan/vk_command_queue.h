#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "render/i_command_queue.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;

    class VulkanCommandQueue final : public ICommandQueue {
    public:
        explicit VulkanCommandQueue(VkDevice device, VkQueue queue, CommandQueueType type, const char* name);
        explicit VulkanCommandQueue(VulkanGraphicsDevice* graphics_device, VkDevice device, VkQueue queue, CommandQueueType type,
                                    const char* name);

        void destroy_resources();

        stl::result<> submit(const sf::render::QueueSubmitDesc& submit_desc) override;
        stl::result<> submit_immediate(IContext* context) override;
        stl::result<> wait_for_idle() override;
        void* get_native_queue() override;

        CommandQueueType get_type() const override { return m_Type; }

        VkQueue get_vk_queue() const { return m_Queue; }
        VkFence get_vk_fence() const { return m_Fence; }
        bool is_fence_complete(u64 fence_value) const;

    private:
        VulkanGraphicsDevice* m_GraphicsDevice{nullptr};
        VkDevice m_Device{VK_NULL_HANDLE};
        VkQueue m_Queue{VK_NULL_HANDLE};
        VkFence m_Fence{VK_NULL_HANDLE};
        u64 m_FenceValue = 0;
        CommandQueueType m_Type;
    };

} // namespace sf::render::vk
