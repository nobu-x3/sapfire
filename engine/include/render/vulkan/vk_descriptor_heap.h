#pragma once
#include <vulkan/vulkan.h>
#include "render/i_descriptor_heap.h"

namespace sf::render::vk {

    class VkDescriptorHeap final: public IDescriptorHeap {
    public:
        explicit VkDescriptorHeap(VkDevice device, u32 descriptor_count, const char* name);

        void destroy_resources();

        u32 allocate_srv(Buffer& buffer) override;
        u32 allocate_srv(Texture& texture, const ShaderResourceViewDesc* desc = nullptr) override;

        u32 allocate_uav(Buffer& buffer) override;
        u32 allocate_uav(Texture& texture, const UnorderedAccessViewDesc* desc = nullptr) override;

        u32 allocate_cbv(Buffer& buffer) override;
        u32 allocate_cbv(const ConstantBufferViewDesc& desc) override;

        u32 allocate_rtv(Texture& texture, const RenderTargetViewDesc* desc = nullptr) override;
        u32 allocate_dsv(Texture& texture, const DepthStencilViewDesc* desc = nullptr) override;

        u32 allocate_sampler(const SamplerDesc& desc) override;

        stl::result<> allocate_descriptor_set() override;

        u32 get_descriptor_count() const override { return m_DescriptorCount; }
        u32 get_allocated_count() const override { return m_CurrentIndex; }

        void* get_native_heap() override { return reinterpret_cast<void*>(m_DescriptorPool); }
        void* get_cpu_handle(u32 index) override;
        void* get_gpu_handle(u32 index) override;

        VkDescriptorPool get_vk_pool() const { return m_DescriptorPool; }
        VkDescriptorSet get_vk_set() const { return m_DescriptorSet; }
        u32 get_set_index() const { return m_SetIndex; }

        void set_descriptor_set_layout(VkDescriptorSetLayout layout, u32 set_index) {
            m_DescriptorSetLayout = layout;
            m_SetIndex = set_index;
        }

    private:
        VkDevice m_Device{VK_NULL_HANDLE};
        VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};
        VkDescriptorSet m_DescriptorSet{VK_NULL_HANDLE};
        VkDescriptorSetLayout m_DescriptorSetLayout{VK_NULL_HANDLE};
        u32 m_DescriptorCount{0};
        u32 m_CurrentIndex{0};
        u32 m_SetIndex{0};
        stl::vector<VkSampler> m_Samplers{mem::MemTag::Render};
    };

} // namespace sf::render::vk
