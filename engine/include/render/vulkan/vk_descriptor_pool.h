#pragma once

#include <vulkan/vulkan.h>
#include "render/i_descriptor_pool.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;

    class VulkanDescriptorPool final : public IDescriptorPool {
    public:
        VulkanDescriptorPool(VulkanGraphicsDevice* device, const DescriptorPoolDesc& desc);
        ~VulkanDescriptorPool() override;

        stl::result<stl::unique_ptr<IDescriptorSet>> allocate_set(const DescriptorSetLayout& layout) override;
        void reset() override;
        void* get_native_pool() override { return reinterpret_cast<void*>(m_Pool); }

        VkDescriptorPool get_vk_pool() const { return m_Pool; }

    private:
        VulkanGraphicsDevice* m_Device = nullptr;
        VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    };

} // namespace sf::render::vk
