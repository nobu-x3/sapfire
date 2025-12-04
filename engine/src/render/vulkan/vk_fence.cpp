#include "render/vulkan/vk_fence.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_graphics_device.h"

namespace sf::render::vk {
    VulkanFence::VulkanFence(VulkanGraphicsDevice* device, bool signaled, const char* name) :
        m_Device(device), m_Fence(VK_NULL_HANDLE), m_Name(name) {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        VkResult result = vkCreateFence(m_Device->get_vk_device(), &fence_info, nullptr, &m_Fence);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create Vulkan fence: {} - {}", m_Name, static_cast<i32>(result));
        }
        CORE_TRACE("Created Vulkan fence: {}", m_Name);
    }

    VulkanFence::~VulkanFence() {
        if (m_Fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_Device->get_vk_device(), m_Fence, nullptr);
            CORE_TRACE("Destroyed Vulkan fence: {}", m_Name);
        }
    }

    stl::result<> VulkanFence::wait(u64 timeout_ns) {
        VK_RETURN_ON_ERROR(vkWaitForFences(m_Device->get_vk_device(), 1, &m_Fence, VK_TRUE, timeout_ns),
                           "Failed to wait on fence {}", m_Name);
        return stl::success;
    }

    stl::result<> VulkanFence::reset() {
        VK_RETURN_ON_ERROR(vkResetFences(m_Device->get_vk_device(), 1, &m_Fence),
                           "Failed to reset fence {}", m_Name);
        return stl::success;
    }

    bool VulkanFence::is_signaled() const {
        VkResult result = vkGetFenceStatus(m_Device->get_vk_device(), m_Fence);
        return result == VK_SUCCESS;
    }

    VulkanSemaphore::VulkanSemaphore(VulkanGraphicsDevice* device, const char* name) :
        m_Device(device), m_Semaphore(VK_NULL_HANDLE), m_Name(name) {
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkResult result = vkCreateSemaphore(m_Device->get_vk_device(), &semaphore_info, nullptr, &m_Semaphore);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create Vulkan semaphore: {} - {}", m_Name, static_cast<i32>(result));
        }
        CORE_TRACE("Created Vulkan semaphore: {}", m_Name);
    }
    
    VulkanSemaphore::~VulkanSemaphore() {
        if (m_Semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_Device->get_vk_device(), m_Semaphore, nullptr);
            CORE_TRACE("Destroyed Vulkan semaphore: {}", m_Name);
        }
    }
} // namespace sf::render::vk
