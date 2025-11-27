#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "render/i_command_queue.h"

namespace sf::render::vk {

    class VkCommandQueue : public ICommandQueue {
    public:
        explicit VkCommandQueue(VkDevice device, VkQueue queue, CommandQueueType type, const char* name);
        VkCommandQueue() = default;
        ~VkCommandQueue() override;

        void destroy_resources();

        void execute_command_lists(IContext** contexts, u32 count) override;
        void execute_command_list(IContext* context) override;

        u64 signal() override;
        void wait_for_fence_value(u64 fence_value) override;
        void wait_for_idle() override;
        u64 get_last_completed_fence_value() const override;

        CommandQueueType get_type() const override { return m_Type; }
        void* get_native_handle() override { return reinterpret_cast<void*>(m_Queue); }

        VkQueue get_vk_queue() const { return m_Queue; }
        VkFence get_vk_fence() const { return m_Fence; }
        bool is_fence_complete(u64 fence_value) const;

    private:
        VkDevice m_Device{VK_NULL_HANDLE};
        VkQueue m_Queue{VK_NULL_HANDLE};
        VkFence m_Fence{VK_NULL_HANDLE};
        u64 m_FenceValue = 0;
        CommandQueueType m_Type;
    };

} // namespace sf::render::vk
