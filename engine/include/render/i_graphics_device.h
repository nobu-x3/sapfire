#pragma once

#include <span>
#include "core/core.h"
#include "i_command_queue.h"
#include "i_context.h"
#include "i_descriptor_heap.h"
#include "i_memory_allocator.h"
#include "i_pipeline_state.h"
#include "render_api.h"
#include "resource_types.h"

namespace sf::render {

    // Descriptor Heap Creation Descriptor
    struct DescriptorHeapDesc {
        u32 descriptor_count = 1000;
        const char* name = "Descriptor Heap";
    };

    // Graphics Device Interface (Stateless Factory)
    // The device owns only the swapchain and provides factory methods for creating resources

    class IGraphicsDevice {
    public:
        virtual ~IGraphicsDevice() = default;

        // Swapchain Management (device owns this)

        virtual stl::result<> begin_frame() = 0;
        virtual stl::result<> end_frame(IGraphicsContext* context) = 0;
        virtual stl::result<> present() = 0;
        virtual stl::result<> wait_for_idle() = 0;
        virtual stl::result<> resize_window(u32 width, u32 height) = 0;

        virtual u32 get_window_width() const = 0;
        virtual u32 get_window_height() const = 0;
        virtual Texture& get_current_back_buffer() = 0;
        virtual Texture& get_back_buffer(u32 index) = 0;
        virtual u32 get_current_back_buffer_index() const = 0;
        virtual u32 get_back_buffer_count() const = 0;
        virtual u32 get_current_frame_index() const = 0;

        // Resource Creation Factories (user owns the created resources)
        virtual stl::result<IPipelineState*> create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) = 0;
        virtual stl::result<IPipelineState*> create_compute_pipeline(const ComputePipelineStateDesc& desc) = 0;

        // Context Creation Factories (user owns the contexts)
        virtual stl::result<stl::unique_ptr<IGraphicsContext>> create_graphics_context() = 0;
        virtual stl::result<stl::unique_ptr<IComputeContext>> create_compute_context() = 0;
        virtual stl::result<stl::unique_ptr<ICopyContext>> create_copy_context() = 0;

        // Command Queue Creation Factories (user owns the queues)
        virtual stl::result<stl::unique_ptr<ICommandQueue>> create_direct_queue(const char* name = "Direct Queue") = 0;
        virtual stl::result<stl::unique_ptr<ICommandQueue>> create_compute_queue(const char* name = "Compute Queue") = 0;
        virtual stl::result<stl::unique_ptr<ICommandQueue>> create_copy_queue(const char* name = "Copy Queue") = 0;

        // Descriptor Heap Creation Factories (user owns the heaps)
        virtual stl::result<stl::unique_ptr<IDescriptorHeap>> create_cbv_srv_uav_heap(const DescriptorHeapDesc& desc) = 0;
        virtual stl::result<stl::unique_ptr<IDescriptorHeap>> create_rtv_heap(const DescriptorHeapDesc& desc) = 0;
        virtual stl::result<stl::unique_ptr<IDescriptorHeap>> create_dsv_heap(const DescriptorHeapDesc& desc) = 0;
        virtual stl::result<stl::unique_ptr<IDescriptorHeap>> create_sampler_heap(const DescriptorHeapDesc& desc) = 0;

        // Memory Allocator Creation Factory (user owns the allocator)
        virtual stl::result<stl::unique_ptr<IMemoryAllocator>> create_memory_allocator() = 0;

        // Backend Information
        virtual RenderAPI get_api() const = 0;
        virtual const char* get_api_name() const = 0;
        virtual void* get_native_device() = 0;
    };

} // namespace sf::render
