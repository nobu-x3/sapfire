#include "render/vulkan/vk_pipeline_layout.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {

    VulkanPipelineLayout::VulkanPipelineLayout(VulkanGraphicsDevice* device, const PipelineLayoutDesc& desc) :
        m_Device(device), m_Desc(desc), m_PipelineLayout(VK_NULL_HANDLE) {
        m_DescriptorSetLayouts.reserve(m_Desc.descriptor_set_layouts.size());
        // Create descriptor set layouts
        for (const auto& set_layout : m_Desc.descriptor_set_layouts) {
            stl::vector<VkDescriptorSetLayoutBinding> bindings(mem::MemTag::Render);
            bindings.reserve(set_layout.bindings.size());
            for (const auto& binding : set_layout.bindings) {
                VkDescriptorSetLayoutBinding vk_binding{};
                vk_binding.binding = binding.binding;
                vk_binding.descriptorType = to_vk_descriptor_type(binding.type);
                vk_binding.descriptorCount = binding.count;
                vk_binding.stageFlags = to_vk_shader_stage_flags(binding.stages);
                vk_binding.pImmutableSamplers = nullptr;
                bindings.push_back(vk_binding);
            }
            // Check if this layout has variable descriptor count
            stl::vector<VkDescriptorBindingFlags> binding_flags(mem::MemTag::Render);
            bool has_variable_count = false;
            for (const auto& binding : set_layout.bindings) {
                VkDescriptorBindingFlags flags = 0;
                if (binding.variable_count) {
                    flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                    flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                    has_variable_count = true;
                }
                binding_flags.push_back(flags);
            }
            VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info{};
            if (has_variable_count) {
                binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                binding_flags_info.bindingCount = static_cast<u32>(binding_flags.size());
                binding_flags_info.pBindingFlags = binding_flags.data();
            }
            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.flags = has_variable_count ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
            layout_info.pNext = has_variable_count ? &binding_flags_info : nullptr;
            layout_info.bindingCount = static_cast<u32>(bindings.size());
            layout_info.pBindings = bindings.data();
            VkDescriptorSetLayout vk_layout;
            VkResult result = vkCreateDescriptorSetLayout(m_Device->get_vk_device(), &layout_info, nullptr, &vk_layout);
            if (result != VK_SUCCESS) {
                // Clean up already created layouts
                for (auto& layout : m_DescriptorSetLayouts) {
                    vkDestroyDescriptorSetLayout(m_Device->get_vk_device(), layout, nullptr);
                }
                m_DescriptorSetLayouts.clear();
                CORE_ERROR("Failed to create Vulkan descriptor set layout: {}", static_cast<i32>(result));
                return;
            }
            m_DescriptorSetLayouts.push_back(vk_layout);
        }
        // Create push constant ranges
        stl::vector<VkPushConstantRange> push_constant_ranges(mem::MemTag::Render);
        push_constant_ranges.reserve(m_Desc.push_constant_ranges.size());
        for (const auto& range : m_Desc.push_constant_ranges) {
            VkPushConstantRange vk_range{};
            vk_range.stageFlags = to_vk_shader_stage_flags(range.stages);
            vk_range.offset = range.offset;
            vk_range.size = range.size;
            push_constant_ranges.push_back(vk_range);
        }
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = static_cast<u32>(m_DescriptorSetLayouts.size());
        pipeline_layout_info.pSetLayouts = m_DescriptorSetLayouts.data();
        pipeline_layout_info.pushConstantRangeCount = static_cast<u32>(push_constant_ranges.size());
        pipeline_layout_info.pPushConstantRanges = push_constant_ranges.empty() ? nullptr : push_constant_ranges.data();
        VkResult result = vkCreatePipelineLayout(m_Device->get_vk_device(), &pipeline_layout_info, nullptr, &m_PipelineLayout);
        if (result != VK_SUCCESS) {
            // Clean up descriptor set layouts
            for (auto& layout : m_DescriptorSetLayouts) {
                vkDestroyDescriptorSetLayout(m_Device->get_vk_device(), layout, nullptr);
            }
            m_DescriptorSetLayouts.clear();
            CORE_ERROR("Failed to create Vulkan pipeline layout: {}", static_cast<i32>(result));
            return;
        }
        CORE_TRACE("Created Vulkan pipeline layout: {} ({} descriptor sets, {} push constant ranges)", m_Desc.name,
                   m_DescriptorSetLayouts.size(), push_constant_ranges.size());
    }

    VulkanPipelineLayout::~VulkanPipelineLayout() {
        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_Device->get_vk_device(), m_PipelineLayout, nullptr);
            CORE_TRACE("Destroyed Vulkan pipeline layout: {}", m_Desc.name);
        }
        for (auto& layout : m_DescriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(m_Device->get_vk_device(), layout, nullptr);
        }
        m_DescriptorSetLayouts.clear();
    }

} // namespace sf::render::vk
