#pragma once

#include <span>
#include "core/core.h"
#include "i_command_queue.h"
#include "i_context.h"
#include "i_descriptor_pool.h"
#include "i_descriptor_set.h"
#include "i_fence.h"
#include "i_framebuffer.h"
#include "i_memory_allocator.h"
#include "i_pipeline_layout.h"
#include "i_pipeline_state.h"
#include "i_render_pass.h"
#include "i_sampler.h"
#include "render_api.h"
#include "resource_types.h"

namespace sf::render {

    // Graphics Device Interface (Stateless Factory)
    // The device owns only the native device handle and swapchain
    // All other resources are created via factory methods and owned by the user

    class IGraphicsDevice {
    public:
        virtual ~IGraphicsDevice() = default;

        // Descriptor pool creation
        virtual stl::result<stl::unique_ptr<IDescriptorPool>> create_descriptor_pool(const DescriptorPoolDesc& desc) = 0;

        // Swapchain operations

        virtual u32 acquire_next_image(ISemaphore* signal_semaphore) = 0;
        virtual stl::result<> present(stl::span<ISemaphore*> wait_semaphores) = 0;
        virtual stl::result<> resize_swapchain(u32 width, u32 height) = 0;

        virtual Texture& get_back_buffer(u32 index) = 0;
        virtual u32 get_back_buffer_count() const = 0;
        virtual u32 get_window_width() const = 0;
        virtual u32 get_window_height() const = 0;

        // Resource creation factories (user owns the created resources)

        virtual stl::result<stl::unique_ptr<ICommandQueue>> create_direct_queue(const char* name = "Direct Queue") = 0;
        virtual stl::result<stl::unique_ptr<ICommandQueue>> create_compute_queue(const char* name = "Compute Queue") = 0;
        virtual stl::result<stl::unique_ptr<ICommandQueue>> create_copy_queue(const char* name = "Copy Queue") = 0;

        virtual stl::result<stl::unique_ptr<IGraphicsContext>> create_graphics_context() = 0;
        virtual stl::result<stl::unique_ptr<IComputeContext>> create_compute_context() = 0;
        virtual stl::result<stl::unique_ptr<ICopyContext>> create_copy_context() = 0;

        virtual stl::result<stl::unique_ptr<IMemoryAllocator>> create_memory_allocator() = 0;

        virtual stl::result<stl::unique_ptr<IRenderPass>> create_render_pass(const RenderPassDesc& desc) = 0;
        virtual stl::result<stl::unique_ptr<IFramebuffer>> create_framebuffer(const FramebufferDesc& desc) = 0;

        virtual stl::result<stl::unique_ptr<IPipelineLayout>> create_pipeline_layout(const PipelineLayoutDesc& desc) = 0;

        // Pipeline creation
        virtual stl::result<stl::unique_ptr<IPipelineState>> create_graphics_pipeline(const GraphicsPipelineDesc& desc) = 0;
        virtual stl::result<stl::unique_ptr<IPipelineState>> create_compute_pipeline(const ComputePipelineDesc& desc) = 0;

        virtual stl::result<stl::unique_ptr<IFence>> create_fence(bool signaled, const char* name = "Fence") = 0;
        virtual stl::result<stl::unique_ptr<ISemaphore>> create_semaphore(const char* name = "Semaphore") = 0;

        virtual stl::result<stl::unique_ptr<ISampler>> create_sampler(const SamplerDesc& desc) = 0;

        // Device properties

        virtual stl::result<> wait_for_idle() = 0;
        virtual RenderAPI get_api() const = 0;
        virtual const char* get_api_name() const = 0;
        virtual void* get_native_device() = 0;
        virtual const GPUDescriptorLimits& get_descriptor_limits() const = 0;
    };

} // namespace sf::render
