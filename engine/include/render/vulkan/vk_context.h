#pragma once
#include <vulkan/vulkan.h>
#include "render/i_context.h"

namespace sf::render {
    class IDescriptorHeap;
}
namespace sf::render::vk {

    class VulkanGraphicsDevice;

    constexpr u32 NUMBER_32_BIT_CONSTANTS = 64;

    class VulkanContext : public IContext {
    public:
        VulkanContext(VulkanGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags = 0);

        void destroy_resources();

        stl::result<> init(VulkanGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags = 0);
        stl::result<> reset() override;
        stl::result<> close() override;

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

    class VulkanGraphicsContext final : public VulkanContext, public IGraphicsContext {
    public:
        explicit VulkanGraphicsContext(VulkanGraphicsDevice* device);

        stl::result<> reset() override;
        stl::result<> close() override;

        void* get_native_command_list() override { return VulkanContext::get_native_command_list(); }

        void begin_render_pass(sf::render::IRenderPass* render_pass, sf::render::IFramebuffer* framebuffer) override;
        void end_render_pass() override;
        void clear_render_target(u32 attachment_index, stl::span<f32, 4> clear_color) override;
        void clear_depth_stencil(f32 depth = 1.0f, u8 stencil = 0) override;
        void bind_pipeline(IPipelineState* pipeline) override;
        void bind_descriptor_set(u32 set_index, sf::render::IDescriptorSet* descriptor_set) override;
        void push_constants(const void* data, u32 size, u32 offset = 0) override;
        void set_scissor(const ScissorRect& scissor) override;
        void set_viewport(const Viewport& viewport) override;
        void bind_vertex_buffer(u32 binding, Buffer& buffer, u64 offset = 0) override;
        void bind_index_buffer(Buffer& buffer, Format format = Format::R32_UINT, u64 offset = 0) override;
        void set_primitive_topology(PrimitiveTopology topology) override;

        void draw(u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) override;
        void draw_indexed(u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0,
                          u32 first_instance = 0) override;
        void pipeline_barrier(const PipelineBarrier& barrier) override;
        void transition_image_layout(Texture& texture, ResourceState old_state, ResourceState new_state) override;
        void transition_buffer_state(Buffer& buffer, ResourceState old_state, ResourceState new_state) override;

    private:
        VulkanGraphicsDevice* m_DevicePtr{nullptr};
        VkRenderPass m_CurrentRenderPass = VK_NULL_HANDLE;
        VkFramebuffer m_CurrentFramebuffer = VK_NULL_HANDLE;
        VkPipelineLayout m_CurrentPipelineLayout = VK_NULL_HANDLE;
        VkViewport m_Viewport{};
    };

    class VulkanComputeContext final : public VulkanContext, public IComputeContext {
    public:
        explicit VulkanComputeContext(VulkanGraphicsDevice* device);

        stl::result<> reset() override;
        stl::result<> close() override { return VulkanContext::close(); }

        void* get_native_command_list() override { return VulkanContext::get_native_command_list(); }

        void bind_pipeline(IPipelineState* pipeline) override;
        void bind_descriptor_set(u32 set_index, sf::render::IDescriptorSet* descriptor_set) override;
        void push_constants(const void* data, u32 size, u32 offset = 0) override;
        void dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) override;
        void pipeline_barrier(const PipelineBarrier& barrier) override;

    private:
        VulkanGraphicsDevice* m_DevicePtr{nullptr};
        VkPipelineLayout m_CurrentPipelineLayout = VK_NULL_HANDLE;
    };

    class VulkanCopyContext final : public VulkanContext, public ICopyContext {
    public:
        explicit VulkanCopyContext(VulkanGraphicsDevice* device);

        stl::result<> reset() override;
        stl::result<> close() override { return VulkanContext::close(); }

        void* get_native_command_list() override { return VulkanContext::get_native_command_list(); }

        void copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset = 0, u64 src_offset = 0) override;
        void copy_texture(Texture& dst, Texture& src) override;
        void copy_buffer_to_texture(Texture& dst, Buffer& src, const BufferTextureCopy& region) override;
        void copy_texture_to_buffer(Buffer& dst, Texture& src, const BufferTextureCopy& region) override;

    private:
        VulkanGraphicsDevice* m_DevicePtr{nullptr};
    };

} // namespace sf::render::vk
