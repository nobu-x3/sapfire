#pragma once

#include <vulkan/vulkan.h>
#include "render/i_sampler.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;

    class VulkanSampler final : public ISampler {
    public:
        VulkanSampler(VulkanGraphicsDevice* device, const SamplerDesc& desc);
        ~VulkanSampler() override;

        // ISampler interface
        const SamplerDesc& get_desc() const override { return m_Desc; }
        void* get_native_sampler() override { return reinterpret_cast<void*>(m_Sampler); }

        // Vulkan-specific accessors
        VkSampler get_vk_sampler() const { return m_Sampler; }

    private:
        VulkanGraphicsDevice* m_Device;
        SamplerDesc m_Desc;
        VkSampler m_Sampler;
    };

} // namespace sf::render::vk
