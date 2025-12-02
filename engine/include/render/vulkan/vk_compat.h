#pragma once

#include <vulkan/vulkan.h>
#include "core/core.h"

namespace sf::render::vk {

    // Convert VkResult to a human-readable error message
    inline const char* vk_result_to_string(VkResult result) {
        switch (result) {
        case VK_SUCCESS:
            return "Success";
        case VK_NOT_READY:
            return "Not ready";
        case VK_TIMEOUT:
            return "Timeout";
        case VK_EVENT_SET:
            return "Event set";
        case VK_EVENT_RESET:
            return "Event reset";
        case VK_INCOMPLETE:
            return "Incomplete";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "Out of host memory";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "Out of device memory";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "Initialization failed";
        case VK_ERROR_DEVICE_LOST:
            return "Device lost";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "Memory map failed";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "Layer not present";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "Extension not present";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "Feature not present";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "Incompatible driver";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "Too many objects";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "Format not supported";
        case VK_ERROR_FRAGMENTED_POOL:
            return "Fragmented pool";
        case VK_ERROR_UNKNOWN:
            return "Unknown error";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "Out of pool memory";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "Invalid external handle";
        case VK_ERROR_FRAGMENTATION:
            return "Fragmentation";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "Invalid opaque capture address";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "Surface lost";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "Native window in use";
        case VK_SUBOPTIMAL_KHR:
            return "Suboptimal";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "Out of date";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "Incompatible display";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "Validation failed";
        case VK_ERROR_INVALID_SHADER_NV:
            return "Invalid shader";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "Invalid DRM format modifier plane layout";
        case VK_ERROR_NOT_PERMITTED_EXT:
            return "Not permitted";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "Full screen exclusive mode lost";
        default:
            return "Unknown VkResult";
        }
    }

// Macro to check VkResult and return error on failure
// Usage: VK_RETURN_ON_ERROR(vkCreateSwapchainKHR(...), "Failed to create swapchain")
#define VK_RETURN_ON_ERROR(call, msg, ...)                                                                                                 \
    do {                                                                                                                                   \
        VkResult vk_result_##__LINE__##__COUNTER__ = (call);                                                                               \
        if (vk_result_##__LINE__##__COUNTER__ != VK_SUCCESS) {                                                                             \
            return ::sf::stl::make_error<>(                                                                                                \
                "[{}:{}] {}: VkResult = {} ({})", __FILE__, __LINE__, msg, static_cast<int>(vk_result_##__LINE__##__COUNTER__),            \
                ::sf::render::vk::vk_result_to_string(vk_result_##__LINE__##__COUNTER__) __VA_OPT__(, ) __VA_ARGS__);                      \
        }                                                                                                                                  \
    } while (0)

// Macro to check VkResult and return typed error on failure
// Usage: VK_RETURN_ON_ERROR_T(Buffer, vkCreateBuffer(...), "Failed to create buffer")
#define VK_RETURN_ON_ERROR_T(T, call, msg, ...)                                                                                            \
    do {                                                                                                                                   \
        VkResult vk_result_##__LINE__##__COUNTER__ = (call);                                                                               \
        if (vk_result_##__LINE__##__COUNTER__ != VK_SUCCESS) {                                                                             \
            return ::sf::stl::make_error<T>(                                                                                               \
                "[{}:{}] {}: VkResult = {} ({})", __FILE__, __LINE__, msg, static_cast<int>(vk_result_##__LINE__##__COUNTER__),            \
                ::sf::render::vk::vk_result_to_string(vk_result_##__LINE__##__COUNTER__) __VA_OPT__(, ) __VA_ARGS__);                      \
        }                                                                                                                                  \
    } while (0)

// Macro to check VkResult, assign to variable on success, return error on failure
// Usage: VK_ASSIGN_OR_RETURN(my_var, vkCreateBuffer(...), "Failed to create buffer")
#define VK_ASSIGN_OR_RETURN(var, call, msg, ...)                                                                                           \
    do {                                                                                                                                   \
        VkResult vk_result_##__LINE__##__COUNTER__ = (call);                                                                               \
        if (vk_result_##__LINE__##__COUNTER__ != VK_SUCCESS) {                                                                             \
            return ::sf::stl::make_error<decltype(var)>(                                                                                   \
                "[{}:{}] {}: VkResult = {} ({})", __FILE__, __LINE__, msg, static_cast<int>(vk_result_##__LINE__##__COUNTER__),            \
                ::sf::render::vk::vk_result_to_string(vk_result_##__LINE__##__COUNTER__) __VA_OPT__(, ) __VA_ARGS__);                      \
        }                                                                                                                                  \
    } while (0)

// Macro to forward stl::result errors
// Usage: FORWARD_ERROR(some_function_returning_result())
#define FORWARD_ERROR(call)                                                                                                                \
    do {                                                                                                                                   \
        auto&& result_##__LINE__##__COUNTER__ = (call);                                                                                    \
        if (!result_##__LINE__##__COUNTER__) {                                                                                             \
            return ::sf::stl::make_error<>("[{}:{}] {}", __FILE__, __LINE__, result_##__LINE__##__COUNTER__.error().c_str());              \
        }                                                                                                                                  \
    } while (0)

// Macro to forward stl::result errors with custom type
// Usage: FORWARD_ERROR_T(Buffer, some_function_returning_result())
#define FORWARD_ERROR_T(T, call)                                                                                                           \
    do {                                                                                                                                   \
        auto&& result_##__LINE__##__COUNTER__ = (call);                                                                                    \
        if (!result_##__LINE__##__COUNTER__) {                                                                                             \
            return ::sf::stl::make_error<T>("[{}:{}] {}", __FILE__, __LINE__, result_##__LINE__##__COUNTER__.error().c_str());             \
        }                                                                                                                                  \
    } while (0)

} // namespace sf::render::vk
