#pragma once

//==============================================================================
// DEPRECATED: This header is deprecated and will be removed in a future version
//==============================================================================
// Migration Guide:
// - Replace d3d::AnsiToWString()    → sf::string_utils::to_wstring()
// - Replace d3d::WStringToANSI()    → sf::string_utils::to_string()
// - Replace d3d::Material           → sf::render::Material
// - Replace d3d::MaterialConstants  → sf::render::MaterialConstants
// - Replace d3d::Light              → sf::render::Light
// - Replace d3d::MAX_FRAMES_IN_FLIGHT → sf::render::MAX_FRAMES_IN_FLIGHT
// - Replace d3d::INVALID_INDEX_U32  → sf::render::INVALID_DESCRIPTOR_INDEX
// - Replace d3d::GraphicsDevice     → sf::render::IGraphicsDevice*
// - Replace d3d::Buffer             → sf::render::Buffer
// - Replace d3d::Texture            → sf::render::Texture
// New includes:
// - #include "core/string_utils.h"    for string conversion
// - #include "render/render_backend.h" for IGraphicsDevice
// - #include "render/resource_types.h" for Buffer, Texture, etc.
// - #include "render/material.h"       for Material types
// - #include "render/lights.h"         for Light type
//==============================================================================

#warning "render_compat.h is deprecated. Please migrate to sf::render:: and sf::string_utils:: namespaces. See header for migration guide."

// Temporary includes for backward compatibility
#include "render/render_api.h"
#include "render/resource_types.h"
#include "render/material.h"
#include "render/lights.h"
#include "core/string_utils.h"

namespace sf::d3d {

// DEPRECATED: Use sf::render:: types directly
using Buffer = sf::render::Buffer;
using Texture = sf::render::Texture;
using Material = sf::render::Material;
using MaterialConstants = sf::render::MaterialConstants;
using Light = sf::render::Light;

// DEPRECATED: Use sf::render::MAX_FRAMES_IN_FLIGHT
constexpr u32 MAX_FRAMES_IN_FLIGHT = sf::render::MAX_FRAMES_IN_FLIGHT;

// DEPRECATED: Use sf::render::INVALID_DESCRIPTOR_INDEX
constexpr u32 INVALID_INDEX_U32 = sf::render::INVALID_DESCRIPTOR_INDEX;

// DEPRECATED: Use sf::string_utils::to_wstring()
inline std::wstring AnsiToWString(const std::string& str) {
    return sf::string_utils::to_wstring(str);
}

// DEPRECATED: Use sf::string_utils::to_string()
inline std::string WStringToANSI(const std::wstring_view wstr) {
    return sf::string_utils::to_string(wstr);
}

} // namespace sf::d3d
