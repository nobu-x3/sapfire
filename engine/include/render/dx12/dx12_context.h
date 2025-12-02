#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS

#include <d3d12.h>
#include <d3dx12_barriers.h>
#include <wrl/client.h>
#include "render/i_context.h"

namespace sf::render::dx12 {

    class DX12GraphicsDevice;

    constexpr u32 NUMBER_32_BIT_CONSTANTS = 64;

    // Base DX12 Context

    class DX12Context : public IContext {
    public:
        DX12Context() = default;
        virtual ~DX12Context() override = default;

        // IContext implementation
        void reset() override;
        void close() override;

        void add_resource_barrier(const ResourceBarrier& barrier) override;
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override;
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override;
        void execute_resource_barriers() override;

        void* get_native_command_list() override { return m_CommandList.Get(); }

        // DX12-specific
        ID3D12GraphicsCommandList1* get_d3d12_command_list() const { return m_CommandList.Get(); }
        ID3D12CommandAllocator* get_d3d12_allocator() const { return m_CommandAllocator.Get(); }

    protected:
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> m_CommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        stl::tvector<CD3DX12_RESOURCE_BARRIER> m_ResourceBarriers;
    };

    // DX12 Graphics Context

    class DX12GraphicsContext : public DX12Context, public IGraphicsContext {
    public:
        explicit DX12GraphicsContext(DX12GraphicsDevice* device);
        ~DX12GraphicsContext() override = default;

        // IContext implementation (inherited from DX12Context)
        void reset() override;
        void close() override { DX12Context::close(); }

        void add_resource_barrier(const ResourceBarrier& barrier) override { DX12Context::add_resource_barrier(barrier); }
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override {
            DX12Context::transition_barrier(buffer, before, after);
        }
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override {
            DX12Context::transition_barrier(texture, before, after);
        }
        void execute_resource_barriers() override { DX12Context::execute_resource_barriers(); }

        void* get_native_command_list() override { return DX12Context::get_native_command_list(); }

        // IGraphicsContext implementation
        void clear_render_target_view(Texture& texture, stl::span<f32, 4> clear_color) override;
        void clear_depth_stencil_view(Texture& texture, f32 depth = 1.0f, u8 stencil = 0) override;

        void set_pipeline_state(IPipelineState* pipeline) override;
        void set_root_signature() override;
        void set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset = 0) override;

        void set_descriptor_heaps(stl::span<IDescriptorHeap*> heaps) override;

        void set_viewport(const Viewport& viewport) override;
        void set_scissor_rect(const ScissorRect& scissor) override;

        void set_render_target(Texture& render_target, Texture* depth_stencil = nullptr) override;
        void set_render_targets(stl::span<Texture*> render_targets, Texture* depth_stencil = nullptr) override;

        void set_index_buffer(Buffer& buffer, Format format = Format::R32_UINT) override;

        void set_primitive_topology(PrimitiveTopology topology) override;

        void draw(u32 vertex_count, u32 instance_count = 1, u32 start_vertex = 0, u32 start_instance = 0) override;
        void draw_indexed(u32 index_count, u32 instance_count = 1, u32 start_index = 0, i32 base_vertex = 0,
                          u32 start_instance = 0) override;
        void draw_indexed_instanced(u32 index_count_per_instance, u32 instance_count, u32 start_index = 0, i32 base_vertex = 0,
                                    u32 start_instance = 0) override;

    private:
        DX12GraphicsDevice* m_Device;
    };

    // DX12 Compute Context

    class DX12ComputeContext : public DX12Context, public IComputeContext {
    public:
        explicit DX12ComputeContext(DX12GraphicsDevice* device);
        ~DX12ComputeContext() override = default;

        // IContext implementation (inherited from DX12Context)
        void reset() override;
        void close() override { DX12Context::close(); }

        void add_resource_barrier(const ResourceBarrier& barrier) override { DX12Context::add_resource_barrier(barrier); }
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override {
            DX12Context::transition_barrier(buffer, before, after);
        }
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override {
            DX12Context::transition_barrier(texture, before, after);
        }
        void execute_resource_barriers() override { DX12Context::execute_resource_barriers(); }

        void* get_native_command_list() override { return DX12Context::get_native_command_list(); }

        // IComputeContext implementation
        void set_pipeline_state(IPipelineState* pipeline) override;
        void set_root_signature() override;
        void set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset = 0) override;

        void set_descriptor_heaps(stl::span<IDescriptorHeap*> heaps) override;

        void dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) override;

    private:
        DX12GraphicsDevice* m_Device;
    };

    // DX12 Copy Context

    class DX12CopyContext : public DX12Context, public ICopyContext {
    public:
        explicit DX12CopyContext(DX12GraphicsDevice* device);
        ~DX12CopyContext() override = default;

        // IContext implementation (inherited from DX12Context)
        void reset() override { DX12Context::reset(); }
        void close() override { DX12Context::close(); }

        void add_resource_barrier(const ResourceBarrier& barrier) override { DX12Context::add_resource_barrier(barrier); }
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override {
            DX12Context::transition_barrier(buffer, before, after);
        }
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override {
            DX12Context::transition_barrier(texture, before, after);
        }
        void execute_resource_barriers() override { DX12Context::execute_resource_barriers(); }

        void* get_native_command_list() override { return DX12Context::get_native_command_list(); }

        // ICopyContext implementation
        void copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset = 0, u64 src_offset = 0) override;
        void copy_texture(Texture& dst, Texture& src) override;
        void copy_buffer_to_texture(Texture& dst, Buffer& src, u32 subresource = 0) override;

    private:
        DX12GraphicsDevice* m_Device;
    };

} // namespace sf::render::dx12
#endif