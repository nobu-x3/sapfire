#pragma once
#include <vulkan/vulkan.h>
#include "render/i_descriptor_heap.h"

namespace sf::render::vk {

    class VkDescriptorHeap : public IDescriptorHeap {
    public:
        explicit VkDescriptorHeap(VkDevice device, u32 descriptor_count, const char* name);
        ~VkDescriptorHeap() override;

        u32 allocate_srv(Buffer& buffer) override;
        u32 allocate_srv(Texture& texture, const ShaderResourceViewDesc* desc = nullptr) override;

        u32 allocate_uav(Buffer& buffer) override;
        u32 allocate_uav(Texture& texture, const UnorderedAccessViewDesc* desc = nullptr) override;

        u32 allocate_cbv(Buffer& buffer) override;
        u32 allocate_cbv(const ConstantBufferViewDesc& desc) override;

        u32 allocate_rtv(Texture& texture, const RenderTargetViewDesc* desc = nullptr) override;
        u32 allocate_dsv(Texture& texture, const DepthStencilViewDesc* desc = nullptr) override;

        u32 allocate_sampler(const SamplerDesc& desc) override;

        u32 get_descriptor_count() const override { return m_DescriptorCount; }
        u32 get_allocated_count() const override { return m_CurrentIndex; }

        void* get_native_heap() override { return reinterpret_cast<void*>(m_DescriptorPool); }
        void* get_cpu_handle(u32 index) override;
        void* get_gpu_handle(u32 index) override;

        VkDescriptorPool get_vk_pool() const { return m_DescriptorPool; }
        VkDescriptorSet get_vk_set() const { return m_DescriptorSet; }

    private:
        VkDevice m_Device;
        VkDescriptorPool m_DescriptorPool;
        VkDescriptorSet m_DescriptorSet;
        VkDescriptorSetLayout m_DescriptorSetLayout;
        u32 m_DescriptorCount;
        u32 m_CurrentIndex = 0;
    };

} // namespace sf::render::vk
