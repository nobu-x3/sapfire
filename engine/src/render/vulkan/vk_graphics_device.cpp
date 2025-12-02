#include "engpch.h"

#include "core/logger.h"
#include "render/render_api.h"
#include "render/resource_types.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_descriptor_heap.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_memory_allocator.h"
#include "render/vulkan/vk_type_conversions.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <set>
#include <vulkan/vulkan_core.h>

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
        m_Headless = desc.headless;
        auto instance_result = init_instance();
        if (!instance_result) {
            CORE_CRITICAL(instance_result.error().c_str());
            return;
        }
        if (!m_Headless) {
            auto surface_result = init_surface(desc);
            if (!surface_result) {
                CORE_CRITICAL(surface_result.error().c_str());
                return;
            }
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
        VmaAllocatorCreateInfo allocator_create_info{};
        allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
        allocator_create_info.instance = m_Instance;
        allocator_create_info.physicalDevice = m_PhysicalDevice;
        allocator_create_info.device = m_Device;
        if (vmaCreateAllocator(&allocator_create_info, &m_InternalAllocator) != VK_SUCCESS) {
            CORE_CRITICAL("Failed to create VMA allocator");
            return;
        }
        if (!m_Headless) {
            auto swapchain_result = init_swapchain(desc);
            if (!swapchain_result) {
                CORE_CRITICAL(swapchain_result.error().c_str());
                return;
            }
        }
        auto sync_objs_result = init_sync_objects();
        if (!sync_objs_result) {
            CORE_CRITICAL(sync_objs_result.error().c_str());
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
        if (!m_Headless) {
            auto framebuffers_result = create_swapchain_framebuffers();
            if (!framebuffers_result) {
                CORE_CRITICAL(framebuffers_result.error().c_str());
                return;
            }
        } else {
            // Create offscreen render targets
            auto offscreen_result = create_offscreen_render_targets(desc);
            if (!offscreen_result) {
                CORE_CRITICAL(offscreen_result.error().c_str());
                return;
            }
        }
        if (m_Headless) {
            CORE_INFO("Vulkan graphics device initialized successfully (headless mode)");
        } else {
            CORE_INFO("Vulkan graphics device initialized successfully");
        }
    }

    VkGraphicsDevice::~VkGraphicsDevice() {
        wait_for_idle();
        cleanup_swapchain();
        m_PipelineStates.clear();
        if (m_BindlessDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_BindlessDescriptorSetLayout, nullptr);
            m_BindlessDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_PerFrameDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_PerFrameDescriptorSetLayout, nullptr);
            m_PerFrameDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_ResourceDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_ResourceDescriptorSetLayout, nullptr);
            m_ResourceDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_MaterialDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_MaterialDescriptorSetLayout, nullptr);
            m_MaterialDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_DummyDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_DummyDescriptorSetLayout, nullptr);
            m_DummyDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_BindlessPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_Device, m_BindlessPipelineLayout, nullptr);
            m_BindlessPipelineLayout = VK_NULL_HANDLE;
        }
        if (m_MainRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_Device, m_MainRenderPass, nullptr);
            m_MainRenderPass = VK_NULL_HANDLE;
        }
        for (auto& fence : m_InFlightFences) {
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(m_Device, fence, nullptr);
                fence = VK_NULL_HANDLE;
            }
        }
        for (auto& semaphore : m_ImageAvailableSemaphores) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, semaphore, nullptr);
                semaphore = VK_NULL_HANDLE;
            }
        }
        for (auto& semaphore : m_RenderFinishedSemaphores) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, semaphore, nullptr);
                semaphore = VK_NULL_HANDLE;
            }
        }
        // Destroy internal VMA allocator (before device)
        if (m_InternalAllocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(m_InternalAllocator);
            m_InternalAllocator = VK_NULL_HANDLE;
        }
        // Destroy device (must be AFTER all child objects are destroyed)
        if (m_Device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
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
            m_Surface = VK_NULL_HANDLE;
        }
        if (m_Instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
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
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::init_surface(const SwapchainCreationDesc& desc) {
        SDL_Window* sdl_window = static_cast<SDL_Window*>(desc.window_handle);
        if (!SDL_Vulkan_CreateSurface(sdl_window, m_Instance, nullptr, &m_Surface)) {
            return stl::make_error<>("Failed to create Vulkan surface: {}", SDL_GetError());
        }
        CORE_INFO("Vulkan surface created");
        return stl::success;
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
        return stl::success;
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
        required_extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        // Optional extensions (will be enabled if available, following vk-bootstrap pattern)
        stl::vector<const char*> optional_extensions = {mem::MemTag::Temp,
                                                        0,
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
        // Enable Vulkan 1.2 features for bindless rendering
        VkPhysicalDeviceVulkan12Features vulkan12_features{};
        vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vulkan12_features.runtimeDescriptorArray = VK_TRUE;
        vulkan12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        vulkan12_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
        vulkan12_features.descriptorBindingPartiallyBound = VK_TRUE;
        vulkan12_features.descriptorIndexing = VK_TRUE;
        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = static_cast<u32>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = static_cast<u32>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();
        create_info.pNext = &vulkan12_features;
        VK_RETURN_ON_ERROR(vkCreateDevice(m_PhysicalDevice, &create_info, nullptr, &m_Device), "Failed to create Vulkan logical device");
        CORE_INFO("Vulkan logical device created");
        return stl::success;
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
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
        return stl::success;
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

    stl::result<stl::unique_ptr<IGraphicsContext>> VkGraphicsDevice::create_graphics_context() {
        auto context = stl::make_unique<VkGraphicsContext>(mem::MemTag::Render, this);
        return stl::unique_ptr<IGraphicsContext>(context.release());
    }

    stl::result<stl::unique_ptr<IComputeContext>> VkGraphicsDevice::create_compute_context() {
        auto context = stl::make_unique<VkComputeContext>(mem::MemTag::Render, this);
        return stl::unique_ptr<IComputeContext>(context.release());
    }

    stl::result<stl::unique_ptr<ICopyContext>> VkGraphicsDevice::create_copy_context() {
        auto context = stl::make_unique<VkCopyContext>(mem::MemTag::Render, this);
        return stl::unique_ptr<ICopyContext>(context.release());
    }

    stl::result<stl::unique_ptr<ICommandQueue>> VkGraphicsDevice::create_direct_queue(const char* name) {
        VkQueue vk_queue;
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &vk_queue);
        auto queue = stl::make_unique<VkCommandQueue>(mem::MemTag::Render, m_Device, vk_queue, CommandQueueType::Direct, name);
        return stl::unique_ptr<ICommandQueue>(queue.release());
    }

    stl::result<stl::unique_ptr<ICommandQueue>> VkGraphicsDevice::create_compute_queue(const char* name) {
        VkQueue vk_queue;
        vkGetDeviceQueue(m_Device, m_ComputeQueueFamily, 0, &vk_queue);
        auto queue = stl::make_unique<VkCommandQueue>(mem::MemTag::Render, m_Device, vk_queue, CommandQueueType::Compute, name);
        return stl::unique_ptr<ICommandQueue>(queue.release());
    }

    stl::result<stl::unique_ptr<ICommandQueue>> VkGraphicsDevice::create_copy_queue(const char* name) {
        VkQueue vk_queue;
        vkGetDeviceQueue(m_Device, m_TransferQueueFamily, 0, &vk_queue);
        auto queue = stl::make_unique<VkCommandQueue>(mem::MemTag::Render, m_Device, vk_queue, CommandQueueType::Copy, name);
        return stl::unique_ptr<ICommandQueue>(queue.release());
    }

    stl::result<stl::unique_ptr<IDescriptorHeap>> VkGraphicsDevice::create_cbv_srv_uav_heap(const DescriptorHeapDesc& desc) {
        auto heap = stl::make_unique<VkDescriptorHeap>(mem::MemTag::Render, m_Device, desc.descriptor_count, desc.name);
        heap->set_descriptor_set_layout(m_ResourceDescriptorSetLayout, 2);
        return stl::unique_ptr<IDescriptorHeap>(heap.release());
    }

    stl::result<stl::unique_ptr<IDescriptorHeap>> VkGraphicsDevice::create_rtv_heap(const DescriptorHeapDesc& desc) {
        // In Vulkan, RTVs are framebuffer attachments, not descriptors - return dummy heap for API compatibility
        auto heap = stl::make_unique<VkDescriptorHeap>(mem::MemTag::Render, m_Device, desc.descriptor_count, desc.name);
        return stl::unique_ptr<IDescriptorHeap>(heap.release());
    }

    stl::result<stl::unique_ptr<IDescriptorHeap>> VkGraphicsDevice::create_dsv_heap(const DescriptorHeapDesc& desc) {
        // In Vulkan, DSVs are framebuffer attachments, not descriptors - return dummy heap for API compatibility
        auto heap = stl::make_unique<VkDescriptorHeap>(mem::MemTag::Render, m_Device, desc.descriptor_count, desc.name);
        return stl::unique_ptr<IDescriptorHeap>(heap.release());
    }

    stl::result<stl::unique_ptr<IDescriptorHeap>> VkGraphicsDevice::create_sampler_heap(const DescriptorHeapDesc& desc) {
        auto heap = stl::make_unique<VkDescriptorHeap>(mem::MemTag::Render, m_Device, desc.descriptor_count, desc.name);
        heap->set_descriptor_set_layout(m_DummyDescriptorSetLayout, 1);
        return stl::unique_ptr<IDescriptorHeap>(heap.release());
    }

    stl::result<stl::unique_ptr<IMemoryAllocator>> VkGraphicsDevice::create_memory_allocator() {
        auto allocator = stl::make_unique<VkMemoryAllocator>(mem::MemTag::Render, m_Instance, m_PhysicalDevice, m_Device);
        return stl::unique_ptr<IMemoryAllocator>(allocator.release());
    }

    stl::result<> VkGraphicsDevice::init_bindless_pipeline_layout() {
        // Create descriptor set layouts for bindless rendering
        // Set 0: Per-frame data (Scene + Pass data)
        stl::vector<VkDescriptorSetLayoutBinding> set0_bindings{mem::MemTag::Render, 2};
        set0_bindings[0].binding = 0;
        set0_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        set0_bindings[0].descriptorCount = 1;
        set0_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        set0_bindings[0].pImmutableSamplers = nullptr;
        set0_bindings[1].binding = 1;
        set0_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        set0_bindings[1].descriptorCount = 1;
        set0_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        set0_bindings[1].pImmutableSamplers = nullptr;
        VkDescriptorSetLayoutCreateInfo set0_info{};
        set0_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set0_info.bindingCount = static_cast<u32>(set0_bindings.size());
        set0_info.pBindings = set0_bindings.data();
        VK_RETURN_ON_ERROR(vkCreateDescriptorSetLayout(m_Device, &set0_info, nullptr, &m_PerFrameDescriptorSetLayout),
                           "Failed to create descriptor set layout 0");
        // Set 2: Bindless resource arrays (with descriptor indexing)
        stl::vector<VkDescriptorSetLayoutBinding> set2_bindings{mem::MemTag::Render, 5};
        set2_bindings[0].binding = 0;
        set2_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        set2_bindings[0].descriptorCount = 3000000;
        set2_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        set2_bindings[0].pImmutableSamplers = nullptr;
        set2_bindings[1].binding = 1;
        set2_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        set2_bindings[1].descriptorCount = 3000000;
        set2_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        set2_bindings[1].pImmutableSamplers = nullptr;
        set2_bindings[2].binding = 2;
        set2_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        set2_bindings[2].descriptorCount = 3000000;
        set2_bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        set2_bindings[2].pImmutableSamplers = nullptr;
        set2_bindings[3].binding = 3;
        set2_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        set2_bindings[3].descriptorCount = 3000000;
        set2_bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        set2_bindings[3].pImmutableSamplers = nullptr;
        set2_bindings[4].binding = 4;
        set2_bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        set2_bindings[4].descriptorCount = 3000000;
        set2_bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        set2_bindings[4].pImmutableSamplers = nullptr;
        VkDescriptorSetLayoutBindingFlagsCreateInfo set2_flags_info{};
        set2_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        // Variable descriptor count flag can only be on the last binding (binding 4)
        stl::vector<VkDescriptorBindingFlags> binding_flags{mem::MemTag::Render, 5};
        binding_flags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT; // Position buffers
        binding_flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT; // Normal buffers
        binding_flags[2] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT; // UV buffers
        binding_flags[3] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT; // Textures
        binding_flags[4] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                           VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT; // Samplers (last binding)
        set2_flags_info.bindingCount = static_cast<u32>(binding_flags.size());
        set2_flags_info.pBindingFlags = binding_flags.data();
        VkDescriptorSetLayoutCreateInfo set2_info{};
        set2_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set2_info.bindingCount = static_cast<u32>(set2_bindings.size());
        set2_info.pBindings = set2_bindings.data();
        set2_info.pNext = &set2_flags_info;
        VK_RETURN_ON_ERROR(vkCreateDescriptorSetLayout(m_Device, &set2_info, nullptr, &m_ResourceDescriptorSetLayout),
                           "Failed to create descriptor set layout 2");
        // Set 3: Material data
        stl::vector<VkDescriptorSetLayoutBinding> set3_bindings{mem::MemTag::Render, 1};
        set3_bindings[0].binding = 0;
        set3_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        set3_bindings[0].descriptorCount = 1;
        set3_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        set3_bindings[0].pImmutableSamplers = nullptr;
        VkDescriptorSetLayoutCreateInfo set3_info{};
        set3_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set3_info.bindingCount = static_cast<u32>(set3_bindings.size());
        set3_info.pBindings = set3_bindings.data();
        VK_RETURN_ON_ERROR(vkCreateDescriptorSetLayout(m_Device, &set3_info, nullptr, &m_MaterialDescriptorSetLayout),
                           "Failed to create descriptor set layout 3");
        // Create pipeline layout with descriptor set layouts
        // Note: We have sets 0, 2, 3. To handle the gap (set 1), we create an empty layout for it
        // Or we can reorganize to use contiguous descriptor set indices
        // For now, let's create a dummy empty layout for set 1
        VkDescriptorSetLayoutCreateInfo empty_set_info{};
        empty_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        empty_set_info.bindingCount = 0;
        empty_set_info.pBindings = nullptr;
        VK_RETURN_ON_ERROR(vkCreateDescriptorSetLayout(m_Device, &empty_set_info, nullptr, &m_DummyDescriptorSetLayout),
                           "Failed to create dummy descriptor set layout");
        stl::vector<VkDescriptorSetLayout> descriptor_layouts{mem::MemTag::Render, 4};
        descriptor_layouts[0] = m_PerFrameDescriptorSetLayout;
        descriptor_layouts[1] = m_DummyDescriptorSetLayout; // Empty layout for unused set 1
        descriptor_layouts[2] = m_ResourceDescriptorSetLayout;
        descriptor_layouts[3] = m_MaterialDescriptorSetLayout;
        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
        push_constant_range.offset = 0;
        push_constant_range.size = vk::NUMBER_32_BIT_CONSTANTS * sizeof(u32); // 256 bytes
        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = static_cast<u32>(descriptor_layouts.size());
        layout_info.pSetLayouts = descriptor_layouts.data();
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push_constant_range;
        VK_RETURN_ON_ERROR(vkCreatePipelineLayout(m_Device, &layout_info, nullptr, &m_BindlessPipelineLayout),
                           "Failed to create bindless pipeline layout");
        return stl::success;
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
        return stl::success;
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
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::create_offscreen_render_targets(const SwapchainCreationDesc& desc) {
        // Create custom render target textures for headless rendering (editor viewports)
        CORE_INFO("Creating {} offscreen render targets ({}x{})", MAX_FRAMES_IN_FLIGHT, m_WindowWidth, m_WindowHeight);
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            // Create VkImage directly using internal VMA allocator
            VkImageCreateInfo image_info{};
            image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_info.imageType = VK_IMAGE_TYPE_2D;
            image_info.extent.width = m_WindowWidth;
            image_info.extent.height = m_WindowHeight;
            image_info.extent.depth = 1;
            image_info.mipLevels = 1;
            image_info.arrayLayers = 1;
            image_info.format = to_vk_format(m_BackBufferFormat);
            image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            image_info.samples = VK_SAMPLE_COUNT_1_BIT;
            image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VmaAllocationCreateInfo alloc_info{};
            alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
            VkImage vk_image;
            VmaAllocation allocation;
            VK_RETURN_ON_ERROR(vmaCreateImage(m_InternalAllocator, &image_info, &alloc_info, &vk_image, &allocation, nullptr),
                               "Failed to create offscreen render target {}", i);
            m_BackBuffers[i].resource = reinterpret_cast<void*>(vk_image);
            m_BackBuffers[i].allocation = allocation;
            m_BackBuffers[i].width = m_WindowWidth;
            m_BackBuffers[i].height = m_WindowHeight;
            m_BackBuffers[i].format = m_BackBufferFormat;
            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image = vk_image;
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = to_vk_format(m_BackBufferFormat);
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
                               "Failed to create offscreen render target image view {}", i);
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
                               "Failed to create offscreen framebuffer {}", i);
        }
        m_BackBufferCount = MAX_FRAMES_IN_FLIGHT;
        // Transition all offscreen images from UNDEFINED to PRESENT_SRC_KHR layout
        VkCommandPool temp_pool;
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = m_GraphicsQueueFamily;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        VK_RETURN_ON_ERROR(vkCreateCommandPool(m_Device, &pool_info, nullptr, &temp_pool), "Failed to create temporary command pool");
        VkCommandBuffer cmd_buffer;
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = temp_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;
        VK_RETURN_ON_ERROR(vkAllocateCommandBuffers(m_Device, &alloc_info, &cmd_buffer), "Failed to allocate transition command buffer");
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_RETURN_ON_ERROR(vkBeginCommandBuffer(cmd_buffer, &begin_info), "Failed to begin transition command buffer");
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = reinterpret_cast<VkImage>(m_BackBuffers[i].resource);
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;
            vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        VK_RETURN_ON_ERROR(vkEndCommandBuffer(cmd_buffer), "Failed to end transition command buffer");
        VkQueue graphics_queue;
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &graphics_queue);
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buffer;
        VK_RETURN_ON_ERROR(vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE), "Failed to submit transition command buffer");
        VK_RETURN_ON_ERROR(vkQueueWaitIdle(graphics_queue), "Failed to wait for transition queue");
        vkFreeCommandBuffers(m_Device, temp_pool, 1, &cmd_buffer);
        vkDestroyCommandPool(m_Device, temp_pool, nullptr);
        CORE_INFO("Offscreen render targets created successfully");
        return stl::success;
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
        if (m_Headless) {
            for (auto& back_buffer : m_BackBuffers) {
                if (back_buffer.resource != nullptr) {
                    VkImage vk_image = reinterpret_cast<VkImage>(back_buffer.resource);
                    VmaAllocation allocation = reinterpret_cast<VmaAllocation>(back_buffer.allocation);
                    vmaDestroyImage(m_InternalAllocator, vk_image, allocation);
                    back_buffer = Texture{};
                }
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
        if (m_Headless) {
            // In headless mode, we cycle through our custom render targets manually
            m_CurrentBackBufferIndex = m_CurrentFrameIndex;
        } else {
            // In swapchain mode, acquire the next image from the swapchain
            VK_RETURN_ON_ERROR(vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIndex],
                                                     VK_NULL_HANDLE, &m_CurrentBackBufferIndex),
                               "Failed to acquire next swapchain image when beginning frame");
        }
        // User is responsible for resetting and managing contexts
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::end_frame() {
        // In stateless API, user handles command buffer submission via ICommandQueue
        // Device only manages swapchain synchronization primitives
        // User should submit their command buffers to queues before calling end_frame
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::present() {
        if (m_Headless) {
            // In headless mode, no presentation needed - just advance frame index
            m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
            return stl::success;
        }
        // Swapchain mode: present to the window
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
        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        return stl::success;
    }

    stl::result<> VkGraphicsDevice::wait_for_idle() {
        if (m_Device != VK_NULL_HANDLE) {
            VK_RETURN_ON_ERROR(vkDeviceWaitIdle(m_Device), "Failed to wait for device idle.");
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
        swapchain_desc.headless = m_Headless;
        if (m_Headless) {
            // Recreate offscreen render targets with new size
            auto offscreen_result = create_offscreen_render_targets(swapchain_desc);
            if (!offscreen_result) {
                return offscreen_result;
            }
        } else {
            // Recreate swapchain for windowed mode
            auto swapchain_result = init_swapchain(swapchain_desc);
            if (!swapchain_result) {
                return swapchain_result;
            }
            auto framebuffers_result = create_swapchain_framebuffers();
            if (!framebuffers_result) {
                return framebuffers_result;
            }
        }
        CORE_INFO("Window resized to {}x{}", width, height);
        return stl::success;
    }

    Texture& VkGraphicsDevice::get_current_back_buffer() { return m_BackBuffers[m_CurrentBackBufferIndex]; }

    Texture& VkGraphicsDevice::get_back_buffer(u32 index) { return m_BackBuffers[index]; }

    stl::result<Buffer> VkGraphicsDevice::create_buffer(const BufferCreationDesc& desc) {
        // User must create their own memory allocator to allocate buffers
        return stl::make_error<Buffer>("create_buffer requires a user-owned IMemoryAllocator. Use IMemoryAllocator::allocate_buffer() instead.");
    }

    stl::result<Buffer> VkGraphicsDevice::create_buffer_with_data(const BufferCreationDesc& desc, const void* data, size_t data_size) {
        // User must handle buffer uploads explicitly using their own allocator, contexts, and queues
        return stl::make_error<Buffer>("create_buffer_with_data not supported in stateless API. Create buffer via IMemoryAllocator, then upload using ICopyContext.");
    }

    stl::result<Texture> VkGraphicsDevice::create_texture(const TextureCreationDesc& desc) {
        // User must create their own memory allocator to allocate textures
        return stl::make_error<Texture>("create_texture requires a user-owned IMemoryAllocator. Use IMemoryAllocator::allocate_texture() instead.");
    }

    stl::result<Texture> VkGraphicsDevice::create_texture_with_data(const TextureCreationDesc& desc, const void* data, size_t data_size) {
        // User must handle texture uploads explicitly using their own allocator, contexts, and queues
        return stl::make_error<Texture>("create_texture_with_data not supported in stateless API. Create texture via IMemoryAllocator, then upload using ICopyContext.");
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

    stl::result<> VkGraphicsDevice::read_texture_pixels(Texture& texture, void* out_data, size_t data_size) {
        // User must handle texture readback explicitly using their own allocator, contexts, and queues
        return stl::make_error("read_texture_pixels not supported in stateless API. Use ICopyContext to copy texture to staging buffer, then read mapped data.");
    }

} // namespace sf::render::vk
