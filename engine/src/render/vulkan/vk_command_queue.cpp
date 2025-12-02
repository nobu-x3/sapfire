#include "render/vulkan/vk_command_queue.h"
#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_graphics_device.h"

namespace sf::render::vk {
    VkCommandQueue::VkCommandQueue(VkDevice device, VkQueue queue, CommandQueueType type, const char* name) :
        m_GraphicsDevice(nullptr), m_Device(device), m_Queue(queue), m_Type(type) {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_Device, &fence_info, nullptr, &m_Fence);
        CORE_INFO("Created Vulkan command queue: {}", name);
    }

    VkCommandQueue::VkCommandQueue(VkGraphicsDevice* graphics_device, VkDevice device, VkQueue queue, CommandQueueType type, const char* name) :
        m_GraphicsDevice(graphics_device), m_Device(device), m_Queue(queue), m_Type(type) {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_Device, &fence_info, nullptr, &m_Fence);
        CORE_INFO("Created Vulkan command queue: {}", name);
    }

    void VkCommandQueue::destroy_resources() {
        if (m_Device != VK_NULL_HANDLE && m_Fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_Device, m_Fence, nullptr);
            m_Fence = VK_NULL_HANDLE;
        }
        m_Device = VK_NULL_HANDLE;
    }

    void VkCommandQueue::execute_command_lists(IContext** contexts, u32 count) {
        stl::vector<VkCommandBuffer> command_buffers{mem::MemTag::Temp, count};
        for (u32 i = 0; i < count; ++i) {
            auto* vk_ctx = static_cast<VkContext*>(contexts[i]);
            command_buffers[i] = vk_ctx->get_vk_command_buffer();
        }
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = count;
        submit_info.pCommandBuffers = command_buffers.data();
        vkQueueSubmit(m_Queue, 1, &submit_info, VK_NULL_HANDLE);
    }

    void VkCommandQueue::execute_command_list(IContext* context) {
        if (!context) {
            CORE_ERROR("VkCommandQueue::execute_command_list - context is null");
            return;
        }
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        if (auto* graphics_ctx = dynamic_cast<IGraphicsContext*>(context)) {
            auto* vk_graphics_ctx = static_cast<VkGraphicsContext*>(graphics_ctx);
            command_buffer = vk_graphics_ctx->get_vk_command_buffer();
        } else if (auto* compute_ctx = dynamic_cast<IComputeContext*>(context)) {
            auto* vk_compute_ctx = static_cast<VkComputeContext*>(compute_ctx);
            command_buffer = vk_compute_ctx->get_vk_command_buffer();
        } else if (auto* copy_ctx = dynamic_cast<ICopyContext*>(context)) {
            auto* vk_copy_ctx = static_cast<VkCopyContext*>(copy_ctx);
            command_buffer = vk_copy_ctx->get_vk_command_buffer();
        }
        if (command_buffer == VK_NULL_HANDLE) {
            CORE_ERROR("VkCommandQueue::execute_command_list - command buffer is null");
            return;
        }
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        vkQueueSubmit(m_Queue, 1, &submit_info, VK_NULL_HANDLE);
    }

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
