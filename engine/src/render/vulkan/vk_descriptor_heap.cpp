#include "engpch.h"

#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "render/vulkan/vk_descriptor_heap.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {
    VkDescriptorHeap::VkDescriptorHeap(VkDevice device, u32 descriptor_count, const char* name) :
        m_Device(device), m_DescriptorCount(descriptor_count) {
        stl::array<VkDescriptorPoolSize, 6> pool_sizes{{
            {VK_DESCRIPTOR_TYPE_SAMPLER, descriptor_count},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor_count},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptor_count},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptor_count},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_count},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor_count},
        }};
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 1;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_DescriptorPool);
        CORE_INFO("Created Vulkan descriptor heap: {}", name);
    }

    VkDescriptorHeap::~VkDescriptorHeap() {
        destroy_resources();
    }

    void VkDescriptorHeap::destroy_resources() {
        if(m_Device == VK_NULL_HANDLE)
            return;
        if (m_DescriptorSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_DescriptorSet);
            m_DescriptorSet = VK_NULL_HANDLE;
        }
        if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
            m_DescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
        m_Device = VK_NULL_HANDLE;
    }

    u32 VkDescriptorHeap::allocate_srv(Buffer& buffer) {
        u32 index = m_CurrentIndex++;
        // For bindless rendering, this would write a descriptor to the descriptor set at the allocated index
        // This requires a descriptor set layout with UPDATE_AFTER_BIND and descriptor arrays
        // Basic implementation - write descriptor for storage buffer
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        buffer_info.offset = 0;
        buffer_info.range = buffer.size_in_bytes;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 0; // Binding for storage buffers
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer_info;
        // Note: This requires the descriptor set to be created with the appropriate layout
        // For now, this is a placeholder that shows the structure
        CORE_WARN("VkDescriptorHeap::allocate_srv(Buffer) - descriptor set layout not fully configured");
        return index;
    }

    u32 VkDescriptorHeap::allocate_srv(Texture& texture, const ShaderResourceViewDesc* desc) {
        u32 index = m_CurrentIndex++;
        // For bindless rendering, this would write a sampled image descriptor
        VkDescriptorImageInfo image_info{};
        image_info.imageView = reinterpret_cast<VkImageView>(texture.resource); // Should use actual image view
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.sampler = VK_NULL_HANDLE; // For sampled images, not combined image samplers
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 1; // Binding for sampled images
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &image_info;
        CORE_WARN("VkDescriptorHeap::allocate_srv(Texture) - descriptor set layout not fully configured");
        return index;
    }

    u32 VkDescriptorHeap::allocate_uav(Buffer& buffer) {
        u32 index = m_CurrentIndex++;
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        buffer_info.offset = 0;
        buffer_info.range = buffer.size_in_bytes;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 2; // Binding for UAV storage buffers
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer_info;
        CORE_WARN("VkDescriptorHeap::allocate_uav(Buffer) - descriptor set layout not fully configured");
        return index;
    }

    u32 VkDescriptorHeap::allocate_uav(Texture& texture, const UnorderedAccessViewDesc* desc) {
        u32 index = m_CurrentIndex++;
        VkDescriptorImageInfo image_info{};
        image_info.imageView = reinterpret_cast<VkImageView>(texture.resource);
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.sampler = VK_NULL_HANDLE;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 3; // Binding for UAV storage images
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo = &image_info;
        CORE_WARN("VkDescriptorHeap::allocate_uav(Texture) - descriptor set layout not fully configured");
        return index;
    }

    u32 VkDescriptorHeap::allocate_cbv(Buffer& buffer) {
        u32 index = m_CurrentIndex++;
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        buffer_info.offset = 0;
        buffer_info.range = buffer.size_in_bytes;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 4; // Binding for uniform buffers
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &buffer_info;
        CORE_WARN("VkDescriptorHeap::allocate_cbv(Buffer) - descriptor set layout not fully configured");
        return index;
    }

    u32 VkDescriptorHeap::allocate_cbv(const ConstantBufferViewDesc& desc) {
        u32 index = m_CurrentIndex++;
        // This version uses a descriptor with explicit buffer location
        // In Vulkan, we would need the buffer handle, which isn't provided here
        CORE_WARN("VkDescriptorHeap::allocate_cbv(Desc) - requires buffer handle, use allocate_cbv(Buffer&) instead");
        return index;
    }

    u32 VkDescriptorHeap::allocate_rtv(Texture& texture, const RenderTargetViewDesc* desc) {
        u32 index = m_CurrentIndex++;
        CORE_WARN("VkDescriptorHeap::allocate_rtv - RTVs are framebuffer attachments in Vulkan, not descriptors");
        return index;
    }

    u32 VkDescriptorHeap::allocate_dsv(Texture& texture, const DepthStencilViewDesc* desc) {
        u32 index = m_CurrentIndex++;
        CORE_WARN("VkDescriptorHeap::allocate_dsv - DSVs are framebuffer attachments in Vulkan, not descriptors");
        return index;
    }

    u32 VkDescriptorHeap::allocate_sampler(const SamplerDesc& desc) {
        u32 index = m_CurrentIndex++;
        // Create a Vulkan sampler from the descriptor
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.maxAnisotropy = 1.0f;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        // TODO: Convert desc to proper Vulkan sampler settings
        // TODO: Store created sampler for cleanup
        // VkSampler sampler;
        // vkCreateSampler(m_Device, &sampler_info, nullptr, &sampler);
        CORE_WARN("VkDescriptorHeap::allocate_sampler - sampler creation not fully implemented");
        return index;
    }

    void* VkDescriptorHeap::get_cpu_handle(u32 index) {
        // In Vulkan, there's no concept of CPU/GPU descriptor handles like in D3D12
        // Descriptors are accessed through descriptor sets
        // For compatibility, return the descriptor set (index is implicit in the binding)
        return reinterpret_cast<void*>(m_DescriptorSet);
    }

    void* VkDescriptorHeap::get_gpu_handle(u32 index) {
        // In Vulkan, GPU access to descriptors is through descriptor sets bound to the pipeline
        // The index would be used as the array element when accessing the descriptor in shaders
        return reinterpret_cast<void*>(m_DescriptorSet);
    }

} // namespace sf::render::vk
