#include "engpch.h"

#include "core/logger.h"
#include "render/vulkan/vk_descriptor_pool.h"
#include "render/vulkan/vk_descriptor_set.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_sampler.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {

    VulkanDescriptorSet::VulkanDescriptorSet(VulkanGraphicsDevice* device, VulkanDescriptorPool* pool, const DescriptorSetLayout& layout) :
        m_Device(device), m_Pool(pool->get_vk_pool()), m_DescriptorSet(VK_NULL_HANDLE) {
        stl::vector<VkDescriptorSetLayoutBinding> bindings{mem::MemTag::Temp};
        bindings.reserve(layout.bindings.size());
        for (const auto& binding : layout.bindings) {
            VkDescriptorSetLayoutBinding vk_binding{};
            vk_binding.binding = binding.binding;
            vk_binding.descriptorType = to_vk_descriptor_type(binding.type);
            vk_binding.descriptorCount = binding.count;
            vk_binding.stageFlags = to_vk_shader_stage_flags(binding.stages);
            vk_binding.pImmutableSamplers = nullptr;
            bindings.push_back(vk_binding);
        }
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<u32>(bindings.size());
        layout_info.pBindings = bindings.data();
        VkDescriptorSetLayout vk_layout;
        VkResult result = vkCreateDescriptorSetLayout(m_Device->get_vk_device(), &layout_info, nullptr, &vk_layout);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create Vulkan descriptor set layout: {}", static_cast<i32>(result));
        }
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = pool->get_vk_pool();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &vk_layout;
        result = vkAllocateDescriptorSets(m_Device->get_vk_device(), &alloc_info, &m_DescriptorSet);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to allocate Vulkan descriptor set: {}", static_cast<i32>(result));
        }
        vkDestroyDescriptorSetLayout(m_Device->get_vk_device(), vk_layout, nullptr);
        CORE_TRACE("Created Vulkan descriptor set");
    }

    VulkanDescriptorSet::~VulkanDescriptorSet() {
        // Descriptor sets are freed when pool is destroyed
        // Individual sets don't need explicit cleanup
        CORE_TRACE("Destroyed Vulkan descriptor set");
    }

    void VulkanDescriptorSet::write_buffer(u32 binding, Buffer& buffer, u64 offset, u64 range) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        buffer_info.offset = offset;
        buffer_info.range = (range == UINT64_MAX) ? VK_WHOLE_SIZE : range;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Could be storage buffer too
        write.pBufferInfo = &buffer_info;
        vkUpdateDescriptorSets(m_Device->get_vk_device(), 1, &write, 0, nullptr);
    }

    void VulkanDescriptorSet::write_texture(u32 binding, Texture& texture, ISampler* sampler) {
        VkDescriptorImageInfo image_info{};
        image_info.imageView = reinterpret_cast<VkImageView>(texture.image_view);
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if (sampler) {
            auto* vk_sampler = static_cast<VulkanSampler*>(sampler);
            image_info.sampler = vk_sampler->get_vk_sampler();
        }
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = sampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(m_Device->get_vk_device(), 1, &write, 0, nullptr);
    }

    void VulkanDescriptorSet::write_sampler(u32 binding, ISampler* sampler) {
        auto* vk_sampler = static_cast<VulkanSampler*>(sampler);
        VkDescriptorImageInfo image_info{};
        image_info.sampler = vk_sampler->get_vk_sampler();
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(m_Device->get_vk_device(), 1, &write, 0, nullptr);
    }

    void VulkanDescriptorSet::write_buffer_array(u32 binding, stl::span<Buffer*> buffers, u32 array_element) {
        stl::vector<VkDescriptorBufferInfo> buffer_infos(mem::MemTag::Render);
        buffer_infos.reserve(buffers.size());
        for (Buffer* buffer : buffers) {
            VkDescriptorBufferInfo info{};
            info.buffer = reinterpret_cast<VkBuffer>(buffer->resource);
            info.offset = 0;
            info.range = VK_WHOLE_SIZE;
            buffer_infos.push_back(info);
        }
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = binding;
        write.dstArrayElement = array_element;
        write.descriptorCount = static_cast<u32>(buffer_infos.size());
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = buffer_infos.data();
        vkUpdateDescriptorSets(m_Device->get_vk_device(), 1, &write, 0, nullptr);
    }

    void VulkanDescriptorSet::write_texture_array(u32 binding, stl::span<Texture*> textures, ISampler* sampler, u32 array_element) {
        stl::vector<VkDescriptorImageInfo> image_infos(mem::MemTag::Render);
        image_infos.reserve(textures.size());
        VkSampler vk_sampler = VK_NULL_HANDLE;
        if (sampler) {
            auto* vk_sampler_obj = static_cast<VulkanSampler*>(sampler);
            vk_sampler = vk_sampler_obj->get_vk_sampler();
        }
        for (Texture* texture : textures) {
            VkDescriptorImageInfo info{};
            info.imageView = reinterpret_cast<VkImageView>(texture->image_view);
            info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.sampler = vk_sampler;
            image_infos.push_back(info);
        }
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = binding;
        write.dstArrayElement = array_element;
        write.descriptorCount = static_cast<u32>(image_infos.size());
        write.descriptorType = sampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = image_infos.data();
        vkUpdateDescriptorSets(m_Device->get_vk_device(), 1, &write, 0, nullptr);
    }

} // namespace sf::render::vk
