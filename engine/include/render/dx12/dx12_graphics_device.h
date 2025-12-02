#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include "render/dx12/dx12_command_queue.h"
#include "render/dx12/dx12_context.h"
#include "render/dx12/dx12_descriptor_heap.h"
#include "render/dx12/dx12_memory_allocator.h"
#include "render/dx12/dx12_pipeline_state.h"
#include "render/i_graphics_device.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <mutex>
#include <wrl/client.h>

namespace sf::render::dx12 {

    class DX12GraphicsDevice : public IGraphicsDevice {
    public:
        explicit DX12GraphicsDevice(const SwapchainCreationDesc& desc);
        ~DX12GraphicsDevice() override;

        // Frame Management

        stl::result<> begin_frame() override;
        stl::result<> end_frame(IGraphicsContext* context) override;
        stl::result<> present() override;
        stl::result<> wait_for_idle() override;

        // Window / Swapchain Management

        stl::result<> resize_window(u32 width, u32 height) override;
        u32 get_window_width() const override { return m_WindowWidth; }
        u32 get_window_height() const override { return m_WindowHeight; }
        Texture& get_current_back_buffer() override;
        Texture& get_back_buffer(u32 index) override;
        u32 get_current_back_buffer_index() const override { return m_CurrentBackBufferIndex; }
        u32 get_back_buffer_count() const override { return m_BackBufferCount; }

        // Resource Creation

        stl::result<Buffer> create_buffer(const BufferCreationDesc& desc) override;
        stl::result<Buffer> create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) override;

        stl::result<Texture> create_texture(const TextureCreationDesc& desc) override;
        stl::result<Texture> create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) override;

        stl::result<IPipelineState*> create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) override;
        stl::result<IPipelineState*> create_compute_pipeline(const ComputePipelineStateDesc& desc) override;

        // Texture readback (screenshots, editor viewport, etc.)
        stl::result<> read_texture_pixels(Texture& texture, void* out_data, size_t data_size) override;

        // Context Creation Factories

        stl::result<IGraphicsContext*> create_graphics_context() override;
        stl::result<IComputeContext*> create_compute_context() override;
        stl::result<ICopyContext*> create_copy_context() override;

        // Command Queue Creation Factories

        stl::result<ICommandQueue*> create_direct_queue(const char* name = "Direct Queue") override;
        stl::result<ICommandQueue*> create_compute_queue(const char* name = "Compute Queue") override;
        stl::result<ICommandQueue*> create_copy_queue(const char* name = "Copy Queue") override;

        // Descriptor Heap Creation Factories

        stl::result<IDescriptorHeap*> create_cbv_srv_uav_heap(const DescriptorHeapDesc& desc) override;
        stl::result<IDescriptorHeap*> create_rtv_heap(const DescriptorHeapDesc& desc) override;
        stl::result<IDescriptorHeap*> create_dsv_heap(const DescriptorHeapDesc& desc) override;
        stl::result<IDescriptorHeap*> create_sampler_heap(const DescriptorHeapDesc& desc) override;

        // Memory Allocator Creation Factory

        stl::result<IMemoryAllocator*> create_memory_allocator() override;

        // Frame Timing

        u32 get_current_frame_index() const override { return m_CurrentFrameIndex; }

        // Backend Information

        RenderAPI get_api() const override { return RenderAPI::DX12; }
        const char* get_api_name() const override { return "DirectX 12"; }
        void* get_native_device() override { return m_Device.Get(); }

        // DX12-specific accessors

        ID3D12Device* get_d3d12_device() const { return m_Device.Get(); }
        IDXGISwapChain3* get_d3d12_swapchain() const { return m_Swapchain.Get(); }
        ID3D12RootSignature* get_bindless_root_signature() const { return m_BindlessRootSignature.Get(); }

    private:
        // Initialization methods
        void init_device_resources();
        void init_swapchain_resources(const SwapchainCreationDesc& desc);
        void init_directx();
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


        // Pipeline states (owned by device)
        stl::tvector<stl::tunique_ptr<DX12PipelineState>> m_PipelineStates;

        // Swapchain data
        stl::array<Texture, MAX_FRAMES_IN_FLIGHT> m_BackBuffers;
        u32 m_BackBufferCount = MAX_FRAMES_IN_FLIGHT;
        u32 m_CurrentBackBufferIndex = 0;
        Format m_BackBufferFormat;

        // Window data
        void* m_WindowHandle = nullptr; // HWND on Windows
        u32 m_WindowWidth = 0;
        u32 m_WindowHeight = 0;

        // Frame tracking
        u32 m_CurrentFrameIndex = 0;
        stl::array<u64, MAX_FRAMES_IN_FLIGHT> m_FenceValues = {};

        // Thread safety
        mutable stl::recursive_mutex m_ResourceMutex;
    };

} // namespace sf::render::dx12
#endif
