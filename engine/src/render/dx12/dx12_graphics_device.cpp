#include "render/dx12/dx12_graphics_device.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/dx12/dx12_shader_compiler.h"
#include "render/dx12/dx12_type_conversions.h"
#include "render/dx12/dx12_util.h"

namespace sf::render::dx12 {
    DX12GraphicsDevice::DX12GraphicsDevice(const SwapchainCreationDesc& desc) {
        m_WindowHandle = desc.window_handle;
        m_WindowWidth = desc.width;
        m_WindowHeight = desc.height;
        m_BackBufferFormat = desc.format;
        m_BackBufferCount = desc.buffer_count;
        init_device_resources();
        init_swapchain_resources(desc);
    }
    DX12GraphicsDevice::~DX12GraphicsDevice() {
        if (m_DirectQueue) {
            m_DirectQueue->wait_for_idle();
        }
    }
    // Frame Management

    stl::result<> DX12GraphicsDevice::begin_frame() {
        m_GraphicsContexts[m_CurrentFrameIndex]->reset();
        return stl::success;
    }

    stl::result<> DX12GraphicsDevice::end_frame(IGraphicsContext* context) {
        // DX12 backend not yet refactored - stub implementation
        m_FenceValues[m_CurrentFrameIndex] = m_DirectQueue->signal();
        m_CurrentBackBufferIndex = m_Swapchain->GetCurrentBackBufferIndex();
        m_DirectQueue->wait_for_fence_value(m_FenceValues[m_CurrentFrameIndex]);
        return stl::success;
    }

    stl::result<> DX12GraphicsDevice::present() {
        dx12_check(m_Swapchain->Present(1, 0));
        return stl::success;
    }

    stl::result<> DX12GraphicsDevice::wait_for_idle() {
        if (m_DirectQueue)
            m_DirectQueue->wait_for_idle();
        if (m_ComputeQueue)
            m_ComputeQueue->wait_for_idle();
        if (m_CopyQueue)
            m_CopyQueue->wait_for_idle();
        return stl::success;
    }
    // Window / Swapchain Management

