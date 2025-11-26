#include "engpch.h"

#include "core/logger.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_type_conversions.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <set>

namespace sf::render::vk {
    // Extension validation helpers (following vk-bootstrap pattern)
    namespace {
        bool check_extension_supported(const stl::vector<VkExtensionProperties>& available, const char* extension) {
            if (!extension)
                return false;
            for (const auto& ext : available) {
                if (strcmp(ext.extensionName, extension) == 0) {
                    return true;
                }
            }
            return false;
        }
        bool check_layer_supported(const stl::vector<VkLayerProperties>& available, const char* layer) {
            if (!layer)
                return false;
            for (const auto& l : available) {
                if (strcmp(l.layerName, layer) == 0) {
                    return true;
                }
            }
            return false;
        }
    } // namespace

#if defined(DEBUG) || defined(_DEBUG)
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                         VkDebugUtilsMessageTypeFlagsEXT type,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
        if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            CORE_WARN("[Vulkan] {}", callback_data->pMessage);
        }
        return VK_FALSE;
    }
#endif

    VkGraphicsDevice::VkGraphicsDevice(const SwapchainCreationDesc& desc) {
        m_WindowHandle = desc.window_handle;
        m_WindowWidth = desc.width;
        m_WindowHeight = desc.height;
        m_BackBufferFormat = desc.format;
        // Initialize all components and log errors if any fail
        auto instance_result = init_instance();
        if (!instance_result) {
            CORE_CRITICAL(instance_result.error().c_str());
            return;
        }
        auto surface_result = init_surface(desc);
        if (!surface_result) {
            CORE_CRITICAL(surface_result.error().c_str());
            return;
        }
        auto physical_device_result = init_physical_device();
        if (!physical_device_result) {
            CORE_CRITICAL(physical_device_result.error().c_str());
            return;
        }
        auto logical_device_result = init_logical_device();
        if (!logical_device_result) {
            CORE_CRITICAL(logical_device_result.error().c_str());
            return;
        }
        auto swapchain_result = init_swapchain(desc);
        if (!swapchain_result) {
            CORE_CRITICAL(swapchain_result.error().c_str());
            return;
        }
        auto sync_objs_result = init_sync_objects();
        if (!sync_objs_result) {
            CORE_CRITICAL(sync_objs_result.error().c_str());
            return;
        }
        auto queues_result = init_command_queues();
        if (!queues_result) {
            CORE_CRITICAL(queues_result.error().c_str());
            return;
        }
        auto heaps_result = init_descriptor_heaps();
        if (!heaps_result) {
            CORE_CRITICAL(heaps_result.error().c_str());
            return;
        }
        auto allocator_result = init_memory_allocator();
        if (!allocator_result) {
            CORE_CRITICAL(allocator_result.error().c_str());
            return;
        }
        auto render_pass_result = init_render_pass();
        if (!render_pass_result) {
            CORE_CRITICAL(render_pass_result.error().c_str());
            return;
        }
        auto pipeline_layout_result = init_bindless_pipeline_layout();
        if (!pipeline_layout_result) {
            CORE_CRITICAL(pipeline_layout_result.error().c_str());
            return;
        }
        auto framebuffers_result = create_swapchain_framebuffers();
        if (!framebuffers_result) {
            CORE_CRITICAL(framebuffers_result.error().c_str());
            return;
        }
        auto contexts_result = init_contexts();
        if (!contexts_result) {
            CORE_CRITICAL(contexts_result.error().c_str());
            return;
        }
        CORE_INFO("Vulkan graphics device initialized successfully");
    }

    VkGraphicsDevice::~VkGraphicsDevice() {
        wait_for_idle();
        cleanup_swapchain();
        // Clean up contexts (must be before destroying command pools)
        for(auto& ctx : m_GraphicsContexts) {
            ctx.reset();
        }
        std::destroy(m_GraphicsContexts.begin(), m_GraphicsContexts.end());
        m_ComputeContext.reset();
        m_ComputeContext.~VkComputeContext();
        m_CopyContext.reset();
        m_CopyContext.~VkCopyContext();
        m_PipelineStates.clear();
        m_DescriptorHeap.~VkDescriptorHeap();
        m_SamplerHeap.~VkDescriptorHeap();
        m_MemoryAllocator.~VkMemoryAllocator();
        if (m_BindlessDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_BindlessDescriptorSetLayout, nullptr);
        }
        if (m_BindlessPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_Device, m_BindlessPipelineLayout, nullptr);
        }
        if (m_MainRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_Device, m_MainRenderPass, nullptr);
        }
        for (auto& fence : m_InFlightFences) {
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(m_Device, fence, nullptr);
            }
        }
        for (auto& semaphore : m_ImageAvailableSemaphores) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, semaphore, nullptr);
            }
        }
        for (auto& semaphore : m_RenderFinishedSemaphores) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, semaphore, nullptr);
            }
        }
        // Destroy device (must be AFTER all child objects are destroyed)
        if (m_Device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_Device, nullptr);
        }
