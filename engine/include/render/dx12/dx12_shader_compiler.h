#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include <dxcapi.h>
#include <wrl.h>
#include "core/core.h"

namespace sf::render::dx12 {

enum class ShaderType : u8 {
    Vertex,
    Pixel,
    Compute,
    RootSignature
};

struct Shader {
    Microsoft::WRL::ComPtr<IDxcBlob> shader_blob{};
    Microsoft::WRL::ComPtr<IDxcBlob> root_signature_blob{};
};

Shader compile(ShaderType type, const stl::string_view path, const stl::string_view entry_point,
               bool extract_root_signature = false);

Shader compile(ShaderType type, const stl::wstring_view path, const stl::wstring_view entry_point,
               bool extract_root_signature = false);

} // namespace sf::render::dx12

// Backward compatibility for old namespace
namespace sf::tools::shader_compiler {
    using sf::render::dx12::Shader;
    using sf::render::dx12::compile;
}

namespace sf::d3d {
    using ShaderType = sf::render::dx12::ShaderType;
}
#endif