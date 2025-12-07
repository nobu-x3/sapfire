#include "render/vulkan/vk_descriptor_pool.h"
#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "render/vulkan/vk_descriptor_set.h"
#include "render/vulkan/vk_graphics_device.h"

namespace sf::render::vk {

    VulkanDescriptorPool::VulkanDescriptorPool(VulkanGraphicsDevice* device, const DescriptorPoolDesc& desc) : m_Device(device) {
        // Build pool sizes array
        stl::vector<VkDescriptorPoolSize> pool_sizes{mem::MemTag::Temp};
        pool_sizes.reserve(6);
        if (desc.max_uniform_buffers > 0) {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, desc.max_uniform_buffers});
        }
        if (desc.max_storage_buffers > 0) {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, desc.max_storage_buffers});
        }
        if (desc.max_sampled_images > 0) {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, desc.max_sampled_images});
        }
        if (desc.max_storage_images > 0) {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, desc.max_storage_images});
        }
        if (desc.max_samplers > 0) {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, desc.max_samplers});
        }
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        pool_info.maxSets = desc.max_sets;
        pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        VkResult result = vkCreateDescriptorPool(m_Device->get_vk_device(), &pool_info, nullptr, &m_Pool);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create descriptor pool: {}", static_cast<i32>(result));
        }
    }

    VulkanDescriptorPool::~VulkanDescriptorPool() {
        if (m_Pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_Device->get_vk_device(), m_Pool, nullptr);
        }
    }

    stl::result<stl::unique_ptr<IDescriptorSet>> VulkanDescriptorPool::allocate_set(const DescriptorSetLayout& layout) {
        auto descriptor_set = stl::make_unique<VulkanDescriptorSet>(mem::MemTag::Render, m_Device, this, layout);
        return stl::result<stl::unique_ptr<IDescriptorSet>>(std::move(descriptor_set));
    }

    void VulkanDescriptorPool::reset() { vkResetDescriptorPool(m_Device->get_vk_device(), m_Pool, 0); }

} // namespace sf::render::vk
