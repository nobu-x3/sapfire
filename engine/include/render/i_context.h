#pragma once

#include "core/base.h"
#include "render_api.h"
#include "resource_types.h"
#include <span>

namespace sf::render {

class IPipelineState;

// ============================================================================
// Base Context Interface (Command List Wrapper)
// ============================================================================

class IContext {
public:
    virtual ~IContext() = default;

    // Command list lifecycle
    virtual void reset() = 0;
    virtual void close() = 0;

    // Resource barriers
    virtual void add_resource_barrier(const ResourceBarrier& barrier) = 0;
    virtual void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) = 0;
    virtual void transition_barrier(Texture& texture, ResourceState before, ResourceState after) = 0;
    virtual void execute_resource_barriers() = 0;

    // Backend-specific handle (for advanced usage)
    virtual void* get_native_command_list() = 0;
};

// ============================================================================
// Graphics Context Interface
// ============================================================================

class IGraphicsContext : public IContext {
public:
    virtual ~IGraphicsContext() = default;

    // Clear operations
    virtual void clear_render_target_view(Texture& texture, stl::span<f32, 4> clear_color) = 0;
    virtual void clear_depth_stencil_view(Texture& texture, f32 depth = 1.0f, u8 stencil = 0) = 0;

    // Pipeline state
    virtual void set_pipeline_state(IPipelineState* pipeline) = 0;
    virtual void set_root_signature() = 0;
    virtual void set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset = 0) = 0;

    // Descriptor heaps
    virtual void set_descriptor_heaps() = 0;

    // Viewport and scissor
    virtual void set_viewport(const Viewport& viewport) = 0;
    virtual void set_scissor_rect(const ScissorRect& scissor) = 0;

    // Render targets
    virtual void set_render_target(Texture& render_target, Texture* depth_stencil = nullptr) = 0;
    virtual void set_render_targets(stl::span<Texture*> render_targets, Texture* depth_stencil = nullptr) = 0;

    // Index buffer
    virtual void set_index_buffer(Buffer& buffer, Format format = Format::R32_UINT) = 0;

    // Primitive topology
    virtual void set_primitive_topology(PrimitiveTopology topology) = 0;

    // Draw commands
    virtual void draw(u32 vertex_count, u32 instance_count = 1, u32 start_vertex = 0, u32 start_instance = 0) = 0;
    virtual void draw_indexed(u32 index_count, u32 instance_count = 1, u32 start_index = 0, i32 base_vertex = 0, u32 start_instance = 0) = 0;
    virtual void draw_indexed_instanced(u32 index_count_per_instance, u32 instance_count, u32 start_index = 0, i32 base_vertex = 0, u32 start_instance = 0) = 0;
};

// ============================================================================
// Compute Context Interface
// ============================================================================

class IComputeContext : public IContext {
public:
    virtual ~IComputeContext() = default;

    // Pipeline state
    virtual void set_pipeline_state(IPipelineState* pipeline) = 0;
    virtual void set_root_signature() = 0;
    virtual void set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset = 0) = 0;

    // Descriptor heaps
    virtual void set_descriptor_heaps() = 0;

    // Dispatch
    virtual void dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) = 0;
};

// ============================================================================
// Copy Context Interface
// ============================================================================

class ICopyContext : public IContext {
public:
    virtual ~ICopyContext() = default;

    // Copy operations
    virtual void copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset = 0, u64 src_offset = 0) = 0;
    virtual void copy_texture(Texture& dst, Texture& src) = 0;
    virtual void copy_buffer_to_texture(Texture& dst, Buffer& src, u32 subresource = 0) = 0;
};

} // namespace sf::render
