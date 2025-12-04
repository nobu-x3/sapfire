#include "render/vulkan/vk_sampler.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {

    VulkanSampler::VulkanSampler(VulkanGraphicsDevice* device, const SamplerDesc& desc) :
        m_Device(device), m_Desc(desc), m_Sampler(VK_NULL_HANDLE) {
        VkFilter min_filter, mag_filter;
        VkSamplerMipmapMode mip_mode;
        bool anisotropy_enable;
        to_vk_sampler_filter(desc.filter, min_filter, mag_filter, mip_mode, anisotropy_enable);
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = mag_filter;
        sampler_info.minFilter = min_filter;
        sampler_info.mipmapMode = mip_mode;
        sampler_info.addressModeU = to_vk_sampler_address_mode(desc.address_u);
        sampler_info.addressModeV = to_vk_sampler_address_mode(desc.address_v);
        sampler_info.addressModeW = to_vk_sampler_address_mode(desc.address_w);
        sampler_info.mipLodBias = desc.mip_lod_bias;
        sampler_info.anisotropyEnable = anisotropy_enable ? VK_TRUE : VK_FALSE;
        sampler_info.maxAnisotropy = static_cast<f32>(desc.max_anisotropy);
        sampler_info.compareEnable = (desc.comparison_func != CompareFunc::Never) ? VK_TRUE : VK_FALSE;
        sampler_info.compareOp = to_vk_compare_op(desc.comparison_func);
        sampler_info.minLod = desc.min_lod;
        sampler_info.maxLod = desc.max_lod;
        sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        VkResult result = vkCreateSampler(m_Device->get_vk_device(), &sampler_info, nullptr, &m_Sampler);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create Vulkan sampler: {}", static_cast<i32>(result));
        }
        CORE_TRACE("Created Vulkan sampler");
    }

    VulkanSampler::~VulkanSampler() {
        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_Device->get_vk_device(), m_Sampler, nullptr);
            CORE_TRACE("Destroyed Vulkan sampler");
        }
    }

} // namespace sf::render::vk
