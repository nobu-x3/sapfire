#include "engpch.h"
#include "render/dx12/dx12_context.h"
#include "render/dx12/dx12_graphics_device.h"
#include "render/dx12/dx12_descriptor_heap.h"
#include "render/dx12/dx12_pipeline_state.h"
#include "render/dx12/dx12_type_conversions.h"
#include "render/dx12/dx12_util.h"

namespace sf::render::dx12 {

// ============================================================================
// DX12Context Implementation
// ============================================================================

void DX12Context::reset() {
    dx12_check(m_CommandAllocator->Reset());
    dx12_check(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
}

void DX12Context::close() {
    dx12_check(m_CommandList->Close());
}

void DX12Context::add_resource_barrier(const ResourceBarrier& barrier) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(barrier.resource);
    D3D12_RESOURCE_STATES before = to_d3d12_resource_state(barrier.state_before);
    D3D12_RESOURCE_STATES after = to_d3d12_resource_state(barrier.state_after);
    m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after));
}

void DX12Context::transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(buffer.resource);
    D3D12_RESOURCE_STATES before_state = to_d3d12_resource_state(before);
    D3D12_RESOURCE_STATES after_state = to_d3d12_resource_state(after);
    m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, before_state, after_state));
}

void DX12Context::transition_barrier(Texture& texture, ResourceState before, ResourceState after) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(texture.resource);
    D3D12_RESOURCE_STATES before_state = to_d3d12_resource_state(before);
    D3D12_RESOURCE_STATES after_state = to_d3d12_resource_state(after);
    m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, before_state, after_state));
}

void DX12Context::execute_resource_barriers() {
    if (!m_ResourceBarriers.empty()) {
        m_CommandList->ResourceBarrier(static_cast<u32>(m_ResourceBarriers.size()), m_ResourceBarriers.data());
        m_ResourceBarriers.clear();
    }
}

// ============================================================================
// DX12GraphicsContext Implementation
// ============================================================================

DX12GraphicsContext::DX12GraphicsContext(DX12GraphicsDevice* device)
    : m_Device(device) {
    ID3D12Device* d3d_device = device->get_d3d12_device();
    dx12_check(d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
    dx12_check(d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
    set_descriptor_heaps();
    dx12_check(m_CommandList->Close());
}

void DX12GraphicsContext::reset() {
    DX12Context::reset();
    set_descriptor_heaps();
}

void DX12GraphicsContext::clear_render_target_view(Texture& texture, stl::span<f32, 4> clear_color) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = m_Device->get_dx12_rtv_heap()->get_cpu_descriptor_handle(texture.rtv_index);
    m_CommandList->ClearRenderTargetView(rtv_handle, clear_color.data(), 0, nullptr);
}

void DX12GraphicsContext::clear_depth_stencil_view(Texture& texture, f32 depth, u8 stencil) {
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_Device->get_dx12_dsv_heap()->get_cpu_descriptor_handle(texture.dsv_index);
    m_CommandList->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

void DX12GraphicsContext::set_pipeline_state(IPipelineState* pipeline) {
    DX12PipelineState* dx12_pso = static_cast<DX12PipelineState*>(pipeline);
    m_CommandList->SetPipelineState(dx12_pso->get_d3d12_pso());
}

void DX12GraphicsContext::set_root_signature() {
    ID3D12RootSignature* root_sig = m_Device->get_bindless_root_signature();
    m_CommandList->SetGraphicsRootSignature(root_sig);
}

void DX12GraphicsContext::set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset) {
    m_CommandList->SetGraphicsRoot32BitConstants(0, num_32bit_values, data, offset);
}

void DX12GraphicsContext::set_descriptor_heaps() {
    stl::array<ID3D12DescriptorHeap*, 2> heaps = {
        m_Device->get_dx12_cbv_srv_uav_heap()->get_d3d12_heap(),
        m_Device->get_dx12_sampler_heap()->get_d3d12_heap()
    };
    m_CommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
}

void DX12GraphicsContext::set_viewport(const Viewport& viewport) {
    D3D12_VIEWPORT vp = {
        .TopLeftX = viewport.x,
        .TopLeftY = viewport.y,
        .Width = viewport.width,
        .Height = viewport.height,
        .MinDepth = viewport.min_depth,
        .MaxDepth = viewport.max_depth
    };
    m_CommandList->RSSetViewports(1, &vp);
}

void DX12GraphicsContext::set_scissor_rect(const ScissorRect& scissor) {
    D3D12_RECT rect = {
        .left = scissor.left,
        .top = scissor.top,
        .right = scissor.right,
        .bottom = scissor.bottom
    };
    m_CommandList->RSSetScissorRects(1, &rect);
}

void DX12GraphicsContext::set_render_target(Texture& render_target, Texture* depth_stencil) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = m_Device->get_dx12_rtv_heap()->get_cpu_descriptor_handle(render_target.rtv_index);

    if (depth_stencil) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_Device->get_dx12_dsv_heap()->get_cpu_descriptor_handle(depth_stencil->dsv_index);
        m_CommandList->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
    } else {
        m_CommandList->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
    }
}

