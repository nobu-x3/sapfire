#pragma once
#include <vulkan/vulkan.h>
#include "render/render_api.h"

namespace sf::render::vk {

    // Format Conversions

    inline VkFormat to_vk_format(Format format) {
        switch (format) {
        case Format::Unknown:
            return VK_FORMAT_UNDEFINED;

        // 8-bit formats
        case Format::R8_UNORM:
            return VK_FORMAT_R8_UNORM;
        case Format::R8_SNORM:
            return VK_FORMAT_R8_SNORM;
        case Format::R8_UINT:
            return VK_FORMAT_R8_UINT;
        case Format::R8_SINT:
            return VK_FORMAT_R8_SINT;

        // 16-bit formats
        case Format::R16_FLOAT:
            return VK_FORMAT_R16_SFLOAT;
        case Format::R16_UNORM:
            return VK_FORMAT_R16_UNORM;
        case Format::R16_UINT:
            return VK_FORMAT_R16_UINT;
        case Format::R16_SINT:
            return VK_FORMAT_R16_SINT;
        case Format::R16_SNORM:
            return VK_FORMAT_R16_SNORM;

        case Format::RG8_UNORM:
            return VK_FORMAT_R8G8_UNORM;
        case Format::RG8_SNORM:
            return VK_FORMAT_R8G8_SNORM;
        case Format::RG8_UINT:
            return VK_FORMAT_R8G8_UINT;
        case Format::RG8_SINT:
            return VK_FORMAT_R8G8_SINT;

        // 32-bit formats
        case Format::R32_FLOAT:
            return VK_FORMAT_R32_SFLOAT;
        case Format::R32_UINT:
            return VK_FORMAT_R32_UINT;
        case Format::R32_SINT:
            return VK_FORMAT_R32_SINT;

        case Format::RG16_FLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case Format::RG16_UNORM:
            return VK_FORMAT_R16G16_UNORM;
        case Format::RG16_UINT:
            return VK_FORMAT_R16G16_UINT;
        case Format::RG16_SINT:
            return VK_FORMAT_R16G16_SINT;
        case Format::RG16_SNORM:
            return VK_FORMAT_R16G16_SNORM;

        case Format::RGBA8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_UNORM_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::RGBA8_SNORM:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case Format::RGBA8_UINT:
            return VK_FORMAT_R8G8B8A8_UINT;
        case Format::RGBA8_SINT:
            return VK_FORMAT_R8G8B8A8_SINT;

        case Format::BGRA8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::BGRA8_UNORM_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;

        case Format::RGB10A2_UNORM:
            return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        case Format::RGB10A2_UINT:
            return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        case Format::RG11B10_FLOAT:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        // 64-bit formats
        case Format::RG32_FLOAT:
            return VK_FORMAT_R32G32_SFLOAT;
        case Format::RG32_UINT:
            return VK_FORMAT_R32G32_UINT;
        case Format::RG32_SINT:
            return VK_FORMAT_R32G32_SINT;

        case Format::RGBA16_FLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::RGBA16_UNORM:
            return VK_FORMAT_R16G16B16A16_UNORM;
        case Format::RGBA16_UINT:
            return VK_FORMAT_R16G16B16A16_UINT;
        case Format::RGBA16_SINT:
            return VK_FORMAT_R16G16B16A16_SINT;
        case Format::RGBA16_SNORM:
            return VK_FORMAT_R16G16B16A16_SNORM;

        // 96-bit formats
        case Format::RGB32_FLOAT:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::RGB32_UINT:
            return VK_FORMAT_R32G32B32_UINT;
        case Format::RGB32_SINT:
            return VK_FORMAT_R32G32B32_SINT;

        // 128-bit formats
        case Format::RGBA32_FLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Format::RGBA32_UINT:
            return VK_FORMAT_R32G32B32A32_UINT;
        case Format::RGBA32_SINT:
            return VK_FORMAT_R32G32B32A32_SINT;

        case Format::D16_UNORM:
            return VK_FORMAT_D16_UNORM;
        case Format::D32_FLOAT:
            return VK_FORMAT_D32_SFLOAT;
        case Format::D24_UNORM_S8_UINT:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32_FLOAT_S8X24_UINT:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;

        case Format::BC1_UNORM:
            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case Format::BC1_UNORM_SRGB:
            return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case Format::BC2_UNORM:
            return VK_FORMAT_BC2_UNORM_BLOCK;
        case Format::BC2_UNORM_SRGB:
            return VK_FORMAT_BC2_SRGB_BLOCK;
        case Format::BC3_UNORM:
            return VK_FORMAT_BC3_UNORM_BLOCK;
        case Format::BC3_UNORM_SRGB:
            return VK_FORMAT_BC3_SRGB_BLOCK;
        case Format::BC4_UNORM:
            return VK_FORMAT_BC4_UNORM_BLOCK;
        case Format::BC4_SNORM:
            return VK_FORMAT_BC4_SNORM_BLOCK;
        case Format::BC5_UNORM:
            return VK_FORMAT_BC5_UNORM_BLOCK;
        case Format::BC5_SNORM:
            return VK_FORMAT_BC5_SNORM_BLOCK;
        case Format::BC6H_UF16:
            return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case Format::BC6H_SF16:
            return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case Format::BC7_UNORM:
            return VK_FORMAT_BC7_UNORM_BLOCK;
        case Format::BC7_UNORM_SRGB:
            return VK_FORMAT_BC7_SRGB_BLOCK;

        default:
            return VK_FORMAT_UNDEFINED;
        }
    }

