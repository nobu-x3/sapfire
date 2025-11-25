#include "render/vulkan/vk_command_queue.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_context.h"

namespace sf::render::vk {
    VkCommandQueue::VkCommandQueue(VkDevice device, VkQueue queue, CommandQueueType type, const char* name) :
        m_Device(device), m_Queue(queue), m_Type(type) {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_Device, &fence_info, nullptr, &m_Fence);
        CORE_INFO("Created Vulkan command queue: {}", name);
    }

    void VkCommandQueue::execute_command_lists(IContext** contexts, u32 count) {
        stl::vector<VkCommandBuffer> command_buffers{mem::MemTag::Temp, count};
        for (u32 i = 0; i < count; ++i) {
            auto* vk_ctx = static_cast<VkContext*>(contexts[i]);
            command_buffers.push_back(vk_ctx->get_vk_command_buffer());
        }
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = count;
        submit_info.pCommandBuffers = command_buffers.data();
        vkQueueSubmit(m_Queue, 1, &submit_info, VK_NULL_HANDLE);
    }

    void VkCommandQueue::execute_command_list(IContext* context) { execute_command_lists(&context, 1); }
    u64 VkCommandQueue::signal() {
        ++m_FenceValue;
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vkQueueSubmit(m_Queue, 1, &submit_info, m_Fence);
        return m_FenceValue;
    }

    void VkCommandQueue::wait_for_fence_value(u64 fence_value) {
        if (fence_value > m_FenceValue) {
            return;
        }
        vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
    }

    void VkCommandQueue::wait_for_idle() { vkQueueWaitIdle(m_Queue); }
    u64 VkCommandQueue::get_last_completed_fence_value() const {
        VkResult result = vkGetFenceStatus(m_Device, m_Fence);
        if (result == VK_SUCCESS) {
            return m_FenceValue;
        }
        return m_FenceValue - 1;
    }

    bool VkCommandQueue::is_fence_complete(u64 fence_value) const {
        if (fence_value > m_FenceValue) {
            return false;
        }
        return vkGetFenceStatus(m_Device, m_Fence) == VK_SUCCESS;
    }
} // namespace sf::render::vk
