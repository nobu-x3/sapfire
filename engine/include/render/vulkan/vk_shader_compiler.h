#pragma once
#include <vulkan/vulkan.h>
#include "core/core.h"

namespace sf::render::vk {

    enum class ShaderType : u8 { Vertex, Fragment, Compute };

    struct Shader {
        stl::vector<u32> spirv_code;
        VkShaderModule module = VK_NULL_HANDLE;
    };

    Shader compile(VkDevice device, ShaderType type, const stl::string_view path, const stl::string_view entry_point);
    Shader compile(VkDevice device, ShaderType type, const stl::wstring_view path, const stl::wstring_view entry_point);

    void destroy_shader_module(VkDevice device, Shader& shader);

} // namespace sf::render::vk