    inline Format from_vk_format(VkFormat format) {
        switch (format) {
        case VK_FORMAT_UNDEFINED:
            return Format::Unknown;
        case VK_FORMAT_R8_UNORM:
            return Format::R8_UNORM;
        case VK_FORMAT_R8_SNORM:
            return Format::R8_SNORM;
        case VK_FORMAT_R8_UINT:
            return Format::R8_UINT;
        case VK_FORMAT_R8_SINT:
            return Format::R8_SINT;
        case VK_FORMAT_R16_SFLOAT:
            return Format::R16_FLOAT;
        case VK_FORMAT_R16_UNORM:
            return Format::R16_UNORM;
        case VK_FORMAT_R16_UINT:
            return Format::R16_UINT;
        case VK_FORMAT_R16_SINT:
            return Format::R16_SINT;
        case VK_FORMAT_R16_SNORM:
            return Format::R16_SNORM;
        case VK_FORMAT_R8G8_UNORM:
            return Format::RG8_UNORM;
        case VK_FORMAT_R8G8_SNORM:
            return Format::RG8_SNORM;
        case VK_FORMAT_R8G8_UINT:
            return Format::RG8_UINT;
        case VK_FORMAT_R8G8_SINT:
            return Format::RG8_SINT;
        case VK_FORMAT_R32_SFLOAT:
            return Format::R32_FLOAT;
        case VK_FORMAT_R32_UINT:
            return Format::R32_UINT;
        case VK_FORMAT_R32_SINT:
            return Format::R32_SINT;
        case VK_FORMAT_R16G16_SFLOAT:
            return Format::RG16_FLOAT;
        case VK_FORMAT_R16G16_UNORM:
            return Format::RG16_UNORM;
        case VK_FORMAT_R16G16_UINT:
            return Format::RG16_UINT;
        case VK_FORMAT_R16G16_SINT:
            return Format::RG16_SINT;
        case VK_FORMAT_R16G16_SNORM:
            return Format::RG16_SNORM;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return Format::RGBA8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return Format::RGBA8_UNORM_SRGB;
        case VK_FORMAT_R8G8B8A8_SNORM:
            return Format::RGBA8_SNORM;
        case VK_FORMAT_R8G8B8A8_UINT:
            return Format::RGBA8_UINT;
        case VK_FORMAT_R8G8B8A8_SINT:
            return Format::RGBA8_SINT;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return Format::BGRA8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return Format::BGRA8_UNORM_SRGB;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            return Format::RGB10A2_UNORM;
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            return Format::RGB10A2_UINT;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return Format::RG11B10_FLOAT;
        case VK_FORMAT_R32G32_SFLOAT:
            return Format::RG32_FLOAT;
        case VK_FORMAT_R32G32_UINT:
            return Format::RG32_UINT;
        case VK_FORMAT_R32G32_SINT:
            return Format::RG32_SINT;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return Format::RGBA16_FLOAT;
        case VK_FORMAT_R16G16B16A16_UNORM:
            return Format::RGBA16_UNORM;
        case VK_FORMAT_R16G16B16A16_UINT:
            return Format::RGBA16_UINT;
        case VK_FORMAT_R16G16B16A16_SINT:
            return Format::RGBA16_SINT;
        case VK_FORMAT_R16G16B16A16_SNORM:
            return Format::RGBA16_SNORM;
        case VK_FORMAT_R32G32B32_SFLOAT:
            return Format::RGB32_FLOAT;
        case VK_FORMAT_R32G32B32_UINT:
            return Format::RGB32_UINT;
        case VK_FORMAT_R32G32B32_SINT:
            return Format::RGB32_SINT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return Format::RGBA32_FLOAT;
        case VK_FORMAT_R32G32B32A32_UINT:
            return Format::RGBA32_UINT;
        case VK_FORMAT_R32G32B32A32_SINT:
            return Format::RGBA32_SINT;
        case VK_FORMAT_D16_UNORM:
            return Format::D16_UNORM;
        case VK_FORMAT_D32_SFLOAT:
            return Format::D32_FLOAT;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return Format::D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return Format::D32_FLOAT_S8X24_UINT;
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            return Format::BC1_UNORM;
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return Format::BC1_UNORM_SRGB;
        case VK_FORMAT_BC2_UNORM_BLOCK:
            return Format::BC2_UNORM;
        case VK_FORMAT_BC2_SRGB_BLOCK:
            return Format::BC2_UNORM_SRGB;
        case VK_FORMAT_BC3_UNORM_BLOCK:
            return Format::BC3_UNORM;
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return Format::BC3_UNORM_SRGB;
        case VK_FORMAT_BC4_UNORM_BLOCK:
            return Format::BC4_UNORM;
        case VK_FORMAT_BC4_SNORM_BLOCK:
            return Format::BC4_SNORM;
        case VK_FORMAT_BC5_UNORM_BLOCK:
            return Format::BC5_UNORM;
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return Format::BC5_SNORM;
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
            return Format::BC6H_UF16;
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            return Format::BC6H_SF16;
        case VK_FORMAT_BC7_UNORM_BLOCK:
            return Format::BC7_UNORM;
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return Format::BC7_UNORM_SRGB;
        default:
            return Format::Unknown;
        }
    }

