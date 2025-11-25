#include "engpch.h"

#include "core/logger.h"
#include "render/vulkan/vk_graphics_device.h"
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
        init_instance();
        init_surface(desc);
        init_physical_device();
        init_logical_device();
        init_swapchain(desc);
        init_command_queues();
        init_descriptor_heaps();
        init_memory_allocator();
        init_render_pass();
        init_bindless_pipeline_layout();
        create_swapchain_framebuffers();
        init_contexts();
        CORE_INFO("Vulkan graphics device initialized successfully");
    }
    VkGraphicsDevice::~VkGraphicsDevice() {
        wait_for_idle();
        cleanup_swapchain();
        m_GraphicsContexts = {};
        m_ComputeContext.reset();
        m_CopyContext.reset();
        m_DescriptorHeap.reset();
        m_SamplerHeap.reset();
        m_MemoryAllocator.reset();
        m_GraphicsQueue.reset();
        m_ComputeQueue.reset();
        m_TransferQueue.reset();
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

    void VkGraphicsDevice::init_instance() {
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
        VkResult result = vkCreateInstance(&create_info, nullptr, &m_Instance);
        if (result != VK_SUCCESS) {
            CORE_CRITICAL("Failed to create Vulkan instance. Error code: {}", static_cast<int>(result));
            return;
        }
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
    }

    void VkGraphicsDevice::init_surface(const SwapchainCreationDesc& desc) {
        SDL_Window* sdl_window = static_cast<SDL_Window*>(desc.window_handle);
        if (!SDL_Vulkan_CreateSurface(sdl_window, m_Instance, nullptr, &m_Surface)) {
            CORE_CRITICAL("Failed to create Vulkan surface: {}", SDL_GetError());
            return;
        }
        CORE_INFO("Vulkan surface created");
    }

    void VkGraphicsDevice::init_physical_device() {
        u32 device_count = 0;
        VkResult result = vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr);
        if (result != VK_SUCCESS) {
            CORE_CRITICAL("Failed to enumerate physical devices. Error code: {}", static_cast<int>(result));
            return;
        }
        if (device_count == 0) {
            CORE_CRITICAL("Failed to find GPUs with Vulkan support");
            return;
        }
        stl::vector<VkPhysicalDevice> devices{mem::MemTag::Temp, device_count};
        vkEnumeratePhysicalDevices(m_Instance, &device_count, devices.data());
        m_PhysicalDevice = devices[0];
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        CORE_INFO("Selected GPU: {}", properties.deviceName);
    }

    void VkGraphicsDevice::init_logical_device() {
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
                CORE_CRITICAL("Required device extension not available: {}", ext);
                return;
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
        VkResult result = vkCreateDevice(m_PhysicalDevice, &create_info, nullptr, &m_Device);
        if (result != VK_SUCCESS) {
            CORE_CRITICAL("Failed to create Vulkan logical device. Error code: {}", static_cast<int>(result));
            return;
        }
        CORE_INFO("Vulkan logical device created");
    }

    void VkGraphicsDevice::init_swapchain(const SwapchainCreationDesc& desc) {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);
        u32 format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &format_count, nullptr);
        stl::vector<VkSurfaceFormatKHR> formats{mem::MemTag::Temp, format_count};
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &format_count, formats.data());
        VkSurfaceFormatKHR surface_format = formats[0];
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
        VkResult result = vkCreateSwapchainKHR(m_Device, &create_info, nullptr, &m_Swapchain);
        if (result != VK_SUCCESS) {
            CORE_CRITICAL("Failed to create Vulkan swapchain. Error code: {}", static_cast<int>(result));
            return;
        }
        u32 swapchain_image_count;
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchain_image_count, nullptr);
        stl::vector<VkImage> swapchain_images{mem::MemTag::Temp, swapchain_image_count};
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchain_image_count, swapchain_images.data());
        m_BackBufferCount = static_cast<u32>(swapchain_images.size());
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
            VkResult view_result = vkCreateImageView(m_Device, &view_info, nullptr, &m_SwapchainImageViews[i]);
            if (view_result != VK_SUCCESS) {
                CORE_CRITICAL("Failed to create swapchain image view {}. Error code: {}", i, static_cast<int>(view_result));
                return;
            }
        }
        for (auto& fence : m_InFlightFences) {
            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            vkCreateFence(m_Device, &fence_info, nullptr, &fence);
        }
        for (auto& semaphore : m_ImageAvailableSemaphores) {
            VkSemaphoreCreateInfo semaphore_info{};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCreateSemaphore(m_Device, &semaphore_info, nullptr, &semaphore);
        }
        for (auto& semaphore : m_RenderFinishedSemaphores) {
            VkSemaphoreCreateInfo semaphore_info{};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCreateSemaphore(m_Device, &semaphore_info, nullptr, &semaphore);
        }
        CORE_INFO("Vulkan swapchain created");
    }

    void VkGraphicsDevice::init_command_queues() {
        VkQueue graphics_queue, compute_queue, transfer_queue;
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &graphics_queue);
        vkGetDeviceQueue(m_Device, m_ComputeQueueFamily, 0, &compute_queue);
        vkGetDeviceQueue(m_Device, m_TransferQueueFamily, 0, &transfer_queue);
        m_GraphicsQueue =
            stl::make_unique<VkCommandQueue>(mem::MemTag::Render, m_Device, graphics_queue, CommandQueueType::Direct, "Graphics Queue");
        m_ComputeQueue =
            stl::make_unique<VkCommandQueue>(mem::MemTag::Render, m_Device, compute_queue, CommandQueueType::Compute, "Compute Queue");
        m_TransferQueue =
            stl::make_unique<VkCommandQueue>(mem::MemTag::Render, m_Device, transfer_queue, CommandQueueType::Copy, "Transfer Queue");
    }

    void VkGraphicsDevice::init_descriptor_heaps() {
        m_DescriptorHeap = stl::make_unique<VkDescriptorHeap>(mem::MemTag::Render, m_Device, 10000, "Main Descriptor Heap");
        m_SamplerHeap = stl::make_unique<VkDescriptorHeap>(mem::MemTag::Render, m_Device, 256, "Sampler Heap");
    }

    void VkGraphicsDevice::init_memory_allocator() {
        m_MemoryAllocator = stl::make_unique<VkMemoryAllocator>(mem::MemTag::Render, m_Instance, m_PhysicalDevice, m_Device);
    }

    void VkGraphicsDevice::init_contexts() {
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_GraphicsContexts[i] = stl::make_unique<VkGraphicsContext>(mem::MemTag::Render, this);
        }
        m_ComputeContext = stl::make_unique<VkComputeContext>(mem::MemTag::Render, this);
        m_CopyContext = stl::make_unique<VkCopyContext>(mem::MemTag::Render, this);
    }

    void VkGraphicsDevice::init_bindless_pipeline_layout() {
        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
        push_constant_range.offset = 0;
        push_constant_range.size = vk::NUMBER_32_BIT_CONSTANTS * sizeof(u32); // 256 bytes
        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push_constant_range;
        vkCreatePipelineLayout(m_Device, &layout_info, nullptr, &m_BindlessPipelineLayout);
    }

    void VkGraphicsDevice::init_render_pass() {
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
        vkCreateRenderPass(m_Device, &render_pass_info, nullptr, &m_MainRenderPass);
    }

    void VkGraphicsDevice::create_swapchain_framebuffers() {
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
            VkResult result = vkCreateFramebuffer(m_Device, &framebuffer_info, nullptr, &m_SwapchainFramebuffers[i]);
            if (result != VK_SUCCESS) {
                CORE_CRITICAL("Failed to create framebuffer {}. Error code: {}", i, static_cast<int>(result));
                return;
            }
        }
    }

    void VkGraphicsDevice::cleanup_swapchain() {
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

    void VkGraphicsDevice::begin_frame() {
        vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex]);
        vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIndex], VK_NULL_HANDLE,
                              &m_CurrentBackBufferIndex);
        get_current_graphics_context().reset();
    }

    void VkGraphicsDevice::end_frame() { get_current_graphics_context().close(); }

    void VkGraphicsDevice::present() {
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_Swapchain;
        present_info.pImageIndices = &m_CurrentBackBufferIndex;
        VkQueue present_queue;
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &present_queue);
        vkQueuePresentKHR(present_queue, &present_info);
        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FramesInFlight;
    }

    void VkGraphicsDevice::wait_for_idle() {
        if (m_Device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_Device);
        }
    }

    void VkGraphicsDevice::resize_window(u32 width, u32 height) {
        if ((m_WindowWidth == width && m_WindowHeight == height) || width == 0 || height == 0) {
            return;
        }
        m_WindowWidth = width;
        m_WindowHeight = height;
        wait_for_idle();
        cleanup_swapchain();
        SwapchainCreationDesc swapchain_desc{};
        swapchain_desc.width = width;
        swapchain_desc.height = height;
        swapchain_desc.format = m_BackBufferFormat;
        swapchain_desc.buffer_count = m_BackBufferCount;
        init_swapchain(swapchain_desc);
        create_swapchain_framebuffers();
        CORE_INFO("Window resized to {}x{}", width, height);
    }
    Texture& VkGraphicsDevice::get_current_back_buffer() { return m_BackBuffers[m_CurrentBackBufferIndex]; }
    Texture& VkGraphicsDevice::get_back_buffer(u32 index) { return m_BackBuffers[index]; }
    Buffer VkGraphicsDevice::create_buffer(const BufferCreationDesc& desc) {
        Buffer buffer{};
        m_MemoryAllocator->allocate_buffer(buffer, desc);
        return buffer;
    }
    Buffer VkGraphicsDevice::create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) {
        Buffer buffer = create_buffer(desc);
        BufferCreationDesc staging_desc{};
        staging_desc.usage = BufferUsage::Upload;
        staging_desc.size_in_bytes = data_size;
        staging_desc.name = L"Staging Buffer";
        Buffer staging_buffer = create_buffer(staging_desc);
        if (staging_buffer.mapped_data) {
            memcpy(staging_buffer.mapped_data, data, data_size);
        } else {
            CORE_ERROR("Failed to map staging buffer for data upload");
            m_MemoryAllocator->free_buffer(staging_buffer);
            return buffer;
        }
        m_CopyContext->reset();
        m_CopyContext->transition_barrier(buffer, ResourceState::Common, ResourceState::CopyDest);
        m_CopyContext->execute_resource_barriers();
        m_CopyContext->copy_buffer(buffer, staging_buffer, data_size);
        m_CopyContext->transition_barrier(buffer, ResourceState::CopyDest, ResourceState::Common);
        m_CopyContext->execute_resource_barriers();
        m_CopyContext->close();
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        VkCommandBuffer cmd_buffer = m_CopyContext->get_vk_command_buffer();
        submit_info.pCommandBuffers = &cmd_buffer;
        VkQueue transfer_queue = m_TransferQueue->get_vk_queue();
        vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
        wait_for_idle();
        m_MemoryAllocator->free_buffer(staging_buffer);
        return buffer;
    }
    Texture VkGraphicsDevice::create_texture(const TextureCreationDesc& desc) {
        Texture texture{};
        m_MemoryAllocator->allocate_texture(texture, desc);
        return texture;
    }
    Texture VkGraphicsDevice::create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) {
        Texture texture = create_texture(desc);
        BufferCreationDesc staging_desc{};
        staging_desc.usage = BufferUsage::Upload;
        staging_desc.size_in_bytes = data_size;
        staging_desc.name = L"Texture Staging Buffer";
        Buffer staging_buffer = create_buffer(staging_desc);
        if (staging_buffer.mapped_data) {
            memcpy(staging_buffer.mapped_data, data, data_size);
        } else {
            CORE_ERROR("Failed to map staging buffer for texture upload");
            m_MemoryAllocator->free_buffer(staging_buffer);
            return texture;
        }
        m_CopyContext->reset();
        m_CopyContext->transition_barrier(texture, ResourceState::Common, ResourceState::CopyDest);
        m_CopyContext->execute_resource_barriers();
        m_CopyContext->copy_buffer_to_texture(texture, staging_buffer, 0);
        m_CopyContext->transition_barrier(texture, ResourceState::CopyDest, ResourceState::Common);
        m_CopyContext->execute_resource_barriers();
        m_CopyContext->close();
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        VkCommandBuffer cmd_buffer = m_CopyContext->get_vk_command_buffer();
        submit_info.pCommandBuffers = &cmd_buffer;
        VkQueue transfer_queue = m_TransferQueue->get_vk_queue();
        vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
        wait_for_idle();
        m_MemoryAllocator->free_buffer(staging_buffer);
        return texture;
    }
    IPipelineState* VkGraphicsDevice::create_graphics_pipeline(const GraphicsPipelineStateDesc& desc) {
        auto pipeline = stl::make_unique<VkPipelineState>(mem::MemTag::Render);
        pipeline->create_graphics(m_Device, desc, m_BindlessPipelineLayout, m_MainRenderPass);
        auto* ptr = pipeline.get();
        m_PipelineStates.push_back(std::move(pipeline));
        return ptr;
    }
    IPipelineState* VkGraphicsDevice::create_compute_pipeline(const ComputePipelineStateDesc& desc) {
        auto pipeline = stl::make_unique<VkPipelineState>(mem::MemTag::Render);
        pipeline->create_compute(m_Device, desc, m_BindlessPipelineLayout);
        auto* ptr = pipeline.get();
        m_PipelineStates.push_back(std::move(pipeline));
        return ptr;
    }
    IGraphicsContext& VkGraphicsDevice::get_current_graphics_context() { return *m_GraphicsContexts[m_CurrentFrameIndex]; }
    IGraphicsContext& VkGraphicsDevice::get_graphics_context(u32 frame_index) { return *m_GraphicsContexts[frame_index]; }
    IComputeContext& VkGraphicsDevice::get_compute_context() { return *m_ComputeContext; }
    ICopyContext& VkGraphicsDevice::get_copy_context() { return *m_CopyContext; }
} // namespace sf::render::vk
