#include "engpch.h"

#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "render/vulkan/vk_command_queue.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_fence.h"
#include "render/vulkan/vk_graphics_device.h"

namespace sf::render::vk {
    VulkanCommandQueue::VulkanCommandQueue(VkDevice device, VkQueue queue, CommandQueueType type, const char* name) :
        m_GraphicsDevice(nullptr), m_Device(device), m_Queue(queue), m_Type(type) {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_Device, &fence_info, nullptr, &m_Fence);
        CORE_INFO("Created Vulkan command queue: {}", name);
    }

    VulkanCommandQueue::VulkanCommandQueue(VulkanGraphicsDevice* graphics_device, VkDevice device, VkQueue queue, CommandQueueType type,
                                           const char* name) :
        m_GraphicsDevice(graphics_device), m_Device(device), m_Queue(queue), m_Type(type) {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_Device, &fence_info, nullptr, &m_Fence);
        CORE_INFO("Created Vulkan command queue: {}", name);
    }

    VulkanCommandQueue::~VulkanCommandQueue() { destroy_resources(); }

    void VulkanCommandQueue::destroy_resources() {
        if (m_Device != VK_NULL_HANDLE && m_Fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_Device, m_Fence, nullptr);
            m_Fence = VK_NULL_HANDLE;
        }
        m_Device = VK_NULL_HANDLE;
    }

    stl::result<> VulkanCommandQueue::wait_for_idle() {
        VK_RETURN_ON_ERROR(vkQueueWaitIdle(m_Queue), "Failed to wait for queue idle");
        return stl::success;
    }

    bool VulkanCommandQueue::is_fence_complete(u64 fence_value) const {
        if (fence_value > m_FenceValue) {
            return false;
        }
        return vkGetFenceStatus(m_Device, m_Fence) == VK_SUCCESS;
    }

    stl::result<> VulkanCommandQueue::submit(const QueueSubmitDesc& submit_desc) {
        stl::vector<VkCommandBuffer> command_buffers(mem::MemTag::Temp);
        command_buffers.reserve(submit_desc.command_contexts.size());
        for (IContext* context : submit_desc.command_contexts) {
            auto* vk_ctx = dynamic_cast<VulkanContext*>(context);
            if (!vk_ctx) {
                return stl::make_error("Failed to cast to VulkanContext* when submitting command buffers in VulkanCommandQueue.");
            }
            command_buffers.push_back(vk_ctx->get_vk_command_buffer());
        }
        stl::vector<VkSemaphore> wait_semaphores(mem::MemTag::Temp);
        stl::vector<VkPipelineStageFlags> wait_stages(mem::MemTag::Temp);
        for (ISemaphore* semaphore : submit_desc.wait_semaphores) {
            auto* vk_semaphore = dynamic_cast<VulkanSemaphore*>(semaphore);
            if (!vk_semaphore) {
                return stl::make_error("Failed to cast to VulkanSemaphore* when submitting wait semaphores in VulkanCommandQueue.");
            }
            wait_semaphores.push_back(vk_semaphore->get_vk_semaphore());
            wait_stages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }
        stl::vector<VkSemaphore> signal_semaphores(mem::MemTag::Temp);
        for (ISemaphore* semaphore : submit_desc.signal_semaphores) {
            auto* vk_semaphore = dynamic_cast<VulkanSemaphore*>(semaphore);
            if (!vk_semaphore) {
                return stl::make_error("Failed to cast to VulkanSemaphore* when submitting signal semaphores in VulkanCommandQueue.");
            }
            signal_semaphores.push_back(vk_semaphore->get_vk_semaphore());
        }
        VkFence fence = VK_NULL_HANDLE;
        if (submit_desc.signal_fence) {
            auto* vk_fence = dynamic_cast<VulkanFence*>(submit_desc.signal_fence);
            if (!vk_fence) {
                return stl::make_error("Failed to cast to VulkanFence* when submitting signal fences in VulkanCommandQueue.");
            }
            fence = vk_fence->get_vk_fence();
        }
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = static_cast<u32>(wait_semaphores.size());
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.commandBufferCount = static_cast<u32>(command_buffers.size());
        submit_info.pCommandBuffers = command_buffers.data();
        submit_info.signalSemaphoreCount = static_cast<u32>(signal_semaphores.size());
        submit_info.pSignalSemaphores = signal_semaphores.data();
        VK_RETURN_ON_ERROR(vkQueueSubmit(m_Queue, 1, &submit_info, fence), "Failed to submit to queue");
        return stl::success;
    }

    stl::result<> VulkanCommandQueue::submit_immediate(IContext* context) {
        if (!context) {
            return stl::make_error<>("VulkanCommandQueue::submit_immediate - context is null");
        }
        auto* vk_ctx = static_cast<VulkanContext*>(context);
        VkCommandBuffer command_buffer = vk_ctx->get_vk_command_buffer();
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        VK_RETURN_ON_ERROR(vkQueueSubmit(m_Queue, 1, &submit_info, VK_NULL_HANDLE), "Failed to submit immediate");
        return stl::success;
    }

    void* VulkanCommandQueue::get_native_queue() { return reinterpret_cast<void*>(m_Queue); }
} // namespace sf::render::vk
