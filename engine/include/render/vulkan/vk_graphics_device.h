#pragma once
#include "render/i_graphics_device.h"
#include "render/vulkan/vk_command_queue.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_descriptor_heap.h"
#include "render/vulkan/vk_memory_allocator.h"
#include "render/vulkan/vk_pipeline_state.h"

#include <vulkan/vulkan.h>

namespace sf::render::vk {

    class VkGraphicsDevice final : public IGraphicsDevice {
    public:
        explicit VkGraphicsDevice(const SwapchainCreationDesc& desc);
        ~VkGraphicsDevice() override;

        // Frame Management

        stl::result<> begin_frame() override;
        stl::result<> end_frame(IGraphicsContext* context) override;
        stl::result<> present() override;
        stl::result<> wait_for_idle() override;

        // Window / Swapchain Management

        stl::result<> resize_window(u32 width, u32 height) override;
        inline u32 get_window_width() const override { return m_WindowWidth; }
        inline u32 get_window_height() const override { return m_WindowHeight; }
        Texture& get_current_back_buffer() override;
        Texture& get_back_buffer(u32 index) override;
        inline u32 get_current_back_buffer_index() const override { return m_CurrentBackBufferIndex; }
        inline u32 get_back_buffer_count() const override { return m_BackBufferCount; }

        // Resource Creation
        stl::result<IPipelineState*> create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) override;
        stl::result<IPipelineState*> create_compute_pipeline(const ComputePipelineStateDesc& desc) override;

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

        // Descriptor Heap Creation Factories

        stl::result<stl::unique_ptr<IDescriptorHeap>> create_cbv_srv_uav_heap(const DescriptorHeapDesc& desc) override;
        stl::result<stl::unique_ptr<IDescriptorHeap>> create_rtv_heap(const DescriptorHeapDesc& desc) override;
        stl::result<stl::unique_ptr<IDescriptorHeap>> create_dsv_heap(const DescriptorHeapDesc& desc) override;
        stl::result<stl::unique_ptr<IDescriptorHeap>> create_sampler_heap(const DescriptorHeapDesc& desc) override;

        // Memory Allocator Creation Factory

        stl::result<stl::unique_ptr<IMemoryAllocator>> create_memory_allocator() override;

        // Frame Timing

        inline u32 get_current_frame_index() const override { return m_CurrentFrameIndex; }

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
        inline VkPipelineLayout get_bindless_pipeline_layout() const { return m_BindlessPipelineLayout; }
        inline VkRenderPass get_main_render_pass() const { return m_MainRenderPass; }
        inline VkFramebuffer get_vk_swapchain_framebuffer(u32 index) const { return m_SwapchainFramebuffers[index]; }

        // Swapchain synchronization primitives
        inline VkSemaphore get_image_available_semaphore() const { return m_ImageAvailableSemaphores[m_CurrentFrameIndex]; }
        inline VkSemaphore get_render_finished_semaphore() const { return m_RenderFinishedSemaphores[m_CurrentFrameIndex]; }
        inline VkFence get_in_flight_fence() const { return m_InFlightFences[m_Headless ? 0 : m_CurrentFrameIndex]; }
        inline bool is_headless() const { return m_Headless; }

        // Get descriptor set layouts for explicit initialization by user
        inline VkDescriptorSetLayout get_per_frame_descriptor_set_layout() const { return m_PerFrameDescriptorSetLayout; }
        inline VkDescriptorSetLayout get_resource_descriptor_set_layout() const { return m_ResourceDescriptorSetLayout; }
        inline VkDescriptorSetLayout get_material_descriptor_set_layout() const { return m_MaterialDescriptorSetLayout; }
        inline VkDescriptorSetLayout get_sampler_descriptor_set_layout() const { return m_DummyDescriptorSetLayout; }

    private:
        stl::result<> init_instance();
        stl::result<> init_surface(const SwapchainCreationDesc& desc);
        stl::result<> init_physical_device();
        stl::result<> init_logical_device();
        stl::result<> init_swapchain(const SwapchainCreationDesc& desc);
        stl::result<> init_sync_objects();
        stl::result<> init_bindless_pipeline_layout();
        stl::result<> init_render_pass();
        stl::result<> create_swapchain_framebuffers();
        stl::result<> create_offscreen_render_targets(const SwapchainCreationDesc& desc);
        void cleanup_swapchain();

        u32 find_queue_family(VkQueueFlags flags);
        u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);

    private:
        // Hot data - accessed every frame (grouped for cache locality)
        VkDevice m_Device = VK_NULL_HANDLE;
        u32 m_CurrentFrameIndex = 0;
        u32 m_CurrentBackBufferIndex = 0;
        u32 m_BackBufferCount = MAX_FRAMES_IN_FLIGHT;

        // Frame sync objects
        stl::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_InFlightFences;
        stl::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_ImageAvailableSemaphores;
        stl::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_RenderFinishedSemaphores;

        // Back buffers
        stl::array<Texture, MAX_FRAMES_IN_FLIGHT> m_BackBuffers;
        stl::array<VkImageView, MAX_FRAMES_IN_FLIGHT> m_SwapchainImageViews = {VK_NULL_HANDLE};
        stl::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> m_SwapchainFramebuffers = {VK_NULL_HANDLE};
        Format m_BackBufferFormat;

        // Swapchain and rendering
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkRenderPass m_MainRenderPass = VK_NULL_HANDLE;
        VkPipelineLayout m_BindlessPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_BindlessDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_PerFrameDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_ResourceDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_MaterialDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DummyDescriptorSetLayout = VK_NULL_HANDLE; // Empty layout for unused set 1

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

        // Dynamic collections
        stl::vector<stl::unique_ptr<VkPipelineState>> m_PipelineStates{mem::MemTag::Render};

        mutable stl::recursive_mutex m_ResourceMutex;
    };

} // namespace sf::render::vk
