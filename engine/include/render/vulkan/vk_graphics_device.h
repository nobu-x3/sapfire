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
        stl::result<> end_frame() override;
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

        stl::result<Buffer> create_buffer(const BufferCreationDesc& desc) override;
        stl::result<Buffer> create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) override;

        stl::result<Texture> create_texture(const TextureCreationDesc& desc) override;
        stl::result<Texture> create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) override;

        stl::result<IPipelineState*> create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) override;
        stl::result<IPipelineState*> create_compute_pipeline(const ComputePipelineStateDesc& desc) override;

        // Context Access

        IGraphicsContext& get_current_graphics_context() override;
        IGraphicsContext& get_graphics_context(u32 frame_index) override;
        IComputeContext& get_compute_context() override;
        ICopyContext& get_copy_context() override;

        // Command Queue Access

        inline ICommandQueue* get_direct_queue() override { return m_GraphicsQueue.get(); }
        inline ICommandQueue* get_compute_queue() override { return m_ComputeQueue.get(); }
        inline ICommandQueue* get_copy_queue() override { return m_TransferQueue.get(); }

        inline u32 get_graphics_queue_family_index() const { return m_GraphicsQueueFamily; }
        inline u32 get_compute_queue_family_index() const { return m_ComputeQueueFamily; }
        inline u32 get_transfer_queue_family_index() const { return m_TransferQueueFamily; }

        // Descriptor Heap Access

        inline IDescriptorHeap* get_cbv_srv_uav_heap() override { return m_DescriptorHeap.get(); }
        inline IDescriptorHeap* get_rtv_heap() override { return m_DescriptorHeap.get(); }
        inline IDescriptorHeap* get_dsv_heap() override { return m_DescriptorHeap.get(); }
        inline IDescriptorHeap* get_sampler_heap() override { return m_SamplerHeap.get(); }

        // Memory Allocator Access

        inline IMemoryAllocator* get_memory_allocator() override { return m_MemoryAllocator.get(); }

        // Frame Timing

        inline u32 get_current_frame_index() const override { return m_CurrentFrameIndex; }
        inline u32 get_frames_in_flight() const override { return m_FramesInFlight; }

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

        inline const VkDescriptorHeap* get_vk_descriptor_heap() const { return m_DescriptorHeap.get(); }
        inline const VkDescriptorHeap* get_vk_sampler_heap() const { return m_SamplerHeap.get(); }
        inline VkFramebuffer get_vk_swapchain_framebuffer(u32 index) const { return m_SwapchainFramebuffers[index]; }

    private:
        stl::result<> init_instance();
        stl::result<> init_surface(const SwapchainCreationDesc& desc);
        stl::result<> init_physical_device();
        stl::result<> init_logical_device();
        stl::result<> init_swapchain(const SwapchainCreationDesc& desc);
        stl::result<> init_sync_objects();
        stl::result<> init_command_queues();
        stl::result<> init_descriptor_heaps();
        stl::result<> init_memory_allocator();
        stl::result<> init_contexts();
        stl::result<> init_bindless_pipeline_layout();
        stl::result<> init_render_pass();
        stl::result<> create_swapchain_framebuffers();
        void cleanup_swapchain();

        u32 find_queue_family(VkQueueFlags flags);
        u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);

    private:
        // Hot data - accessed every frame (grouped for cache locality)
        VkDevice m_Device = VK_NULL_HANDLE;
        u32 m_CurrentFrameIndex = 0;
        u32 m_CurrentBackBufferIndex = 0;
        u32 m_FramesInFlight = MAX_FRAMES_IN_FLIGHT;
        u32 m_BackBufferCount = MAX_FRAMES_IN_FLIGHT;

        // Frequently accessed objects (now heap-allocated to avoid placement new)
        stl::unique_ptr<VkCommandQueue> m_GraphicsQueue;
        stl::unique_ptr<VkCommandQueue> m_ComputeQueue;
        stl::unique_ptr<VkCommandQueue> m_TransferQueue;
        stl::array<stl::unique_ptr<VkGraphicsContext>, MAX_FRAMES_IN_FLIGHT> m_GraphicsContexts;
        stl::unique_ptr<VkComputeContext> m_ComputeContext;
        stl::unique_ptr<VkCopyContext> m_CopyContext;
        stl::unique_ptr<VkDescriptorHeap> m_DescriptorHeap;
        stl::unique_ptr<VkDescriptorHeap> m_SamplerHeap;
        stl::unique_ptr<VkMemoryAllocator> m_MemoryAllocator;

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

        // Init-only data (cold)
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
#endif

        // Window state
        void* m_WindowHandle = nullptr;
        u32 m_WindowWidth = 0;
        u32 m_WindowHeight = 0;

        // Queue families
        u32 m_GraphicsQueueFamily = UINT32_MAX;
        u32 m_ComputeQueueFamily = UINT32_MAX;
        u32 m_TransferQueueFamily = UINT32_MAX;

        // Dynamic collections
        stl::vector<stl::unique_ptr<VkPipelineState>> m_PipelineStates{mem::MemTag::Render};

        mutable stl::recursive_mutex m_ResourceMutex;
    };

} // namespace sf::render::vk
