#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include "render/i_memory_allocator.h"
#include "D3D12MemAlloc.h"
#include <wrl/client.h>

namespace sf::render::dx12 {

class DX12MemoryAllocator : public IMemoryAllocator {
public:
    explicit DX12MemoryAllocator(ID3D12Device* device, IDXGIAdapter* adapter);
    ~DX12MemoryAllocator() override;

    // IMemoryAllocator implementation
    void allocate_buffer(Buffer& buffer, const BufferCreationDesc& desc) override;
    void allocate_texture(Texture& texture, const TextureCreationDesc& desc) override;

    void free_buffer(Buffer& buffer) override;
    void free_texture(Texture& texture) override;

    void get_stats(void* stats_out) override;
    void* get_native_allocator() override { return m_Allocator.Get(); }

    // DX12-specific
    D3D12MA::Allocator* get_d3d12_allocator() const { return m_Allocator.Get(); }

private:
    ID3D12Device* m_Device;
    Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_Allocator;
};

} // namespace sf::render::dx12
#endif