    // Resource State (Image Layout) Conversions

    inline VkImageLayout to_vk_image_layout(ResourceState state) {
        switch (state) {
        case ResourceState::Common:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceState::RenderTarget:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ResourceState::UnorderedAccess:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceState::DepthWrite:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ResourceState::DepthRead:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case ResourceState::ShaderResource:
        case ResourceState::PixelShaderResource:
        case ResourceState::NonPixelShaderResource:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ResourceState::CopyDest:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ResourceState::CopySource:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ResourceState::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        default:
            return VK_IMAGE_LAYOUT_GENERAL;
        }
    }

    inline VkAccessFlags to_vk_access_flags(ResourceState state) {
        switch (state) {
        case ResourceState::Common:
            return 0;
        case ResourceState::VertexAndConstantBuffer:
            return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
        case ResourceState::IndexBuffer:
            return VK_ACCESS_INDEX_READ_BIT;
        case ResourceState::RenderTarget:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case ResourceState::UnorderedAccess:
            return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        case ResourceState::DepthWrite:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case ResourceState::DepthRead:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case ResourceState::ShaderResource:
        case ResourceState::PixelShaderResource:
        case ResourceState::NonPixelShaderResource:
            return VK_ACCESS_SHADER_READ_BIT;
        case ResourceState::CopyDest:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case ResourceState::CopySource:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case ResourceState::Present:
            return 0;
        default:
            return 0;
        }
    }

    inline VkPipelineStageFlags to_vk_pipeline_stage(ResourceState state) {
        switch (state) {
        case ResourceState::Common:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case ResourceState::VertexAndConstantBuffer:
            return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case ResourceState::IndexBuffer:
            return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        case ResourceState::RenderTarget:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case ResourceState::UnorderedAccess:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case ResourceState::DepthWrite:
        case ResourceState::DepthRead:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case ResourceState::ShaderResource:
        case ResourceState::PixelShaderResource:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case ResourceState::NonPixelShaderResource:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case ResourceState::CopyDest:
        case ResourceState::CopySource:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case ResourceState::Present:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        default:
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }
    }

    // Primitive Topology Conversions

