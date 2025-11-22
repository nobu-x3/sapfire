#include "engpch.h"
#include "render/vulkan/vk_descriptor_heap.h"
#include "render/vulkan/vk_type_conversions.h"
#include "core/logger.h"

namespace sf::render::vk {

VkDescriptorHeap::VkDescriptorHeap(VkDevice device, u32 descriptor_count, const char* name)
    : m_Device(device), m_DescriptorCount(descriptor_count) {

    stl::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptor_count},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptor_count},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor_count},
    };

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
    if (m_DescriptorSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_DescriptorSet);
    }
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    }
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    }
}

u32 VkDescriptorHeap::allocate_srv(Buffer& buffer) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_srv(Buffer) - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_srv(Texture& texture, const ShaderResourceViewDesc* desc) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_srv(Texture) - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_uav(Buffer& buffer) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_uav(Buffer) - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_uav(Texture& texture, const UnorderedAccessViewDesc* desc) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_uav(Texture) - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_cbv(Buffer& buffer) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_cbv(Buffer) - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_cbv(const ConstantBufferViewDesc& desc) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_cbv(Desc) - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_rtv(Texture& texture, const RenderTargetViewDesc* desc) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_rtv - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_dsv(Texture& texture, const DepthStencilViewDesc* desc) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_dsv - stub implementation");
    return index;
}

u32 VkDescriptorHeap::allocate_sampler(const SamplerDesc& desc) {
    u32 index = m_CurrentIndex++;
    CORE_WARN("VkDescriptorHeap::allocate_sampler - stub implementation");
    return index;
}

void* VkDescriptorHeap::get_cpu_handle(u32 index) {
    CORE_WARN("VkDescriptorHeap::get_cpu_handle - stub implementation");
    return nullptr;
}

void* VkDescriptorHeap::get_gpu_handle(u32 index) {
    CORE_WARN("VkDescriptorHeap::get_gpu_handle - stub implementation");
    return nullptr;
}

} // namespace sf::render::vk
