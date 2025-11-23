#pragma once
#include "render/i_graphics_device.h"
#include "render/vulkan/vk_command_queue.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_descriptor_heap.h"
#include "render/vulkan/vk_memory_allocator.h"
#include "render/vulkan/vk_pipeline_state.h"

#include <vulkan/vulkan.h>

namespace sf::render::vk {

class VkGraphicsDevice : public IGraphicsDevice {
public:
    explicit VkGraphicsDevice(const SwapchainCreationDesc& desc);
    ~VkGraphicsDevice() override;

    // Frame Management

    void begin_frame() override;
    void end_frame() override;
    void present() override;
    void wait_for_idle() override;

    // Window / Swapchain Management

    void resize_window(u32 width, u32 height) override;
    u32 get_window_width() const override { return m_WindowWidth; }
    u32 get_window_height() const override { return m_WindowHeight; }
    Texture& get_current_back_buffer() override;
    Texture& get_back_buffer(u32 index) override;
    u32 get_current_back_buffer_index() const override { return m_CurrentBackBufferIndex; }
    u32 get_back_buffer_count() const override { return m_BackBufferCount; }

    // Resource Creation

    Buffer create_buffer(const BufferCreationDesc& desc) override;
    Buffer create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) override;

    Texture create_texture(const TextureCreationDesc& desc) override;
    Texture create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) override;

    IPipelineState* create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) override;
    IPipelineState* create_compute_pipeline(const ComputePipelineStateDesc& desc) override;

    // Context Access

    IGraphicsContext& get_current_graphics_context() override;
    IGraphicsContext& get_graphics_context(u32 frame_index) override;
    IComputeContext& get_compute_context() override;
    ICopyContext& get_copy_context() override;

    // Command Queue Access

    ICommandQueue* get_direct_queue() override { return m_GraphicsQueue.get(); }
    ICommandQueue* get_compute_queue() override { return m_ComputeQueue.get(); }
    ICommandQueue* get_copy_queue() override { return m_TransferQueue.get(); }

    // Descriptor Heap Access

    IDescriptorHeap* get_cbv_srv_uav_heap() override { return m_DescriptorHeap.get(); }
    IDescriptorHeap* get_rtv_heap() override { return m_DescriptorHeap.get(); }
    IDescriptorHeap* get_dsv_heap() override { return m_DescriptorHeap.get(); }
    IDescriptorHeap* get_sampler_heap() override { return m_SamplerHeap.get(); }

    // Memory Allocator Access

    IMemoryAllocator* get_memory_allocator() override { return m_MemoryAllocator.get(); }

    // Frame Timing

    u32 get_current_frame_index() const override { return m_CurrentFrameIndex; }
    u32 get_frames_in_flight() const override { return m_FramesInFlight; }

    // Backend Information

    RenderAPI get_api() const override { return RenderAPI::Vulkan; }
    const char* get_api_name() const override { return "Vulkan"; }
    void* get_native_device() override { return reinterpret_cast<void*>(m_Device); }

    // Vulkan-specific accessors

    VkInstance get_vk_instance() const { return m_Instance; }
    VkPhysicalDevice get_vk_physical_device() const { return m_PhysicalDevice; }
    VkDevice get_vk_device() const { return m_Device; }
    VkSwapchainKHR get_vk_swapchain() const { return m_Swapchain; }
    VkSurfaceKHR get_vk_surface() const { return m_Surface; }
    VkPipelineLayout get_bindless_pipeline_layout() const { return m_BindlessPipelineLayout; }
    VkRenderPass get_main_render_pass() const { return m_MainRenderPass; }

    VkDescriptorHeap* get_vk_descriptor_heap() const { return m_DescriptorHeap.get(); }
    VkDescriptorHeap* get_vk_sampler_heap() const { return m_SamplerHeap.get(); }
    VkFramebuffer get_vk_swapchain_framebuffer(u32 index) const { return m_SwapchainFramebuffers[index]; }

private:
    void init_instance();
    void init_surface(const SwapchainCreationDesc& desc);
    void init_physical_device();
    void init_logical_device();
    void init_swapchain(const SwapchainCreationDesc& desc);
    void init_command_queues();
    void init_descriptor_heaps();
    void init_memory_allocator();
    void init_contexts();
    void init_bindless_pipeline_layout();
    void init_render_pass();
    void create_swapchain_framebuffers();
    void cleanup_swapchain();

    u32 find_queue_family(VkQueueFlags flags);
    u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);

private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkPipelineLayout m_BindlessPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_BindlessDescriptorSetLayout = VK_NULL_HANDLE;
    VkRenderPass m_MainRenderPass = VK_NULL_HANDLE;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
#endif

    stl::unique_ptr<VkCommandQueue> m_GraphicsQueue;
    stl::unique_ptr<VkCommandQueue> m_ComputeQueue;
    stl::unique_ptr<VkCommandQueue> m_TransferQueue;

    stl::array<stl::unique_ptr<VkGraphicsContext>, MAX_FRAMES_IN_FLIGHT> m_GraphicsContexts;
    stl::unique_ptr<VkComputeContext> m_ComputeContext;
    stl::unique_ptr<VkCopyContext> m_CopyContext;

    stl::unique_ptr<VkDescriptorHeap> m_DescriptorHeap;
    stl::unique_ptr<VkDescriptorHeap> m_SamplerHeap;

    stl::unique_ptr<VkMemoryAllocator> m_MemoryAllocator;

    stl::vector<stl::unique_ptr<VkPipelineState>> m_PipelineStates {mem::MemTag::Render};

    stl::array<Texture, MAX_FRAMES_IN_FLIGHT> m_BackBuffers;
    stl::array<VkImageView, MAX_FRAMES_IN_FLIGHT> m_SwapchainImageViews = {VK_NULL_HANDLE};
    stl::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> m_SwapchainFramebuffers = {VK_NULL_HANDLE};
    u32 m_BackBufferCount = MAX_FRAMES_IN_FLIGHT;
    u32 m_CurrentBackBufferIndex = 0;
    Format m_BackBufferFormat;

    void* m_WindowHandle = nullptr;
    u32 m_WindowWidth = 0;
    u32 m_WindowHeight = 0;

    u32 m_CurrentFrameIndex = 0;
    u32 m_FramesInFlight = MAX_FRAMES_IN_FLIGHT;
    stl::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_InFlightFences;
    stl::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_ImageAvailableSemaphores;
    stl::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_RenderFinishedSemaphores;

    u32 m_GraphicsQueueFamily = UINT32_MAX;
    u32 m_ComputeQueueFamily = UINT32_MAX;
    u32 m_TransferQueueFamily = UINT32_MAX;

    mutable stl::recursive_mutex m_ResourceMutex;
};

} // namespace sf::render::vk