#if defined(DEBUG) || defined(_DEBUG)
        if (m_DebugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(m_Instance, m_DebugMessenger, nullptr);
            }
        }
#endif
        if (m_Surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        }
        if (m_Instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_Instance, nullptr);
        }
        CORE_INFO("Vulkan graphics device destroyed");
    }

    stl::result<> VkGraphicsDevice::init_instance() {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Sapfire Application";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "Sapfire Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_3;
        // Get required extensions from SDL3 (handles X11/Wayland/Windows automatically)
        u32 sdl_extension_count = 0;
        const char* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
        stl::vector<const char*> extensions{mem::MemTag::Temp, sdl_extension_count};
        for (u32 i = 0; i < sdl_extension_count; ++i) {
            extensions[i] = sdl_extensions[i];
        }
        // SDL already provides the correct platform surface extension (e.g., VK_KHR_wayland_surface,
        // VK_KHR_xcb_surface, VK_KHR_win32_surface). Don't add them manually!
        CORE_INFO("Requesting {} instance extensions from SDL", sdl_extension_count);
        for (u32 i = 0; i < sdl_extension_count; ++i) {
            CORE_INFO("  - {}", sdl_extensions[i]);
        }
        stl::vector<const char*> validation_layers{mem::MemTag::Temp};
#if defined(DEBUG) || defined(_DEBUG)
        u32 layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        stl::vector<VkLayerProperties> available_layers(mem::MemTag::Temp, layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
        bool validation_layer_available = false;
        for (const auto& layer : available_layers) {
            if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                validation_layer_available = true;
                break;
            }
        }
        if (validation_layer_available) {
            validation_layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            CORE_INFO("Validation layers enabled");
        } else {
            CORE_WARN("Validation layers requested but not available. Install vulkan-validation-layers package.");
        }
#endif
        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = static_cast<u32>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledLayerCount = static_cast<u32>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
        VK_RETURN_ON_ERROR(vkCreateInstance(&create_info, nullptr, &m_Instance), "Failed to create Vulkan instance");
        CORE_INFO("Vulkan instance created");
#if defined(DEBUG) || defined(_DEBUG)
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_Instance, &debug_create_info, nullptr, &m_DebugMessenger);
        }
