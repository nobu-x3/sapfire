#pragma once

#include "core/base.h"

namespace sf::render {

// ============================================================================
// Render API Selection
// ============================================================================

enum class RenderAPI : u8 {
    None = 0,
    DX12,
    Vulkan
};

// ============================================================================
// Resource Formats
// ============================================================================

enum class Format : u32 {
    Unknown = 0,

    // 8-bit formats
    R8_UNORM,
    R8_SNORM,
    R8_UINT,
    R8_SINT,

    // 16-bit formats
    R16_FLOAT,
    R16_UNORM,
    R16_UINT,
    R16_SINT,
    R16_SNORM,

    RG8_UNORM,
    RG8_SNORM,
    RG8_UINT,
    RG8_SINT,

    // 32-bit formats
    R32_FLOAT,
    R32_UINT,
    R32_SINT,

    RG16_FLOAT,
    RG16_UNORM,
    RG16_UINT,
    RG16_SINT,
    RG16_SNORM,

    RGBA8_UNORM,
    RGBA8_UNORM_SRGB,
    RGBA8_SNORM,
    RGBA8_UINT,
    RGBA8_SINT,

    BGRA8_UNORM,
    BGRA8_UNORM_SRGB,

    RGB10A2_UNORM,
    RGB10A2_UINT,
    RG11B10_FLOAT,

    // 64-bit formats
    RG32_FLOAT,
    RG32_UINT,
    RG32_SINT,

    RGBA16_FLOAT,
    RGBA16_UNORM,
    RGBA16_UINT,
    RGBA16_SINT,
    RGBA16_SNORM,

    // 96-bit formats
    RGB32_FLOAT,
    RGB32_UINT,
    RGB32_SINT,

    // 128-bit formats
    RGBA32_FLOAT,
    RGBA32_UINT,
    RGBA32_SINT,

    // Depth-stencil formats
    D16_UNORM,
    D32_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT_S8X24_UINT,

