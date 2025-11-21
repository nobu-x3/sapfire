#pragma once

#include "core/core.h"
#include "render_api.h"
#include "resource_types.h"
#include "i_command_queue.h"
#include "i_context.h"
#include "i_descriptor_heap.h"
#include "i_memory_allocator.h"
#include "i_pipeline_state.h"
#include <span>

namespace sf::render {

// ============================================================================
// Graphics Device Interface (Main Rendering Facade)
// ============================================================================

class IGraphicsDevice {
public:
    virtual ~IGraphicsDevice() = default;

    // ========================================================================
    // Frame Management
    // ========================================================================

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void present() = 0;
    virtual void wait_for_idle() = 0;

    // ========================================================================
    // Window / Swapchain Management
    // ========================================================================

    virtual void resize_window(u32 width, u32 height) = 0;
    virtual u32 get_window_width() const = 0;
    virtual u32 get_window_height() const = 0;
    virtual Texture& get_current_back_buffer() = 0;
    virtual Texture& get_back_buffer(u32 index) = 0;
    virtual u32 get_current_back_buffer_index() const = 0;
    virtual u32 get_back_buffer_count() const = 0;

    // ========================================================================
    // Resource Creation
    // ========================================================================

    // Buffer creation
    virtual Buffer create_buffer(const BufferCreationDesc& desc) = 0;

    template<typename T>
    Buffer create_buffer(const BufferCreationDesc& desc, stl::span<T> data) {
        BufferCreationDesc buffer_desc = desc;
        buffer_desc.size_in_bytes = data.size_bytes();
        return create_buffer_with_data(buffer_desc, data.data(), data.size_bytes());
    }

    virtual Buffer create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) = 0;

    // Texture creation
    virtual Texture create_texture(const TextureCreationDesc& desc) = 0;
    virtual Texture create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) = 0;

    // Pipeline state creation
    virtual IPipelineState* create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) = 0;
    virtual IPipelineState* create_compute_pipeline(const ComputePipelineStateDesc& desc) = 0;

    // ========================================================================
    // Context Access (Command Lists)
    // ========================================================================

    virtual IGraphicsContext& get_current_graphics_context() = 0;
    virtual IGraphicsContext& get_graphics_context(u32 frame_index) = 0;
    virtual IComputeContext& get_compute_context() = 0;
    virtual ICopyContext& get_copy_context() = 0;

    // ========================================================================
    // Command Queue Access
    // ========================================================================

    virtual ICommandQueue* get_direct_queue() = 0;
    virtual ICommandQueue* get_compute_queue() = 0;
    virtual ICommandQueue* get_copy_queue() = 0;

    // ========================================================================
    // Descriptor Heap Access
    // ========================================================================

    virtual IDescriptorHeap* get_cbv_srv_uav_heap() = 0;
    virtual IDescriptorHeap* get_rtv_heap() = 0;
    virtual IDescriptorHeap* get_dsv_heap() = 0;
    virtual IDescriptorHeap* get_sampler_heap() = 0;

    // ========================================================================
    // Memory Allocator Access
    // ========================================================================

    virtual IMemoryAllocator* get_memory_allocator() = 0;

    // ========================================================================
    // Frame Timing
    // ========================================================================

    virtual u32 get_current_frame_index() const = 0;
    virtual u32 get_frames_in_flight() const = 0;

    // ========================================================================
    // Backend Information
    // ========================================================================

    virtual RenderAPI get_api() const = 0;
    virtual const char* get_api_name() const = 0;
    virtual void* get_native_device() = 0;
};

} // namespace sf::render
