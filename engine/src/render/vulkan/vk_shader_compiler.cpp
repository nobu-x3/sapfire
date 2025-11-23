#include "engpch.h"
#include "render/vulkan/vk_shader_compiler.h"
#include "core/logger.h"
#include "core/file_system.h"
#include <fstream>

namespace sf::render::vk {

// Helper function to read binary SPIR-V file
static stl::vector<u32> read_spirv_file(const stl::string& path) {
    std::ifstream file(path.c_str(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        CORE_ERROR("Failed to open SPIR-V shader file: {}", path);
        return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    if (file_size % 4 != 0) {
        CORE_ERROR("SPIR-V file size is not a multiple of 4 bytes: {}", path);
        return {};
    }

    stl::vector<u32> buffer(mem::MemTag::Temp, file_size / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();

    return buffer;
}

Shader compile(VkDevice device, ShaderType type, const stl::string_view path, const stl::string_view entry_point) {
    Shader shader{};

    // Convert path to full path and append .spv extension if not present
    stl::string shader_path = fs::full_path(stl::string(path));
    if (shader_path.empty()) {
        shader_path = stl::string(path);
    }

    // If path doesn't end with .spv, add it
    if (shader_path.size() < 4 || shader_path.substr(shader_path.size() - 4) != ".spv") {
        shader_path += ".spv";
    }

    // Read SPIR-V binary
    shader.spirv_code = read_spirv_file(shader_path);
    if (shader.spirv_code.empty()) {
        CORE_ERROR("Failed to load SPIR-V shader: {}", shader_path);
        return shader;
    }

    // Create shader module
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = shader.spirv_code.size() * sizeof(u32);
    create_info.pCode = shader.spirv_code.data();

    VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &shader.module);
    if (result != VK_SUCCESS) {
        CORE_ERROR("Failed to create shader module for: {}. Error code: {}", shader_path, static_cast<int>(result));
        shader.spirv_code.clear();
        return shader;
    }

    CORE_INFO("Loaded SPIR-V shader: {}", shader_path);
    return shader;
}

Shader compile(VkDevice device, ShaderType type, const stl::wstring_view path, const stl::wstring_view entry_point) {
    stl::string narrow_path(path.begin(), path.end());
    stl::string narrow_entry(entry_point.begin(), entry_point.end());
    return compile(device, type, narrow_path, narrow_entry);
}

void destroy_shader_module(VkDevice device, Shader& shader) {
    if (shader.module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, shader.module, nullptr);
        shader.module = VK_NULL_HANDLE;
    }
    shader.spirv_code.clear();
}

} // namespace sf::render::vk