void DX12GraphicsContext::set_render_targets(stl::span<Texture*> render_targets, Texture* depth_stencil) {
    stl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handles;
    rtv_handles.reserve(render_targets.size());

    for (Texture* rt : render_targets) {
        rtv_handles.push_back(m_Device->get_dx12_rtv_heap()->get_cpu_descriptor_handle(rt->rtv_index));
    }

    if (depth_stencil) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_Device->get_dx12_dsv_heap()->get_cpu_descriptor_handle(depth_stencil->dsv_index);
        m_CommandList->OMSetRenderTargets(static_cast<UINT>(rtv_handles.size()), rtv_handles.data(), FALSE, &dsv_handle);
    } else {
        m_CommandList->OMSetRenderTargets(static_cast<UINT>(rtv_handles.size()), rtv_handles.data(), FALSE, nullptr);
    }
}

void DX12GraphicsContext::set_index_buffer(Buffer& buffer, Format format) {
    D3D12_INDEX_BUFFER_VIEW ibv = {
        .BufferLocation = static_cast<ID3D12Resource*>(buffer.resource)->GetGPUVirtualAddress(),
        .SizeInBytes = static_cast<UINT>(buffer.size_in_bytes),
        .Format = to_dxgi_format(format)
    };
    m_CommandList->IASetIndexBuffer(&ibv);
}

void DX12GraphicsContext::set_primitive_topology(PrimitiveTopology topology) {
    m_CommandList->IASetPrimitiveTopology(to_d3d12_primitive_topology(topology));
}

void DX12GraphicsContext::draw(u32 vertex_count, u32 instance_count, u32 start_vertex, u32 start_instance) {
    m_CommandList->DrawInstanced(vertex_count, instance_count, start_vertex, start_instance);
}

void DX12GraphicsContext::draw_indexed(u32 index_count, u32 instance_count, u32 start_index, i32 base_vertex, u32 start_instance) {
    m_CommandList->DrawIndexedInstanced(index_count, instance_count, start_index, base_vertex, start_instance);
}

void DX12GraphicsContext::draw_indexed_instanced(u32 index_count_per_instance, u32 instance_count, u32 start_index, i32 base_vertex, u32 start_instance) {
    m_CommandList->DrawIndexedInstanced(index_count_per_instance, instance_count, start_index, base_vertex, start_instance);
}

// ============================================================================
// DX12ComputeContext Implementation
// ============================================================================

DX12ComputeContext::DX12ComputeContext(DX12GraphicsDevice* device)
    : m_Device(device) {
    ID3D12Device* d3d_device = device->get_d3d12_device();
    dx12_check(d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_CommandAllocator)));
    dx12_check(d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
    set_descriptor_heaps();
    dx12_check(m_CommandList->Close());
}

void DX12ComputeContext::reset() {
    DX12Context::reset();
    set_descriptor_heaps();
}

void DX12ComputeContext::set_pipeline_state(IPipelineState* pipeline) {
    DX12PipelineState* dx12_pso = static_cast<DX12PipelineState*>(pipeline);
    m_CommandList->SetPipelineState(dx12_pso->get_d3d12_pso());
}

void DX12ComputeContext::set_root_signature() {
    ID3D12RootSignature* root_sig = m_Device->get_bindless_root_signature();
    m_CommandList->SetComputeRootSignature(root_sig);
}

void DX12ComputeContext::set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset) {
    m_CommandList->SetComputeRoot32BitConstants(0, num_32bit_values, data, offset);
}

void DX12ComputeContext::set_descriptor_heaps() {
    stl::array<ID3D12DescriptorHeap*, 2> heaps = {
        m_Device->get_dx12_cbv_srv_uav_heap()->get_d3d12_heap(),
        m_Device->get_dx12_sampler_heap()->get_d3d12_heap()
    };
    m_CommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
}

void DX12ComputeContext::dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) {
    m_CommandList->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

// ============================================================================
// DX12CopyContext Implementation
// ============================================================================

DX12CopyContext::DX12CopyContext(DX12GraphicsDevice* device)
    : m_Device(device) {
    ID3D12Device* d3d_device = device->get_d3d12_device();
    dx12_check(d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_CommandAllocator)));
    dx12_check(d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
    dx12_check(m_CommandList->Close());
}

void DX12CopyContext::copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset, u64 src_offset) {
    ID3D12Resource* dst_resource = static_cast<ID3D12Resource*>(dst.resource);
    ID3D12Resource* src_resource = static_cast<ID3D12Resource*>(src.resource);
    m_CommandList->CopyBufferRegion(dst_resource, dst_offset, src_resource, src_offset, size);
}

void DX12CopyContext::copy_texture(Texture& dst, Texture& src) {
    ID3D12Resource* dst_resource = static_cast<ID3D12Resource*>(dst.resource);
    ID3D12Resource* src_resource = static_cast<ID3D12Resource*>(src.resource);
    m_CommandList->CopyResource(dst_resource, src_resource);
}

void DX12CopyContext::copy_buffer_to_texture(Texture& dst, Buffer& src, u32 subresource) {
    ID3D12Resource* dst_resource = static_cast<ID3D12Resource*>(dst.resource);
    ID3D12Resource* src_resource = static_cast<ID3D12Resource*>(src.resource);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    u64 row_size_in_bytes = 0;
    u64 total_bytes = 0;

    D3D12_RESOURCE_DESC desc = dst_resource->GetDesc();
    m_Device->get_d3d12_device()->GetCopyableFootprints(&desc, subresource, 1, 0, &layout, nullptr, &row_size_in_bytes, &total_bytes);

    D3D12_TEXTURE_COPY_LOCATION dst_location = {
        .pResource = dst_resource,
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = subresource
    };

    D3D12_TEXTURE_COPY_LOCATION src_location = {
        .pResource = src_resource,
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = layout
    };

    m_CommandList->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
}

} // namespace sf::render::dx12