#endif
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_surface(const SwapchainCreationDesc& desc) {
        SDL_Window* sdl_window = static_cast<SDL_Window*>(desc.window_handle);
        if (!SDL_Vulkan_CreateSurface(sdl_window, m_Instance, nullptr, &m_Surface)) {
            return stl::make_error<>("Failed to create Vulkan surface: {}", SDL_GetError());
        }
        CORE_INFO("Vulkan surface created");
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_physical_device() {
        u32 device_count = 0;
        VK_RETURN_ON_ERROR(vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr), "Failed to enumerate physical devices");
        if (device_count == 0) {
            return stl::make_error<>("Failed to find GPUs with Vulkan support");
        }
        stl::vector<VkPhysicalDevice> devices{mem::MemTag::Temp, device_count};
        vkEnumeratePhysicalDevices(m_Instance, &device_count, devices.data());
        m_PhysicalDevice = devices[0];
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        CORE_INFO("Selected GPU: {}", properties.deviceName);
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_logical_device() {
        m_GraphicsQueueFamily = find_queue_family(VK_QUEUE_GRAPHICS_BIT);
        m_ComputeQueueFamily = find_queue_family(VK_QUEUE_COMPUTE_BIT);
        m_TransferQueueFamily = find_queue_family(VK_QUEUE_TRANSFER_BIT);
        stl::vector<VkDeviceQueueCreateInfo> queue_create_infos{mem::MemTag::Temp};
        std::set<u32> unique_queue_families = {m_GraphicsQueueFamily, m_ComputeQueueFamily, m_TransferQueueFamily};
        f32 queue_priority = 1.0f;
        for (u32 queue_family : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }
        VkPhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE;
        u32 available_ext_count = 0;
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &available_ext_count, nullptr);
        stl::vector<VkExtensionProperties> available_extensions{mem::MemTag::Temp, available_ext_count};
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &available_ext_count, available_extensions.data());
        // Required extensions (following vk-bootstrap defaults)
        stl::vector<const char*> required_extensions = {mem::MemTag::Temp,
                                                        1,
                                                        {
                                                            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                        }};
        // Optional extensions (will be enabled if available, following vk-bootstrap pattern)
        stl::vector<const char*> optional_extensions = {mem::MemTag::Temp,
                                                        1,
                                                        {
                                                            // Add commonly used optional extensions here as needed
                                                            // Example: VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
                                                        }};
        stl::vector<const char*> device_extensions{mem::MemTag::Temp};
        for (const auto* ext : required_extensions) {
            if (!check_extension_supported(available_extensions, ext)) {
                return stl::make_error<>("Required device extension not available: {}", ext);
            }
            device_extensions.push_back(ext);
        }
        for (const auto* ext : optional_extensions) {
            if (check_extension_supported(available_extensions, ext)) {
                device_extensions.push_back(ext);
                CORE_INFO("Optional extension enabled: {}", ext);
            }
        }
        CORE_INFO("Enabling {} device extensions:", device_extensions.size());
        for (const auto* ext : device_extensions) {
            CORE_INFO("  - {}", ext);
        }
        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = static_cast<u32>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = static_cast<u32>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();
        VK_RETURN_ON_ERROR(vkCreateDevice(m_PhysicalDevice, &create_info, nullptr, &m_Device), "Failed to create Vulkan logical device");
        CORE_INFO("Vulkan logical device created");
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_swapchain(const SwapchainCreationDesc& desc) {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);
        u32 format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &format_count, nullptr);
        stl::vector<VkSurfaceFormatKHR> surface_formats{mem::MemTag::Temp, format_count};
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &format_count, surface_formats.data());
        // Arbitrarily first
        VkSurfaceFormatKHR surface_format = surface_formats[0];
        if (surface_format.format != to_vk_format(desc.format)) {
            // TODO: write a function that properly formats these enums to string
            CORE_WARN("Given format {} has not been choses due to device capabilities. Using {} format.", static_cast<u32>(desc.format),
                      static_cast<u32>(surface_format.format));
            m_BackBufferFormat = from_vk_format(surface_format.format);
        }
        u32 image_count = desc.buffer_count;
        if (image_count < capabilities.minImageCount) {
            image_count = capabilities.minImageCount;
        }
        if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
            image_count = capabilities.maxImageCount;
        }
        // Try modes in order of preference, use first supported one
        VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (u32 i = 0; i < 4; i++) {
            if (capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
                composite_alpha = composite_alpha_flags[i];
                break;
            }
        }
        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = m_Surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = {desc.width, desc.height};
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.preTransform = capabilities.currentTransform;
        create_info.compositeAlpha = composite_alpha;
        create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        create_info.clipped = VK_TRUE;
        VK_RETURN_ON_ERROR(vkCreateSwapchainKHR(m_Device, &create_info, nullptr, &m_Swapchain), "Failed to create Vulkan swapchain");
        u32 swapchain_image_count;
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchain_image_count, nullptr);
        stl::vector<VkImage> swapchain_images{mem::MemTag::Temp, swapchain_image_count};
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchain_image_count, swapchain_images.data());
        m_BackBufferCount = static_cast<u32>(swapchain_image_count);
        for (u32 i = 0; i < m_BackBufferCount && i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_BackBuffers[i].resource = reinterpret_cast<void*>(swapchain_images[i]);
            m_BackBuffers[i].format = from_vk_format(surface_format.format);
            m_BackBuffers[i].width = desc.width;
            m_BackBuffers[i].height = desc.height;
            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image = swapchain_images[i];
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = surface_format.format;
            view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;
            VK_RETURN_ON_ERROR(vkCreateImageView(m_Device, &view_info, nullptr, &m_SwapchainImageViews[i]),
                               "Failed to create swapchain image view {}", i);
        }
        CORE_INFO("Vulkan swapchain created");
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_sync_objects() {
        for (auto& fence : m_InFlightFences) {
            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait
            VK_RETURN_ON_ERROR(vkCreateFence(m_Device, &fence_info, nullptr, &fence), "Failed to create fence");
        }
        for (auto& semaphore : m_ImageAvailableSemaphores) {
            VkSemaphoreCreateInfo semaphore_info{};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VK_RETURN_ON_ERROR(vkCreateSemaphore(m_Device, &semaphore_info, nullptr, &semaphore),
                               "Failed to create image available semaphore");
        }
        for (auto& semaphore : m_RenderFinishedSemaphores) {
            VkSemaphoreCreateInfo semaphore_info{};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VK_RETURN_ON_ERROR(vkCreateSemaphore(m_Device, &semaphore_info, nullptr, &semaphore),
                               "Failed to create render finished semaphore");
        }
        CORE_INFO("Vulkan synchronization objects created");
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::init_command_queues() {
        VkQueue graphics_queue, compute_queue, transfer_queue;
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &graphics_queue);
        vkGetDeviceQueue(m_Device, m_ComputeQueueFamily, 0, &compute_queue);
        vkGetDeviceQueue(m_Device, m_TransferQueueFamily, 0, &transfer_queue);
        new (&m_GraphicsQueue) VkCommandQueue(m_Device, graphics_queue, CommandQueueType::Direct, "Graphics Queue");
        new (&m_ComputeQueue) VkCommandQueue(m_Device, compute_queue, CommandQueueType::Compute, "Compute Queue");
        new (&m_TransferQueue) VkCommandQueue(m_Device, transfer_queue, CommandQueueType::Copy, "Transfer Queue");
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_descriptor_heaps() {
        new (&m_DescriptorHeap) VkDescriptorHeap(m_Device, 10000, "Main Descriptor Heap");
        new (&m_SamplerHeap) VkDescriptorHeap(m_Device, 256, "Sampler Heap");
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_memory_allocator() {
        new (&m_MemoryAllocator) VkMemoryAllocator(m_Instance, m_PhysicalDevice, m_Device);
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_contexts() {
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            new (&m_GraphicsContexts[i]) VkGraphicsContext(this);
        }
        new (&m_ComputeContext) VkComputeContext(this);
        new (&m_CopyContext) VkCopyContext(this);
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_bindless_pipeline_layout() {
        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
        push_constant_range.offset = 0;
        push_constant_range.size = vk::NUMBER_32_BIT_CONSTANTS * sizeof(u32); // 256 bytes
        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push_constant_range;
        VK_RETURN_ON_ERROR(vkCreatePipelineLayout(m_Device, &layout_info, nullptr, &m_BindlessPipelineLayout),
                           "Failed to create bindless pipeline layout");
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::init_render_pass() {
        VkAttachmentDescription color_attachment{};
        color_attachment.format = to_vk_format(m_BackBufferFormat);
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;
        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_attachment;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        VK_RETURN_ON_ERROR(vkCreateRenderPass(m_Device, &render_pass_info, nullptr, &m_MainRenderPass), "Failed to create render pass");
        return stl::result_success();
    }

    stl::result<> VkGraphicsDevice::create_swapchain_framebuffers() {
        for (u32 i = 0; i < m_BackBufferCount && i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkImageView attachments[] = {m_SwapchainImageViews[i]};
            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = m_MainRenderPass;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments = attachments;
            framebuffer_info.width = m_WindowWidth;
            framebuffer_info.height = m_WindowHeight;
            framebuffer_info.layers = 1;
            VK_RETURN_ON_ERROR(vkCreateFramebuffer(m_Device, &framebuffer_info, nullptr, &m_SwapchainFramebuffers[i]),
                               "Failed to create framebuffer {}", i);
        }
        return stl::result_success();
    }

    void VkGraphicsDevice::cleanup_swapchain() {
        wait_for_idle();
        for (auto& framebuffer : m_SwapchainFramebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
                framebuffer = VK_NULL_HANDLE;
            }
        }
        for (auto& image_view : m_SwapchainImageViews) {
            if (image_view != VK_NULL_HANDLE) {
                vkDestroyImageView(m_Device, image_view, nullptr);
                image_view = VK_NULL_HANDLE;
            }
        }
        if (m_Swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }
    }

    u32 VkGraphicsDevice::find_queue_family(VkQueueFlags flags) {
        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_count, nullptr);
        stl::vector<VkQueueFamilyProperties> queue_families(mem::MemTag::Temp, queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_count, queue_families.data());
        for (u32 i = 0; i < queue_family_count; ++i) {
            if (queue_families[i].queueFlags & flags) {
                return i;
            }
        }
        return 0;
    }

    u32 VkGraphicsDevice::find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &mem_properties);
        for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i) {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        return 0;
    }

    stl::result<> VkGraphicsDevice::begin_frame() {
        VK_RETURN_ON_ERROR(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX),
                           "Failed to wait for fences when beginning frame.");
        VK_RETURN_ON_ERROR(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex]),
                           "Failed to reset fences when beginning frame.");
        VK_RETURN_ON_ERROR(vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIndex],
                                                 VK_NULL_HANDLE, &m_CurrentBackBufferIndex),
                           "Failed to acquire next swapchain image when beginning frame");
        return get_current_graphics_context().reset();
    }

    stl::result<> VkGraphicsDevice::end_frame() {
        auto close_result = get_current_graphics_context().close();
        if (!close_result) {
            return close_result;
        }
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore wait_semaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrameIndex]};
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        VkCommandBuffer cmd_buffer = reinterpret_cast<VkCommandBuffer>(get_current_graphics_context().get_native_command_list());
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buffer;
        VkSemaphore signal_semaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrameIndex]};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;
        VkQueue graphics_queue;
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &graphics_queue);
        VK_RETURN_ON_ERROR(vkQueueSubmit(graphics_queue, 1, &submit_info, m_InFlightFences[m_CurrentFrameIndex]),
                           "Failed to submit command buffer in end_frame");
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::present() {
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_Swapchain;
        present_info.pImageIndices = &m_CurrentBackBufferIndex;
        VkQueue present_queue;
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &present_queue);
        VK_RETURN_ON_ERROR(vkQueuePresentKHR(present_queue, &present_info), "Failed to present queue.");
        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FramesInFlight;
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::wait_for_idle() {
        if (m_Device != VK_NULL_HANDLE) {
            VK_RETURN_ON_ERROR(vkDeviceWaitIdle(m_Device), "Failed to wait for device idle.");
            ;
        }
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::resize_window(u32 width, u32 height) {
        if ((m_WindowWidth == width && m_WindowHeight == height) || width == 0 || height == 0) {
            return stl::success;
        }
        m_WindowWidth = width;
        m_WindowHeight = height;
        auto wait_result = wait_for_idle();
        if (!wait_result) {
            return wait_result;
        }
        cleanup_swapchain();
        SwapchainCreationDesc swapchain_desc{};
        swapchain_desc.width = width;
        swapchain_desc.height = height;
        swapchain_desc.format = m_BackBufferFormat;
        swapchain_desc.buffer_count = m_BackBufferCount;

        auto swapchain_result = init_swapchain(swapchain_desc);
        if (!swapchain_result) {
            return swapchain_result;
        }

        auto framebuffers_result = create_swapchain_framebuffers();
        if (!framebuffers_result) {
            return framebuffers_result;
        }
        CORE_INFO("Window resized to {}x{}", width, height);
        return stl::success;
    }

    Texture& VkGraphicsDevice::get_current_back_buffer() { return m_BackBuffers[m_CurrentBackBufferIndex]; }

    Texture& VkGraphicsDevice::get_back_buffer(u32 index) { return m_BackBuffers[index]; }

    stl::result<Buffer> VkGraphicsDevice::create_buffer(const BufferCreationDesc& desc) { return m_MemoryAllocator.allocate_buffer(desc); }

    stl::result<Buffer> VkGraphicsDevice::create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) {
        auto buffer_result = create_buffer(desc);
        if (!buffer_result) {
            return buffer_result;
        }
        Buffer buffer = std::move(buffer_result.value());
        BufferCreationDesc staging_desc{};
        staging_desc.usage = BufferUsage::Upload;
        staging_desc.size_in_bytes = data_size;
        staging_desc.name = L"Staging Buffer";
        auto staging_result = create_buffer(staging_desc);
        if (!staging_result) {
            return staging_result;
        }
        Buffer staging_buffer = std::move(staging_result.value());
        if (!staging_buffer.mapped_data) {
            m_MemoryAllocator.free_buffer(staging_buffer);
            return stl::make_error<Buffer>("Failed to map staging buffer for data upload");
        }
        memcpy(staging_buffer.mapped_data, data, data_size);
        m_CopyContext.reset();
        m_CopyContext.transition_barrier(buffer, ResourceState::Common, ResourceState::CopyDest);
        m_CopyContext.execute_resource_barriers();
        m_CopyContext.copy_buffer(buffer, staging_buffer, data_size);
        m_CopyContext.transition_barrier(buffer, ResourceState::CopyDest, ResourceState::Common);
        m_CopyContext.execute_resource_barriers();
        m_CopyContext.close();
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        VkCommandBuffer cmd_buffer = m_CopyContext.get_vk_command_buffer();
        submit_info.pCommandBuffers = &cmd_buffer;
        VkQueue transfer_queue = m_TransferQueue.get_vk_queue();
        VK_RETURN_ON_ERROR_T(Buffer, vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE), "Failed to submit buffer upload");
        wait_for_idle();
        m_MemoryAllocator.free_buffer(staging_buffer);
        return buffer;
    }

    stl::result<Texture> VkGraphicsDevice::create_texture(const TextureCreationDesc& desc) {
        return m_MemoryAllocator.allocate_texture(desc);
    }

    stl::result<Texture> VkGraphicsDevice::create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) {
        auto texture_result = create_texture(desc);
        if (!texture_result) {
            return texture_result;
        }
        Texture texture = std::move(texture_result.value());

        BufferCreationDesc staging_desc{};
        staging_desc.usage = BufferUsage::Upload;
        staging_desc.size_in_bytes = data_size;
        staging_desc.name = L"Texture Staging Buffer";

        auto staging_result = create_buffer(staging_desc);
        if (!staging_result) {
            return stl::make_error<Texture>(staging_result.error().c_str());
        }
        Buffer staging_buffer = std::move(staging_result.value());

        if (!staging_buffer.mapped_data) {
            m_MemoryAllocator.free_buffer(staging_buffer);
            return stl::make_error<Texture>("Failed to map staging buffer for texture upload");
        }

        memcpy(staging_buffer.mapped_data, data, data_size);

        m_CopyContext.reset();
        m_CopyContext.transition_barrier(texture, ResourceState::Common, ResourceState::CopyDest);
        m_CopyContext.execute_resource_barriers();
        m_CopyContext.copy_buffer_to_texture(texture, staging_buffer, 0);
        m_CopyContext.transition_barrier(texture, ResourceState::CopyDest, ResourceState::Common);
        m_CopyContext.execute_resource_barriers();
        m_CopyContext.close();

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        VkCommandBuffer cmd_buffer = m_CopyContext.get_vk_command_buffer();
        submit_info.pCommandBuffers = &cmd_buffer;
        VkQueue transfer_queue = m_TransferQueue.get_vk_queue();

        VK_RETURN_ON_ERROR_T(Texture, vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE), "Failed to submit texture upload");

        wait_for_idle();
        m_MemoryAllocator.free_buffer(staging_buffer);

        return texture;
    }

    stl::result<IPipelineState*> VkGraphicsDevice::create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) {
        auto pipeline = stl::make_unique<VkPipelineState>(mem::MemTag::Render);

        auto create_result = pipeline->create_graphics(m_Device, desc, m_BindlessPipelineLayout, m_MainRenderPass);
        if (!create_result) {
            return stl::make_error<IPipelineState*>(create_result.error().c_str());
        }

        auto* ptr = pipeline.get();
        m_PipelineStates.push_back(std::move(pipeline));
        return ptr;
    }

    stl::result<IPipelineState*> VkGraphicsDevice::create_compute_pipeline(const ComputePipelineStateDesc& desc) {
        auto pipeline = stl::make_unique<VkPipelineState>(mem::MemTag::Render);

        auto create_result = pipeline->create_compute(m_Device, desc, m_BindlessPipelineLayout);
        if (!create_result) {
            return stl::make_error<IPipelineState*>(create_result.error().c_str());
        }

        auto* ptr = pipeline.get();
        m_PipelineStates.push_back(std::move(pipeline));
        return ptr;
    }

    IGraphicsContext& VkGraphicsDevice::get_current_graphics_context() { return m_GraphicsContexts[m_CurrentFrameIndex]; }

    IGraphicsContext& VkGraphicsDevice::get_graphics_context(u32 frame_index) { return m_GraphicsContexts[frame_index]; }

    IComputeContext& VkGraphicsDevice::get_compute_context() { return m_ComputeContext; }

    ICopyContext& VkGraphicsDevice::get_copy_context() { return m_CopyContext; }
} // namespace sf::render::vk
