#include "engpch.h"

#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "render/vulkan/vk_descriptor_heap.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {
    VkDescriptorHeap::VkDescriptorHeap(VkDevice device, u32 descriptor_count, const char* name) :
        m_Device(device), m_DescriptorCount(descriptor_count) {
        // Multiply by 3 to account for the 3 bindings in the resource descriptor set layout (SRV, UAV, CBV)
        u32 pool_descriptor_count = descriptor_count * 3;
        stl::array<VkDescriptorPoolSize, 6> pool_sizes{{
            {VK_DESCRIPTOR_TYPE_SAMPLER, pool_descriptor_count},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pool_descriptor_count},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, pool_descriptor_count},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, pool_descriptor_count},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pool_descriptor_count},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pool_descriptor_count},
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

    void VkDescriptorHeap::destroy_resources() {
        if (m_Device == VK_NULL_HANDLE)
            return;
        for (VkSampler sampler : m_Samplers) {
            if (sampler != VK_NULL_HANDLE) {
                vkDestroySampler(m_Device, sampler, nullptr);
            }
        }
        m_Samplers.clear();
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
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        buffer_info.offset = 0;
        buffer_info.range = buffer.size_in_bytes;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 0;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer_info;
        vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
        return index;
    }

    u32 VkDescriptorHeap::allocate_srv(Texture& texture, const ShaderResourceViewDesc* desc) {
        u32 index = m_CurrentIndex++;
        VkDescriptorImageInfo image_info{};
        image_info.imageView = reinterpret_cast<VkImageView>(texture.resource);
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.sampler = VK_NULL_HANDLE;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 1;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
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
        write.dstBinding = 2;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer_info;
        vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
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
        write.dstBinding = 3;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
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
        write.dstBinding = 4;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &buffer_info;
        vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
        return index;
    }

    u32 VkDescriptorHeap::allocate_cbv(const ConstantBufferViewDesc& desc) {
        CORE_ERROR("VkDescriptorHeap::allocate_cbv(ConstantBufferViewDesc) - not supported in Vulkan, use allocate_cbv(Buffer&) instead");
        return UINT32_MAX;
    }

    u32 VkDescriptorHeap::allocate_rtv(Texture& texture, const RenderTargetViewDesc* desc) {
        CORE_ERROR(
            "VkDescriptorHeap::allocate_rtv - RTVs are framebuffer attachments in Vulkan, not descriptors. This should not be called.");
        return UINT32_MAX;
    }

    u32 VkDescriptorHeap::allocate_dsv(Texture& texture, const DepthStencilViewDesc* desc) {
        CORE_ERROR(
            "VkDescriptorHeap::allocate_dsv - DSVs are framebuffer attachments in Vulkan, not descriptors. This should not be called.");
        return UINT32_MAX;
    }

    u32 VkDescriptorHeap::allocate_sampler(const SamplerDesc& desc) {
        u32 index = m_CurrentIndex++;
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        VkFilter min_filter, mag_filter;
        VkSamplerMipmapMode mip_mode;
        bool enable_anisotropy;
        to_vk_sampler_filter(desc.filter, min_filter, mag_filter, mip_mode, enable_anisotropy);
        sampler_info.minFilter = min_filter;
        sampler_info.magFilter = mag_filter;
        sampler_info.mipmapMode = mip_mode;
        sampler_info.addressModeU = to_vk_sampler_address_mode(desc.address_u);
        sampler_info.addressModeV = to_vk_sampler_address_mode(desc.address_v);
        sampler_info.addressModeW = to_vk_sampler_address_mode(desc.address_w);
        sampler_info.mipLodBias = desc.mip_lod_bias;
        sampler_info.anisotropyEnable = enable_anisotropy ? VK_TRUE : VK_FALSE;
        sampler_info.maxAnisotropy = static_cast<f32>(desc.max_anisotropy);
        sampler_info.compareEnable = desc.comparison_func != CompareFunc::Never ? VK_TRUE : VK_FALSE;
        sampler_info.compareOp = to_vk_compare_op(desc.comparison_func);
        sampler_info.minLod = desc.min_lod;
        sampler_info.maxLod = desc.max_lod;
        sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        VkSampler sampler;
        VkResult result = vkCreateSampler(m_Device, &sampler_info, nullptr, &sampler);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create Vulkan sampler: VkResult = {}", static_cast<i32>(result));
            return UINT32_MAX;
        }
        m_Samplers.push_back(sampler);
        VkDescriptorImageInfo image_info{};
        image_info.sampler = sampler;
        image_info.imageView = VK_NULL_HANDLE;
        image_info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 0;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
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

    stl::result<> VkDescriptorHeap::allocate_descriptor_set() {
        if (m_DescriptorSet != VK_NULL_HANDLE) {
            CORE_WARN("VkDescriptorHeap::allocate_descriptor_set - descriptor set already allocated");
            return stl::success;
        }
        if (m_DescriptorSetLayout == VK_NULL_HANDLE) {
            return stl::make_error("Cannot allocate descriptor set: layout not set. Call set_descriptor_set_layout() first.");
        }
        // Check if this is the resource descriptor set (Set 2) which uses variable descriptor counts
        // Set 1 (dummy/sampler) and Set 0 (per-frame) don't use variable counts
        bool use_variable_counts = (m_SetIndex == 2);
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_DescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_DescriptorSetLayout;
        u32 variable_descriptor_count = m_DescriptorCount;
        VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_info{};
        if (use_variable_counts) {
            variable_count_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
            variable_count_info.descriptorSetCount = 1;
            variable_count_info.pDescriptorCounts = &variable_descriptor_count;
            alloc_info.pNext = &variable_count_info;
        }
        VkResult result = vkAllocateDescriptorSets(m_Device, &alloc_info, &m_DescriptorSet);
        if (result != VK_SUCCESS) {
            return stl::make_error("Failed to allocate descriptor set: VkResult = {}", static_cast<i32>(result));
        }
        CORE_INFO("Allocated descriptor set from pool");
        return stl::success;
    }

} // namespace sf::render::vk