    stl::result<> DX12GraphicsDevice::resize_window(u32 width, u32 height) {
        m_DirectQueue->wait_for_idle();
        m_CopyQueue->wait_for_idle();
        for (u32 i = 0; i < m_BackBufferCount; ++i) {
            m_BackBuffers[i].resource = nullptr;
            m_FenceValues[i] = m_DirectQueue->get_last_completed_fence_value();
        }
        DXGI_SWAP_CHAIN_DESC swapchain_desc{};
        dx12_check(m_Swapchain->GetDesc(&swapchain_desc));
        dx12_check(m_Swapchain->ResizeBuffers(m_BackBufferCount, width, height, to_dxgi_format(m_BackBufferFormat), swapchain_desc.Flags));
        m_WindowWidth = width;
        m_WindowHeight = height;
        m_CurrentBackBufferIndex = m_Swapchain->GetCurrentBackBufferIndex();
        create_backbuffer_rtvs();
        return stl::success;
    }
    Texture& DX12GraphicsDevice::get_current_back_buffer() { return m_BackBuffers[m_CurrentBackBufferIndex]; }
    Texture& DX12GraphicsDevice::get_back_buffer(u32 index) { return m_BackBuffers[index]; }
    // Resource Creation
    Buffer DX12GraphicsDevice::create_buffer(const BufferCreationDesc& desc) {
        Buffer buffer{};
        m_MemoryAllocator->allocate_buffer(buffer, desc);
        if (desc.usage == BufferUsage::Structured) {
            buffer.srv_index = m_CbvSrvUavHeap->allocate_srv(buffer);
            buffer.uav_index = m_CbvSrvUavHeap->allocate_uav(buffer);
        } else if (desc.usage == BufferUsage::Constant) {
            buffer.cbv_index = m_CbvSrvUavHeap->allocate_cbv(buffer);
        }
        return buffer;
    }
    Buffer DX12GraphicsDevice::create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) {
        BufferCreationDesc buffer_desc = desc;
        buffer_desc.size_in_bytes = data_size;
        Buffer buffer = create_buffer(buffer_desc);
        BufferCreationDesc upload_desc = {.usage = BufferUsage::Upload, .size_in_bytes = data_size, .name = L"Upload Buffer"};
        Buffer upload_buffer = create_buffer(upload_desc);
        if (upload_buffer.mapped_data) {
            memcpy(upload_buffer.mapped_data, data, data_size);
        }
        m_CopyContext->reset();
        m_CopyContext->copy_buffer(buffer, upload_buffer, data_size);
        m_CopyContext->close();
        m_CopyQueue->execute_command_list(m_CopyContext.get());
        m_CopyQueue->wait_for_idle();
        m_MemoryAllocator->free_buffer(upload_buffer);
        return buffer;
    }
    Texture DX12GraphicsDevice::create_texture(const TextureCreationDesc& desc) {
        Texture texture{};
        m_MemoryAllocator->allocate_texture(texture, desc);
        if (has_flag(desc.resource_usage, ResourceUsage::ShaderResource) || desc.usage == TextureUsage::ShaderResource) {
            texture.srv_index = m_CbvSrvUavHeap->allocate_srv(texture);
        }
        if (has_flag(desc.resource_usage, ResourceUsage::UnorderedAccess) || desc.usage == TextureUsage::UnorderedAccess) {
            texture.uav_index = m_CbvSrvUavHeap->allocate_uav(texture);
        }
        if (has_flag(desc.resource_usage, ResourceUsage::RenderTarget) || desc.usage == TextureUsage::RenderTarget) {
            texture.rtv_index = m_RtvHeap->allocate_rtv(texture);
        }
        if (has_flag(desc.resource_usage, ResourceUsage::DepthStencil) || desc.usage == TextureUsage::DepthStencil) {
            texture.dsv_index = m_DsvHeap->allocate_dsv(texture);
        }
        return texture;
    }
    Texture DX12GraphicsDevice::create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) {
        Texture texture = create_texture(desc);
        if (data && data_size > 0) {
            BufferCreationDesc upload_desc = {.usage = BufferUsage::Upload, .size_in_bytes = data_size, .name = L"Texture Upload Buffer"};
            Buffer upload_buffer = create_buffer(upload_desc);
            if (upload_buffer.mapped_data) {
                memcpy(upload_buffer.mapped_data, data, data_size);
            }
            m_CopyContext->reset();
            m_CopyContext->copy_buffer_to_texture(texture, upload_buffer, 0);
            m_CopyContext->close();
            m_CopyQueue->execute_command_list(m_CopyContext.get());
            m_CopyQueue->wait_for_idle();
            m_MemoryAllocator->free_buffer(upload_buffer);
        }
        return texture;
    }
    IPipelineState* DX12GraphicsDevice::create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) {
        auto pipeline = stl::make_tunique<DX12PipelineState>();
        pipeline->create_graphics(m_Device.Get(), m_BindlessRootSignature.Get(), desc);
        IPipelineState* result = pipeline.get();
        m_PipelineStates.push_back(std::move(pipeline));
        return result;
    }
    IPipelineState* DX12GraphicsDevice::create_compute_pipeline(const ComputePipelineStateDesc& desc) {
        auto pipeline = stl::make_tunique<DX12PipelineState>();
        pipeline->create_compute(m_Device.Get(), m_BindlessRootSignature.Get(), desc);
        IPipelineState* result = pipeline.get();
        m_PipelineStates.push_back(std::move(pipeline));
        return result;
    }
    // Context Access
    IGraphicsContext& DX12GraphicsDevice::get_current_graphics_context() { return *m_GraphicsContexts[m_CurrentFrameIndex]; }
    IGraphicsContext& DX12GraphicsDevice::get_graphics_context(u32 frame_index) { return *m_GraphicsContexts[frame_index]; }
    IComputeContext& DX12GraphicsDevice::get_compute_context() { return *m_ComputeContext; }
    ICopyContext& DX12GraphicsDevice::get_copy_context() { return *m_CopyContext; }
    // Initialization

    void DX12GraphicsDevice::init_device_resources() {
        init_directx();
        init_command_queues();
        init_descriptor_heaps();
        init_memory_allocator();
        init_contexts();
        init_bindless_root_signature();
    }

    void DX12GraphicsDevice::init_directx() {
#ifdef _DEBUG
        dx12_check(D3D12GetDebugInterface(IID_PPV_ARGS(&m_Debug)));
        m_Debug->EnableDebugLayer();
        m_Debug->SetEnableGPUBasedValidation(TRUE);
#endif
        UINT dxgi_factory_flags = 0;
#ifdef _DEBUG
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        dx12_check(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_Factory)));
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
        for (UINT adapter_index = 0; DXGI_ERROR_NOT_FOUND != m_Factory->EnumAdapters1(adapter_index, &adapter1); ++adapter_index) {
            DXGI_ADAPTER_DESC1 desc;
            adapter1->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
        adapter1.As(&m_Adapter);
        dx12_check(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device)));
        m_Device->SetName(L"Main D3D12 Device");
#ifdef _DEBUG
        dx12_check(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugDevice)));