    // Compressed formats
    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UF16,
    BC6H_SF16,
    BC7_UNORM,
    BC7_UNORM_SRGB,
};

// ============================================================================
// Resource States
// ============================================================================

enum class ResourceState : u32 {
    Common = 0,
    VertexAndConstantBuffer,
    IndexBuffer,
    RenderTarget,
    UnorderedAccess,
    DepthWrite,
    DepthRead,
    NonPixelShaderResource,
    PixelShaderResource,
    ShaderResource,
    StreamOut,
    IndirectArgument,
    CopyDest,
    CopySource,
    ResolveDest,
    ResolveSource,
    GenericRead,
    Present,
    Predication,
};

// ============================================================================
// Resource Usage Flags
// ============================================================================

enum class ResourceUsage : u32 {
    None = 0,
    RenderTarget = 1 << 0,
    DepthStencil = 1 << 1,
    UnorderedAccess = 1 << 2,
    ShaderResource = 1 << 3,
};

inline ResourceUsage operator|(ResourceUsage a, ResourceUsage b) {
    return static_cast<ResourceUsage>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline ResourceUsage operator&(ResourceUsage a, ResourceUsage b) {
    return static_cast<ResourceUsage>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool has_flag(ResourceUsage flags, ResourceUsage flag) {
    return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}

// ============================================================================
// Texture Types
// ============================================================================

enum class TextureType : u8 {
    Texture1D,
    Texture2D,
    Texture3D,
    TextureCube,
};

// ============================================================================
// Primitive Topology
// ============================================================================

enum class PrimitiveTopology : u8 {
    Undefined = 0,
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

// ============================================================================
// Cull Mode
// ============================================================================

enum class CullMode : u8 {
    None = 0,
    Front,
    Back,
};

// ============================================================================
// Fill Mode
// ============================================================================

enum class FillMode : u8 {
    Solid = 0,
    Wireframe,
};

// ============================================================================
// Blend Mode
// ============================================================================

enum class Blend : u8 {
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstAlpha,
    InvDstAlpha,
    DstColor,
    InvDstColor,
    SrcAlphaSat,
    BlendFactor,
    InvBlendFactor,
    Src1Color,
    InvSrc1Color,
    Src1Alpha,
    InvSrc1Alpha,
};

enum class BlendOp : u8 {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,
};

// ============================================================================
// Comparison Function
// ============================================================================

enum class CompareFunc : u8 {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

// ============================================================================
// Stencil Operation
// ============================================================================

enum class StencilOp : u8 {
    Keep = 0,
    Zero,
    Replace,
    IncrSat,
    DecrSat,
    Invert,
    Incr,
    Decr,
};

// ============================================================================
// Texture Addressing Mode
// ============================================================================

enum class TextureAddressMode : u8 {
    Wrap = 0,
    Mirror,
    Clamp,
    Border,
    MirrorOnce,
};

// ============================================================================
// Filter Mode
// ============================================================================

enum class Filter : u8 {
    MinMagMipPoint = 0,
    MinMagPointMipLinear,
    MinPointMagLinearMipPoint,
    MinPointMagMipLinear,
    MinLinearMagMipPoint,
    MinLinearMagPointMipLinear,
    MinMagLinearMipPoint,
    MinMagMipLinear,
    Anisotropic,
    ComparisonMinMagMipPoint,
    ComparisonMinMagPointMipLinear,
    ComparisonMinPointMagLinearMipPoint,
    ComparisonMinPointMagMipLinear,
    ComparisonMinLinearMagMipPoint,
    ComparisonMinLinearMagPointMipLinear,
    ComparisonMinMagLinearMipPoint,
    ComparisonMinMagMipLinear,
    ComparisonAnisotropic,
    MinimumMinMagMipPoint,
    MinimumMinMagPointMipLinear,
    MinimumMinPointMagLinearMipPoint,
    MinimumMinPointMagMipLinear,
    MinimumMinLinearMagMipPoint,
    MinimumMinLinearMagPointMipLinear,
    MinimumMinMagLinearMipPoint,
    MinimumMinMagMipLinear,
    MinimumAnisotropic,
    MaximumMinMagMipPoint,
    MaximumMinMagPointMipLinear,
    MaximumMinPointMagLinearMipPoint,
    MaximumMinPointMagMipLinear,
    MaximumMinLinearMagMipPoint,
    MaximumMinLinearMagPointMipLinear,
    MaximumMinMagLinearMipPoint,
    MaximumMinMagMipLinear,
    MaximumAnisotropic,
};

// ============================================================================
// Color Write Mask
// ============================================================================

enum class ColorWriteMask : u8 {
    None = 0,
    Red = 1 << 0,
    Green = 1 << 1,
    Blue = 1 << 2,
    Alpha = 1 << 3,
    All = Red | Green | Blue | Alpha,
};

inline ColorWriteMask operator|(ColorWriteMask a, ColorWriteMask b) {
    return static_cast<ColorWriteMask>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline ColorWriteMask operator&(ColorWriteMask a, ColorWriteMask b) {
    return static_cast<ColorWriteMask>(static_cast<u8>(a) & static_cast<u8>(b));
}

// ============================================================================
// Shader Stage
// ============================================================================

enum class ShaderStage : u8 {
    Vertex = 1 << 0,
    Hull = 1 << 1,
    Domain = 1 << 2,
    Geometry = 1 << 3,
    Pixel = 1 << 4,
    Compute = 1 << 5,

    AllGraphics = Vertex | Hull | Domain | Geometry | Pixel,
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline ShaderStage operator&(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<u8>(a) & static_cast<u8>(b));
}

// ============================================================================
// Command Queue Type
// ============================================================================

enum class CommandQueueType : u8 {
    Direct = 0,  // Graphics + Compute + Copy
    Compute,     // Compute + Copy
    Copy,        // Copy only
};

// ============================================================================
// Agnostic Structures
// ============================================================================

struct Viewport {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;
    f32 min_depth = 0.0f;
    f32 max_depth = 1.0f;
};

struct ScissorRect {
    i32 left = 0;
    i32 top = 0;
    i32 right = 0;
    i32 bottom = 0;
};

struct ResourceBarrier {
    void* resource = nullptr;  // Backend-specific handle
    ResourceState state_before = ResourceState::Common;
    ResourceState state_after = ResourceState::Common;
};

struct BlendState {
    bool blend_enable = false;
    Blend src_blend = Blend::One;
    Blend dst_blend = Blend::Zero;
    BlendOp blend_op = BlendOp::Add;
    Blend src_blend_alpha = Blend::One;
    Blend dst_blend_alpha = Blend::Zero;
    BlendOp blend_op_alpha = BlendOp::Add;
    ColorWriteMask write_mask = ColorWriteMask::All;
};

struct RasterizerState {
    FillMode fill_mode = FillMode::Solid;
    CullMode cull_mode = CullMode::Back;
    bool front_counter_clockwise = false;
    i32 depth_bias = 0;
    f32 depth_bias_clamp = 0.0f;
    f32 slope_scaled_depth_bias = 0.0f;
    bool depth_clip_enable = true;
    bool multisample_enable = false;
    bool antialiased_line_enable = false;
};

struct DepthStencilState {
    bool depth_enable = true;
    bool depth_write_enable = true;
    CompareFunc depth_func = CompareFunc::Less;
    bool stencil_enable = false;
    u8 stencil_read_mask = 0xFF;
    u8 stencil_write_mask = 0xFF;

    struct StencilOpDesc {
        StencilOp fail_op = StencilOp::Keep;
        StencilOp depth_fail_op = StencilOp::Keep;
        StencilOp pass_op = StencilOp::Keep;
        CompareFunc stencil_func = CompareFunc::Always;
    };

    StencilOpDesc front_face;
    StencilOpDesc back_face;
};

struct SamplerDesc {
    Filter filter = Filter::MinMagMipLinear;
    TextureAddressMode address_u = TextureAddressMode::Wrap;
    TextureAddressMode address_v = TextureAddressMode::Wrap;
    TextureAddressMode address_w = TextureAddressMode::Wrap;
    f32 mip_lod_bias = 0.0f;
    u32 max_anisotropy = 16;
    CompareFunc comparison_func = CompareFunc::Never;
    f32 border_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    f32 min_lod = 0.0f;
    f32 max_lod = 3.402823466e+38f;  // FLT_MAX
};

} // namespace sf::render
