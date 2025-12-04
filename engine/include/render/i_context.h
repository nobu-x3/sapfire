#pragma once

#include <span>
#include "core/core.h"
#include "render_api.h"
#include "resource_types.h"

namespace sf::render {

    class IPipelineState;
    class IRenderPass;
    class IFramebuffer;
    class IDescriptorSet;

    // Base Context Interface (Command List Wrapper)

    class IContext {
    public:
        virtual ~IContext() = default;

        // Command list lifecycle
        virtual stl::result<> reset() = 0;
        virtual stl::result<> close() = 0;

        // Backend-specific handle (for advanced usage)
        virtual void* get_native_command_list() = 0;
    };

    // Graphics Context Interface

    class IGraphicsContext : public IContext {
    public:
        virtual ~IGraphicsContext() = default;

        // Render pass management
        virtual void begin_render_pass(IRenderPass* render_pass, IFramebuffer* framebuffer) = 0;
        virtual void end_render_pass() = 0;

        // Clear operations (must be called within render pass)
        virtual void clear_render_target(u32 attachment_index, stl::span<f32, 4> clear_color) = 0;
        virtual void clear_depth_stencil(f32 depth = 1.0f, u8 stencil = 0) = 0;

        // Pipeline state
        virtual void bind_pipeline(IPipelineState* pipeline) = 0;
        virtual void bind_descriptor_set(u32 set_index, IDescriptorSet* descriptor_set) = 0;
        virtual void push_constants(const void* data, u32 size, u32 offset = 0) = 0;

        // Viewport and scissor
        virtual void set_viewport(const Viewport& viewport) = 0;
        virtual void set_scissor(const ScissorRect& scissor) = 0;

        // Vertex/index buffers
        virtual void bind_vertex_buffer(u32 binding, Buffer& buffer, u64 offset = 0) = 0;
        virtual void bind_index_buffer(Buffer& buffer, Format format = Format::R32_UINT, u64 offset = 0) = 0;

        // Primitive topology
        virtual void set_primitive_topology(PrimitiveTopology topology) = 0;

        // Draw commands
        virtual void draw(u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) = 0;
        virtual void draw_indexed(u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0,
                                  u32 first_instance = 0) = 0;

        // Resource barriers
        virtual void pipeline_barrier(const PipelineBarrier& barrier) = 0;
        virtual void transition_image_layout(Texture& texture, ResourceState old_state, ResourceState new_state) = 0;
        virtual void transition_buffer_state(Buffer& buffer, ResourceState old_state, ResourceState new_state) = 0;
    };

    // Compute Context Interface

    class IComputeContext : public IContext {
    public:
        virtual ~IComputeContext() = default;

        // Pipeline state
        virtual void bind_pipeline(IPipelineState* pipeline) = 0;
        virtual void bind_descriptor_set(u32 set_index, IDescriptorSet* descriptor_set) = 0;
        virtual void push_constants(const void* data, u32 size, u32 offset = 0) = 0;

        // Dispatch
        virtual void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z) = 0;

        // Resource barriers
        virtual void pipeline_barrier(const PipelineBarrier& barrier) = 0;
    };

    // Copy Context Interface

    class ICopyContext : public IContext {
    public:
        virtual ~ICopyContext() = default;

        // Copy operations
        virtual void copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset = 0, u64 src_offset = 0) = 0;
        virtual void copy_texture(Texture& dst, Texture& src) = 0;
        virtual void copy_buffer_to_texture(Texture& dst, Buffer& src, const BufferTextureCopy& region) = 0;
        virtual void copy_texture_to_buffer(Buffer& dst, Texture& src, const BufferTextureCopy& region) = 0;
    };

} // namespace sf::render
