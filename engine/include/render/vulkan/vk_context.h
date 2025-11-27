#pragma once
#include <vulkan/vulkan.h>
#include "render/i_context.h"

namespace sf::render::vk {

    class VkGraphicsDevice;

    constexpr u32 NUMBER_32_BIT_CONSTANTS = 64;

    class VkContext : public IContext {
    public:
        VkContext(VkGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags = 0);
        VkContext() = default;
        virtual ~VkContext() override;
        void destroy_resources();

        stl::result<> init(VkGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags = 0);
        stl::result<> reset() override;
        stl::result<> close() override;

        void add_resource_barrier(const ResourceBarrier& barrier) override;
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override;
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override;
        void execute_resource_barriers() override;

        void* get_native_command_list() override { return reinterpret_cast<void*>(m_CommandBuffer); }

        VkCommandBuffer get_vk_command_buffer() const { return m_CommandBuffer; }
        VkCommandPool get_vk_command_pool() const { return m_CommandPool; }

    protected:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        stl::vector<VkImageMemoryBarrier> m_ImageBarriers{mem::MemTag::Render};
        stl::vector<VkBufferMemoryBarrier> m_BufferBarriers{mem::MemTag::Render};
    };

    class VkGraphicsContext final: public VkContext, public IGraphicsContext {
    public:
        explicit VkGraphicsContext(VkGraphicsDevice* device);
        VkGraphicsContext() = default;

        stl::result<> reset() override;
        stl::result<> close() override;

        void add_resource_barrier(const ResourceBarrier& barrier) override { VkContext::add_resource_barrier(barrier); }
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override {
            VkContext::transition_barrier(buffer, before, after);
        }
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override {
            VkContext::transition_barrier(texture, before, after);
        }
        void execute_resource_barriers() override { VkContext::execute_resource_barriers(); }

        void* get_native_command_list() override { return VkContext::get_native_command_list(); }

        void clear_render_target_view(Texture& texture, stl::span<f32, 4> clear_color) override;
        void clear_depth_stencil_view(Texture& texture, f32 depth = 1.0f, u8 stencil = 0) override;

        void set_pipeline_state(IPipelineState* pipeline) override;
        void set_root_signature() override;
        void set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset = 0) override;

        void set_descriptor_heaps() override;

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
        VkGraphicsDevice* m_DevicePtr{nullptr};
        VkRenderPass m_CurrentRenderPass = VK_NULL_HANDLE;
        VkFramebuffer m_CurrentFramebuffer = VK_NULL_HANDLE;
    };

    class VkComputeContext final: public VkContext, public IComputeContext {
    public:
        explicit VkComputeContext(VkGraphicsDevice* device);
        VkComputeContext() = default;

        stl::result<> reset() override;
        stl::result<> close() override { return VkContext::close(); }

        void add_resource_barrier(const ResourceBarrier& barrier) override { VkContext::add_resource_barrier(barrier); }
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override {
            VkContext::transition_barrier(buffer, before, after);
        }
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override {
            VkContext::transition_barrier(texture, before, after);
        }
        void execute_resource_barriers() override { VkContext::execute_resource_barriers(); }

        void* get_native_command_list() override { return VkContext::get_native_command_list(); }

        void set_pipeline_state(IPipelineState* pipeline) override;
        void set_root_signature() override;
        void set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset = 0) override;

        void set_descriptor_heaps() override;

        void dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) override;

    private:
        VkGraphicsDevice* m_DevicePtr{nullptr};
    };

    // Vulkan Copy Context

    class VkCopyContext final: public VkContext, public ICopyContext {
    public:
        explicit VkCopyContext(VkGraphicsDevice* device);
        VkCopyContext() = default;

        stl::result<> reset() override;
        stl::result<> close() override { return VkContext::close(); }

        void add_resource_barrier(const ResourceBarrier& barrier) override { VkContext::add_resource_barrier(barrier); }
        void transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) override {
            VkContext::transition_barrier(buffer, before, after);
        }
        void transition_barrier(Texture& texture, ResourceState before, ResourceState after) override {
            VkContext::transition_barrier(texture, before, after);
        }
        void execute_resource_barriers() override { VkContext::execute_resource_barriers(); }

        void* get_native_command_list() override { return VkContext::get_native_command_list(); }

        void copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset = 0, u64 src_offset = 0) override;
        void copy_texture(Texture& dst, Texture& src) override;
        void copy_buffer_to_texture(Texture& dst, Buffer& src, u32 subresource = 0) override;

    private:
        VkGraphicsDevice* m_DevicePtr{nullptr};
    };

} // namespace sf::render::vk