    inline VkPrimitiveTopology to_vk_primitive_topology(PrimitiveTopology topology) {
        switch (topology) {
        case PrimitiveTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PrimitiveTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        default:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    // Cull Mode Conversions

    inline VkCullModeFlags to_vk_cull_mode(CullMode mode) {
        switch (mode) {
        case CullMode::None:
            return VK_CULL_MODE_NONE;
        case CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:
            return VK_CULL_MODE_BACK_BIT;
        default:
            return VK_CULL_MODE_BACK_BIT;
        }
    }

    // Fill Mode Conversions

    inline VkPolygonMode to_vk_polygon_mode(FillMode mode) {
        switch (mode) {
        case FillMode::Solid:
            return VK_POLYGON_MODE_FILL;
        case FillMode::Wireframe:
            return VK_POLYGON_MODE_LINE;
        default:
            return VK_POLYGON_MODE_FILL;
        }
    }

    // Blend Conversions

    inline VkBlendFactor to_vk_blend_factor(Blend blend) {
        switch (blend) {
        case Blend::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case Blend::One:
            return VK_BLEND_FACTOR_ONE;
        case Blend::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case Blend::InvSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case Blend::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case Blend::InvSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case Blend::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case Blend::InvDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case Blend::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case Blend::InvDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case Blend::SrcAlphaSat:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case Blend::BlendFactor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case Blend::InvBlendFactor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case Blend::Src1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case Blend::InvSrc1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case Blend::Src1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case Blend::InvSrc1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            return VK_BLEND_FACTOR_ZERO;
        }
    }

    inline VkBlendOp to_vk_blend_op(BlendOp op) {
        switch (op) {
        case BlendOp::Add:
            return VK_BLEND_OP_ADD;
        case BlendOp::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case BlendOp::RevSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min:
            return VK_BLEND_OP_MIN;
        case BlendOp::Max:
            return VK_BLEND_OP_MAX;
        default:
            return VK_BLEND_OP_ADD;
        }
    }

    // Comparison Function Conversions

    inline VkCompareOp to_vk_compare_op(CompareFunc func) {
        switch (func) {
        case CompareFunc::Never:
            return VK_COMPARE_OP_NEVER;
        case CompareFunc::Less:
            return VK_COMPARE_OP_LESS;
        case CompareFunc::Equal:
            return VK_COMPARE_OP_EQUAL;
        case CompareFunc::LessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareFunc::Greater:
            return VK_COMPARE_OP_GREATER;
        case CompareFunc::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case CompareFunc::GreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareFunc::Always:
            return VK_COMPARE_OP_ALWAYS;
        default:
            return VK_COMPARE_OP_NEVER;
        }
    }

    // Stencil Op Conversions

    inline VkStencilOp to_vk_stencil_op(StencilOp op) {
        switch (op) {
        case StencilOp::Keep:
            return VK_STENCIL_OP_KEEP;
        case StencilOp::Zero:
            return VK_STENCIL_OP_ZERO;
        case StencilOp::Replace:
            return VK_STENCIL_OP_REPLACE;
        case StencilOp::IncrSat:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case StencilOp::DecrSat:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case StencilOp::Invert:
            return VK_STENCIL_OP_INVERT;
        case StencilOp::Incr:
            return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case StencilOp::Decr:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default:
            return VK_STENCIL_OP_KEEP;
        }
    }

    // Texture Address Mode Conversions

    inline VkSamplerAddressMode to_vk_sampler_address_mode(TextureAddressMode mode) {
        switch (mode) {
        case TextureAddressMode::Wrap:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureAddressMode::Mirror:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TextureAddressMode::Clamp:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureAddressMode::Border:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case TextureAddressMode::MirrorOnce:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    // Filter Conversions

    inline void to_vk_sampler_filter(Filter filter, VkFilter& minFilter, VkFilter& magFilter, VkSamplerMipmapMode& mipMode,
                                     bool& anisotropyEnable) {
        anisotropyEnable = false;

        switch (filter) {
        case Filter::MinMagMipPoint:
            minFilter = VK_FILTER_NEAREST;
            magFilter = VK_FILTER_NEAREST;
            mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case Filter::MinMagPointMipLinear:
            minFilter = VK_FILTER_NEAREST;
            magFilter = VK_FILTER_NEAREST;
            mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case Filter::MinPointMagLinearMipPoint:
            minFilter = VK_FILTER_NEAREST;
            magFilter = VK_FILTER_LINEAR;
            mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case Filter::MinPointMagMipLinear:
            minFilter = VK_FILTER_NEAREST;
            magFilter = VK_FILTER_LINEAR;
            mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case Filter::MinLinearMagMipPoint:
            minFilter = VK_FILTER_LINEAR;
            magFilter = VK_FILTER_NEAREST;
            mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case Filter::MinLinearMagPointMipLinear:
            minFilter = VK_FILTER_LINEAR;
            magFilter = VK_FILTER_NEAREST;
            mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case Filter::MinMagLinearMipPoint:
            minFilter = VK_FILTER_LINEAR;
            magFilter = VK_FILTER_LINEAR;
            mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case Filter::MinMagMipLinear:
        default:
            minFilter = VK_FILTER_LINEAR;
            magFilter = VK_FILTER_LINEAR;
            mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case Filter::Anisotropic:
            minFilter = VK_FILTER_LINEAR;
            magFilter = VK_FILTER_LINEAR;
            mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            anisotropyEnable = true;
            break;
        }
    }

    // Command Queue Type Conversions

    inline VkQueueFlags to_vk_queue_flags(CommandQueueType type) {
        switch (type) {
        case CommandQueueType::Direct:
            return VK_QUEUE_GRAPHICS_BIT;
        case CommandQueueType::Compute:
            return VK_QUEUE_COMPUTE_BIT;
        case CommandQueueType::Copy:
            return VK_QUEUE_TRANSFER_BIT;
        default:
            return VK_QUEUE_GRAPHICS_BIT;
        }
    }

} // namespace sf::render::vk
