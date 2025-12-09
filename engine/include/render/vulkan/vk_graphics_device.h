#pragma once
#include "render/i_graphics_device.h"
#include "render/vulkan/vk_command_queue.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_descriptor_pool.h"
#include "render/vulkan/vk_memory_allocator.h"

#include <vulkan/vulkan.h>

namespace sf::render {
    // Forward declarations from sf::render namespace
    class IFence;
    class ISemaphore;
    class IRenderPass;
    class IFramebuffer;
    class IPipelineLayout;
    class IDescriptorSet;
    class ISampler;

    struct RenderPassDesc;
    struct FramebufferDesc;
    struct PipelineLayoutDesc;
    struct DescriptorSetLayout;
    struct SamplerDesc;
} // namespace sf::render

namespace sf::render::vk {

    class VulkanGraphicsDevice final : public IGraphicsDevice {
    public:
        explicit VulkanGraphicsDevice(const SwapchainCreationDesc& desc);
        ~VulkanGraphicsDevice() override;

        stl::result<> wait_for_idle() override;

        // Window / Swapchain Management

        inline u32 get_window_width() const override { return m_WindowWidth; }
        inline u32 get_window_height() const override { return m_WindowHeight; }
        Texture& get_back_buffer(u32 index) override;
        inline u32 get_back_buffer_count() const override { return m_BackBufferCount; }

        // Context Creation Factories

        stl::result<stl::unique_ptr<IGraphicsContext>> create_graphics_context() override;
        stl::result<stl::unique_ptr<IComputeContext>> create_compute_context() override;
        stl::result<stl::unique_ptr<ICopyContext>> create_copy_context() override;

        // Command Queue Creation Factories

        stl::result<stl::unique_ptr<ICommandQueue>> create_direct_queue(const char* name = "Direct Queue") override;
        stl::result<stl::unique_ptr<ICommandQueue>> create_compute_queue(const char* name = "Compute Queue") override;
        stl::result<stl::unique_ptr<ICommandQueue>> create_copy_queue(const char* name = "Copy Queue") override;

        inline u32 get_graphics_queue_family_index() const { return m_GraphicsQueueFamily; }
        inline u32 get_compute_queue_family_index() const { return m_ComputeQueueFamily; }
        inline u32 get_transfer_queue_family_index() const { return m_TransferQueueFamily; }

        // Memory Allocator Creation Factory

        stl::result<stl::unique_ptr<IMemoryAllocator>> create_memory_allocator() override;

        // Backend Information

        inline RenderAPI get_api() const override { return RenderAPI::Vulkan; }
        inline const char* get_api_name() const override { return "Vulkan"; }
        inline void* get_native_device() override { return reinterpret_cast<void*>(m_Device); }

        // Vulkan-specific accessors

        inline VkInstance get_vk_instance() const { return m_Instance; }
        inline VkPhysicalDevice get_vk_physical_device() const { return m_PhysicalDevice; }
        inline VkDevice get_vk_device() const { return m_Device; }
        inline VkSwapchainKHR get_vk_swapchain() const { return m_Swapchain; }
        inline VkSurfaceKHR get_vk_surface() const { return m_Surface; }
        inline VkRenderPass get_main_render_pass() const { return m_MainRenderPass; }
        inline VkFramebuffer get_vk_swapchain_framebuffer(u32 index) const { return m_SwapchainFramebuffers[index]; }

        inline bool is_headless() const { return m_Headless; }

        // New API - Factory methods for render pass and framebuffer
        stl::result<stl::unique_ptr<sf::render::IRenderPass>> create_render_pass(const sf::render::RenderPassDesc& desc) override;
        stl::result<stl::unique_ptr<sf::render::IFramebuffer>> create_framebuffer(const sf::render::FramebufferDesc& desc) override;
        stl::result<stl::unique_ptr<sf::render::IPipelineLayout>>
        create_pipeline_layout(const sf::render::PipelineLayoutDesc& desc) override;
        stl::result<stl::unique_ptr<sf::render::IFence>> create_fence(bool signaled, const char* name = "Fence") override;
        stl::result<stl::unique_ptr<sf::render::ISemaphore>> create_semaphore(const char* name = "Semaphore") override;
        stl::result<stl::unique_ptr<sf::render::ISampler>> create_sampler(const sf::render::SamplerDesc& desc) override;

        // Pipeline creation
        stl::result<stl::unique_ptr<sf::render::IPipelineState>>
        create_graphics_pipeline(const sf::render::GraphicsPipelineDesc& desc) override;
        stl::result<stl::unique_ptr<sf::render::IPipelineState>>
        create_compute_pipeline(const sf::render::ComputePipelineDesc& desc) override;

        // Swapchain operations with explicit sync
        u32 acquire_next_image(sf::render::ISemaphore* signal_semaphore) override;
        stl::result<> present(stl::span<sf::render::ISemaphore*> wait_semaphores) override;
        stl::result<> resize_swapchain(u32 width, u32 height) override;

        // Descriptor pool creation
        stl::result<stl::unique_ptr<IDescriptorPool>> create_descriptor_pool(const DescriptorPoolDesc& desc) override;

    private:
        stl::result<> init_instance();
        stl::result<> init_surface(const SwapchainCreationDesc& desc);
        stl::result<> init_physical_device();
        stl::result<> init_logical_device();
        stl::result<> init_swapchain(const SwapchainCreationDesc& desc);
        stl::result<> init_render_pass();
        stl::result<> create_swapchain_framebuffers();
        stl::result<> create_offscreen_render_targets(const SwapchainCreationDesc& desc);
        void cleanup_swapchain();

        u32 find_queue_family(VkQueueFlags flags);
        u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);

    private:
        // Hot data - accessed every frame (grouped for cache locality)
        VkDevice m_Device = VK_NULL_HANDLE;
        u32 m_CurrentBackBufferIndex = 0;
        u32 m_BackBufferCount = MAX_FRAMES_IN_FLIGHT;

        // Back buffers
        stl::array<Texture, MAX_FRAMES_IN_FLIGHT> m_BackBuffers;
        stl::array<VkImageView, MAX_FRAMES_IN_FLIGHT> m_SwapchainImageViews = {VK_NULL_HANDLE};
        stl::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> m_SwapchainFramebuffers = {VK_NULL_HANDLE};
        Format m_BackBufferFormat;

        // Swapchain and rendering
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkRenderPass m_MainRenderPass = VK_NULL_HANDLE;

        // Init-only data (cold)
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VmaAllocator m_InternalAllocator = VK_NULL_HANDLE; // For device-owned backbuffers only

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
#endif

        // Window state
        void* m_WindowHandle = nullptr;
        u32 m_WindowWidth = 0;
        u32 m_WindowHeight = 0;
        bool m_Headless = false; // True for offscreen rendering (editor viewports)

        // Queue families
        u32 m_GraphicsQueueFamily = UINT32_MAX;
        u32 m_ComputeQueueFamily = UINT32_MAX;
        u32 m_TransferQueueFamily = UINT32_MAX;
    };

} // namespace sf::render::vk
