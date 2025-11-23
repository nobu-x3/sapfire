#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include "render/i_graphics_device.h"
#include "render/dx12/dx12_command_queue.h"
#include "render/dx12/dx12_context.h"
#include "render/dx12/dx12_descriptor_heap.h"
#include "render/dx12/dx12_memory_allocator.h"
#include "render/dx12/dx12_pipeline_state.h"

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <mutex>

namespace sf::render::dx12 {

class DX12GraphicsDevice : public IGraphicsDevice {
public:
    explicit DX12GraphicsDevice(const SwapchainCreationDesc& desc);
    ~DX12GraphicsDevice() override;

    // Frame Management

    void begin_frame() override;
    void end_frame() override;
    void present() override;
    void wait_for_idle() override;

    // Window / Swapchain Management

    void resize_window(u32 width, u32 height) override;
    u32 get_window_width() const override { return m_WindowWidth; }
    u32 get_window_height() const override { return m_WindowHeight; }
    Texture& get_current_back_buffer() override;
    Texture& get_back_buffer(u32 index) override;
    u32 get_current_back_buffer_index() const override { return m_CurrentBackBufferIndex; }
    u32 get_back_buffer_count() const override { return m_BackBufferCount; }

    // Resource Creation

    Buffer create_buffer(const BufferCreationDesc& desc) override;
    Buffer create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) override;

    Texture create_texture(const TextureCreationDesc& desc) override;
    Texture create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) override;

    IPipelineState* create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) override;
    IPipelineState* create_compute_pipeline(const ComputePipelineStateDesc& desc) override;

    // Context Access

    IGraphicsContext& get_current_graphics_context() override;
    IGraphicsContext& get_graphics_context(u32 frame_index) override;
    IComputeContext& get_compute_context() override;
    ICopyContext& get_copy_context() override;

    // Command Queue Access

    ICommandQueue* get_direct_queue() override { return m_DirectQueue.get(); }
    ICommandQueue* get_compute_queue() override { return m_ComputeQueue.get(); }
    ICommandQueue* get_copy_queue() override { return m_CopyQueue.get(); }

    // Descriptor Heap Access

    IDescriptorHeap* get_cbv_srv_uav_heap() override { return m_CbvSrvUavHeap.get(); }
    IDescriptorHeap* get_rtv_heap() override { return m_RtvHeap.get(); }
    IDescriptorHeap* get_dsv_heap() override { return m_DsvHeap.get(); }
    IDescriptorHeap* get_sampler_heap() override { return m_SamplerHeap.get(); }

    // Memory Allocator Access

    IMemoryAllocator* get_memory_allocator() override { return m_MemoryAllocator.get(); }

    // Frame Timing

    u32 get_current_frame_index() const override { return m_CurrentFrameIndex; }
    u32 get_frames_in_flight() const override { return m_FramesInFlight; }

    // Backend Information

    RenderAPI get_api() const override { return RenderAPI::DX12; }
    const char* get_api_name() const override { return "DirectX 12"; }
    void* get_native_device() override { return m_Device.Get(); }

    // DX12-specific accessors

    ID3D12Device* get_d3d12_device() const { return m_Device.Get(); }
    IDXGISwapChain3* get_d3d12_swapchain() const { return m_Swapchain.Get(); }
    ID3D12RootSignature* get_bindless_root_signature() const { return m_BindlessRootSignature.Get(); }

    DX12DescriptorHeap* get_dx12_cbv_srv_uav_heap() const { return m_CbvSrvUavHeap.get(); }
    DX12DescriptorHeap* get_dx12_rtv_heap() const { return m_RtvHeap.get(); }
    DX12DescriptorHeap* get_dx12_dsv_heap() const { return m_DsvHeap.get(); }
    DX12DescriptorHeap* get_dx12_sampler_heap() const { return m_SamplerHeap.get(); }

private:
    // Initialization methods
    void init_device_resources();
    void init_swapchain_resources(const SwapchainCreationDesc& desc);
    void init_directx();
    void init_command_queues();
    void init_descriptor_heaps();
    void init_memory_allocator();
    void init_contexts();
    void init_bindless_root_signature();
    void create_backbuffer_rtvs();

private:
    // DX12 core objects
    Microsoft::WRL::ComPtr<IDXGIFactory6> m_Factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter> m_Adapter;
    Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_Swapchain;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_BindlessRootSignature;

    // Debug objects
#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug3> m_Debug;
    Microsoft::WRL::ComPtr<ID3D12DebugDevice> m_DebugDevice;
#endif

    // Command queues
    stl::tunique_ptr<DX12CommandQueue> m_DirectQueue;
    stl::tunique_ptr<DX12CommandQueue> m_ComputeQueue;
    stl::tunique_ptr<DX12CommandQueue> m_CopyQueue;

    // Contexts (triple buffered for graphics)
    stl::array<stl::tunique_ptr<DX12GraphicsContext>, MAX_FRAMES_IN_FLIGHT> m_GraphicsContexts;
    stl::tunique_ptr<DX12ComputeContext> m_ComputeContext;
    stl::tunique_ptr<DX12CopyContext> m_CopyContext;

    // Descriptor heaps
    stl::tunique_ptr<DX12DescriptorHeap> m_CbvSrvUavHeap;
    stl::tunique_ptr<DX12DescriptorHeap> m_RtvHeap;
    stl::tunique_ptr<DX12DescriptorHeap> m_DsvHeap;
    stl::tunique_ptr<DX12DescriptorHeap> m_SamplerHeap;

    // Memory allocator
    stl::tunique_ptr<DX12MemoryAllocator> m_MemoryAllocator;

    // Pipeline states (owned by device)
    stl::tvector<stl::tunique_ptr<DX12PipelineState>> m_PipelineStates;

    // Swapchain data
    stl::array<Texture, MAX_FRAMES_IN_FLIGHT> m_BackBuffers;
    u32 m_BackBufferCount = MAX_FRAMES_IN_FLIGHT;
    u32 m_CurrentBackBufferIndex = 0;
    Format m_BackBufferFormat;

    // Window data
    void* m_WindowHandle = nullptr;  // HWND on Windows
    u32 m_WindowWidth = 0;
    u32 m_WindowHeight = 0;

    // Frame tracking
    u32 m_CurrentFrameIndex = 0;
    u32 m_FramesInFlight = MAX_FRAMES_IN_FLIGHT;
    stl::array<u64, MAX_FRAMES_IN_FLIGHT> m_FenceValues = {};

    // Thread safety
    mutable stl::recursive_mutex m_ResourceMutex;
};

} // namespace sf::render::dx12
#endif
