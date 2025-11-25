#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include <d3d12.h>
#include <wrl/client.h>
#include "render/i_descriptor_heap.h"

namespace sf::render::dx12 {

    class DX12DescriptorHeap : public IDescriptorHeap {
    public:
        explicit DX12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, u32 descriptor_count, const wchar_t* name);
        ~DX12DescriptorHeap() override = default;

        // IDescriptorHeap implementation
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

        void* get_native_heap() override { return m_DescriptorHeap.Get(); }
        void* get_cpu_handle(u32 index) override;
        void* get_gpu_handle(u32 index) override;

        // DX12-specific methods
        ID3D12DescriptorHeap* get_d3d12_heap() const { return m_DescriptorHeap.Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_descriptor_handle(u32 index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_descriptor_handle(u32 index) const;
        u32 get_descriptor_size() const { return m_DescriptorSize; }

    private:
        ID3D12Device* m_Device;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
        D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
        u32 m_DescriptorCount;
        u32 m_DescriptorSize;
        u32 m_CurrentIndex = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE m_CPUStart;
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUStart;
    };

} // namespace sf::render::dx12
#endif