#endif
    }

    void DX12GraphicsDevice::init_command_queues() {
        m_DirectQueue = stl::make_tunique<DX12CommandQueue>(m_Device.Get(), CommandQueueType::Direct, L"Direct Queue");
        m_ComputeQueue = stl::make_tunique<DX12CommandQueue>(m_Device.Get(), CommandQueueType::Compute, L"Compute Queue");
        m_CopyQueue = stl::make_tunique<DX12CommandQueue>(m_Device.Get(), CommandQueueType::Copy, L"Copy Queue");
    }

    void DX12GraphicsDevice::init_descriptor_heaps() {
        m_CbvSrvUavHeap =
            stl::make_tunique<DX12DescriptorHeap>(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10000, L"CBV_SRV_UAV Heap");
        m_RtvHeap = stl::make_tunique<DX12DescriptorHeap>(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 50, L"RTV Heap");
        m_DsvHeap = stl::make_tunique<DX12DescriptorHeap>(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 50, L"DSV Heap");
        m_SamplerHeap = stl::make_tunique<DX12DescriptorHeap>(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024, L"Sampler Heap");
    }

    void DX12GraphicsDevice::init_memory_allocator() {
        m_MemoryAllocator = stl::make_tunique<DX12MemoryAllocator>(m_Device.Get(), m_Adapter.Get());
    }

    void DX12GraphicsDevice::init_contexts() {
        for (u32 i = 0; i < m_FramesInFlight; ++i) {
            m_GraphicsContexts[i] = stl::make_tunique<DX12GraphicsContext>(this);
        }
        m_ComputeContext = stl::make_tunique<DX12ComputeContext>(this);
        m_CopyContext = stl::make_tunique<DX12CopyContext>(this);
    }

    void DX12GraphicsDevice::init_bindless_root_signature() {
        auto shader_result =
            sf::render::dx12::compile(sf::render::dx12::ShaderType::RootSignature, L"assets/shaders/bindless_rs.hlsl", L"VS", true);
        if (!shader_result.root_signature_blob.Get()) {
            CORE_ERROR("Failed to compile bindless root signature");
            return;
        }
        dx12_check(m_Device->CreateRootSignature(0, shader_result.root_signature_blob->GetBufferPointer(),
                                                 shader_result.root_signature_blob->GetBufferSize(),
                                                 IID_PPV_ARGS(&m_BindlessRootSignature)));
        m_BindlessRootSignature->SetName(L"Bindless Root Signature");
    }

    void DX12GraphicsDevice::init_swapchain_resources(const SwapchainCreationDesc& desc) {
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {.Width = desc.width,
                                                .Height = desc.height,
                                                .Format = to_dxgi_format(desc.format),
                                                .Stereo = FALSE,
                                                .SampleDesc = {.Count = 1, .Quality = 0},
                                                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                                                .BufferCount = desc.buffer_count,
                                                .Scaling = DXGI_SCALING_STRETCH,
                                                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
                                                .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
                                                .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING};
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;
        dx12_check(m_Factory->CreateSwapChainForHwnd(m_DirectQueue->get_d3d12_queue(), static_cast<HWND>(m_WindowHandle), &swapchain_desc,
                                                     nullptr, nullptr, &swapchain1));
        dx12_check(m_Factory->MakeWindowAssociation(static_cast<HWND>(m_WindowHandle), DXGI_MWA_NO_ALT_ENTER));
        dx12_check(swapchain1.As(&m_Swapchain));
        m_CurrentBackBufferIndex = m_Swapchain->GetCurrentBackBufferIndex();
        create_backbuffer_rtvs();
    }

    void DX12GraphicsDevice::create_backbuffer_rtvs() {
        for (u32 i = 0; i < m_BackBufferCount; ++i) {
            ID3D12Resource* backbuffer_resource = nullptr;
            dx12_check(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer_resource)));
            m_BackBuffers[i].resource = backbuffer_resource;
            m_BackBuffers[i].width = m_WindowWidth;
            m_BackBuffers[i].height = m_WindowHeight;
            m_BackBuffers[i].format = m_BackBufferFormat;
            m_BackBuffers[i].rtv_index = m_RtvHeap->allocate_rtv(m_BackBuffers[i]);
            backbuffer_resource->SetName((L"Backbuffer " + std::to_wstring(i)).c_str());
        }
    }

    stl::result<> DX12GraphicsDevice::initialize_descriptor_heaps() {
        // DX12 descriptor heaps are already created and ready to use
        // Unlike Vulkan, DX12 doesn't require explicit descriptor set allocation
        // The heaps are already bound and accessible via the root signature
        CORE_INFO("DX12 descriptor heaps are ready (no additional initialization needed)");
        return stl::success;
    }
} // namespace sf::render::dx12
