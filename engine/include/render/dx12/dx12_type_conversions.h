#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include <d3d12.h>
#include <dxgiformat.h>
#include "render/render_api.h"

namespace sf::render::dx12 {

    // Format Conversions

    inline DXGI_FORMAT to_dxgi_format(Format format) {
        switch (format) {
        case Format::Unknown:
            return DXGI_FORMAT_UNKNOWN;

        // 8-bit formats
        case Format::R8_UNORM:
            return DXGI_FORMAT_R8_UNORM;
        case Format::R8_SNORM:
            return DXGI_FORMAT_R8_SNORM;
        case Format::R8_UINT:
            return DXGI_FORMAT_R8_UINT;
        case Format::R8_SINT:
            return DXGI_FORMAT_R8_SINT;

        // 16-bit formats
        case Format::R16_FLOAT:
            return DXGI_FORMAT_R16_FLOAT;
        case Format::R16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case Format::R16_UINT:
            return DXGI_FORMAT_R16_UINT;
        case Format::R16_SINT:
            return DXGI_FORMAT_R16_SINT;
        case Format::R16_SNORM:
            return DXGI_FORMAT_R16_SNORM;

        case Format::RG8_UNORM:
            return DXGI_FORMAT_R8G8_UNORM;
        case Format::RG8_SNORM:
            return DXGI_FORMAT_R8G8_SNORM;
        case Format::RG8_UINT:
            return DXGI_FORMAT_R8G8_UINT;
        case Format::RG8_SINT:
            return DXGI_FORMAT_R8G8_SINT;

        // 32-bit formats
        case Format::R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case Format::R32_UINT:
            return DXGI_FORMAT_R32_UINT;
        case Format::R32_SINT:
            return DXGI_FORMAT_R32_SINT;

        case Format::RG16_FLOAT:
            return DXGI_FORMAT_R16G16_FLOAT;
        case Format::RG16_UNORM:
            return DXGI_FORMAT_R16G16_UNORM;
        case Format::RG16_UINT:
            return DXGI_FORMAT_R16G16_UINT;
        case Format::RG16_SINT:
            return DXGI_FORMAT_R16G16_SINT;
        case Format::RG16_SNORM:
            return DXGI_FORMAT_R16G16_SNORM;

        case Format::RGBA8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::RGBA8_SNORM:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case Format::RGBA8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case Format::RGBA8_SINT:
            return DXGI_FORMAT_R8G8B8A8_SINT;

        case Format::BGRA8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case Format::BGRA8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case Format::RGB10A2_UNORM:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case Format::RGB10A2_UINT:
            return DXGI_FORMAT_R10G10B10A2_UINT;
        case Format::RG11B10_FLOAT:
            return DXGI_FORMAT_R11G11B10_FLOAT;

        // 64-bit formats
        case Format::RG32_FLOAT:
            return DXGI_FORMAT_R32G32_FLOAT;
        case Format::RG32_UINT:
            return DXGI_FORMAT_R32G32_UINT;
        case Format::RG32_SINT:
            return DXGI_FORMAT_R32G32_SINT;

        case Format::RGBA16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::RGBA16_UNORM:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case Format::RGBA16_UINT:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case Format::RGBA16_SINT:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case Format::RGBA16_SNORM:
            return DXGI_FORMAT_R16G16B16A16_SNORM;

        // 96-bit formats
        case Format::RGB32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::RGB32_UINT:
            return DXGI_FORMAT_R32G32B32_UINT;
        case Format::RGB32_SINT:
            return DXGI_FORMAT_R32G32B32_SINT;

        // 128-bit formats
        case Format::RGBA32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::RGBA32_UINT:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case Format::RGBA32_SINT:
            return DXGI_FORMAT_R32G32B32A32_SINT;

        case Format::D16_UNORM:
            return DXGI_FORMAT_D16_UNORM;
        case Format::D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        case Format::D24_UNORM_S8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        case Format::BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM;
        case Format::BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case Format::BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM;
        case Format::BC2_UNORM_SRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case Format::BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM;
        case Format::BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case Format::BC4_UNORM:
            return DXGI_FORMAT_BC4_UNORM;
        case Format::BC4_SNORM:
            return DXGI_FORMAT_BC4_SNORM;
        case Format::BC5_UNORM:
            return DXGI_FORMAT_BC5_UNORM;
        case Format::BC5_SNORM:
            return DXGI_FORMAT_BC5_SNORM;
        case Format::BC6H_UF16:
            return DXGI_FORMAT_BC6H_UF16;
        case Format::BC6H_SF16:
            return DXGI_FORMAT_BC6H_SF16;
        case Format::BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM;
        case Format::BC7_UNORM_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    inline Format from_dxgi_format(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_UNKNOWN:
            return Format::Unknown;
        case DXGI_FORMAT_R8_UNORM:
            return Format::R8_UNORM;
        case DXGI_FORMAT_R8_SNORM:
            return Format::R8_SNORM;
        case DXGI_FORMAT_R8_UINT:
            return Format::R8_UINT;
        case DXGI_FORMAT_R8_SINT:
            return Format::R8_SINT;
        case DXGI_FORMAT_R16_FLOAT:
            return Format::R16_FLOAT;
        case DXGI_FORMAT_R16_UNORM:
            return Format::R16_UNORM;
        case DXGI_FORMAT_R16_UINT:
            return Format::R16_UINT;
        case DXGI_FORMAT_R16_SINT:
            return Format::R16_SINT;
        case DXGI_FORMAT_R16_SNORM:
            return Format::R16_SNORM;
        case DXGI_FORMAT_R8G8_UNORM:
            return Format::RG8_UNORM;
        case DXGI_FORMAT_R8G8_SNORM:
            return Format::RG8_SNORM;
        case DXGI_FORMAT_R8G8_UINT:
            return Format::RG8_UINT;
        case DXGI_FORMAT_R8G8_SINT:
            return Format::RG8_SINT;
        case DXGI_FORMAT_R32_FLOAT:
            return Format::R32_FLOAT;
        case DXGI_FORMAT_R32_UINT:
            return Format::R32_UINT;
        case DXGI_FORMAT_R32_SINT:
            return Format::R32_SINT;
        case DXGI_FORMAT_R16G16_FLOAT:
            return Format::RG16_FLOAT;
        case DXGI_FORMAT_R16G16_UNORM:
            return Format::RG16_UNORM;
        case DXGI_FORMAT_R16G16_UINT:
            return Format::RG16_UINT;
        case DXGI_FORMAT_R16G16_SINT:
            return Format::RG16_SINT;
        case DXGI_FORMAT_R16G16_SNORM:
            return Format::RG16_SNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return Format::RGBA8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return Format::RGBA8_UNORM_SRGB;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return Format::RGBA8_SNORM;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return Format::RGBA8_UINT;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return Format::RGBA8_SINT;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return Format::BGRA8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return Format::BGRA8_UNORM_SRGB;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return Format::RGB10A2_UNORM;
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return Format::RGB10A2_UINT;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return Format::RG11B10_FLOAT;
        case DXGI_FORMAT_R32G32_FLOAT:
            return Format::RG32_FLOAT;
        case DXGI_FORMAT_R32G32_UINT:
            return Format::RG32_UINT;
        case DXGI_FORMAT_R32G32_SINT:
            return Format::RG32_SINT;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return Format::RGBA16_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return Format::RGBA16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return Format::RGBA16_UINT;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return Format::RGBA16_SINT;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return Format::RGBA16_SNORM;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return Format::RGB32_FLOAT;
        case DXGI_FORMAT_R32G32B32_UINT:
            return Format::RGB32_UINT;
        case DXGI_FORMAT_R32G32B32_SINT:
            return Format::RGB32_SINT;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return Format::RGBA32_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return Format::RGBA32_UINT;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return Format::RGBA32_SINT;
        case DXGI_FORMAT_D16_UNORM:
            return Format::D16_UNORM;
        case DXGI_FORMAT_D32_FLOAT:
            return Format::D32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return Format::D24_UNORM_S8_UINT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return Format::D32_FLOAT_S8X24_UINT;
        case DXGI_FORMAT_BC1_UNORM:
            return Format::BC1_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return Format::BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM:
            return Format::BC2_UNORM;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return Format::BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:
            return Format::BC3_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return Format::BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC4_UNORM:
            return Format::BC4_UNORM;
        case DXGI_FORMAT_BC4_SNORM:
            return Format::BC4_SNORM;
        case DXGI_FORMAT_BC5_UNORM:
            return Format::BC5_UNORM;
        case DXGI_FORMAT_BC5_SNORM:
            return Format::BC5_SNORM;
        case DXGI_FORMAT_BC6H_UF16:
            return Format::BC6H_UF16;
        case DXGI_FORMAT_BC6H_SF16:
            return Format::BC6H_SF16;
        case DXGI_FORMAT_BC7_UNORM:
            return Format::BC7_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return Format::BC7_UNORM_SRGB;
        default:
            return Format::Unknown;
        }
    }

    // Resource State Conversions

    inline D3D12_RESOURCE_STATES to_d3d12_resource_state(ResourceState state) {
        switch (state) {
        case ResourceState::Common:
            return D3D12_RESOURCE_STATE_COMMON;
        case ResourceState::VertexAndConstantBuffer:
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case ResourceState::IndexBuffer:
            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case ResourceState::RenderTarget:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ResourceState::UnorderedAccess:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case ResourceState::DepthWrite:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case ResourceState::DepthRead:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case ResourceState::NonPixelShaderResource:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case ResourceState::PixelShaderResource:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case ResourceState::ShaderResource:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case ResourceState::StreamOut:
            return D3D12_RESOURCE_STATE_STREAM_OUT;
        case ResourceState::IndirectArgument:
            return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case ResourceState::CopyDest:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceState::CopySource:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceState::ResolveDest:
            return D3D12_RESOURCE_STATE_RESOLVE_DEST;
        case ResourceState::ResolveSource:
            return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        case ResourceState::GenericRead:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case ResourceState::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        case ResourceState::Predication:
            return D3D12_RESOURCE_STATE_PREDICATION;
        default:
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    // Primitive Topology Conversions

    inline D3D_PRIMITIVE_TOPOLOGY to_d3d12_primitive_topology(PrimitiveTopology topology) {
        switch (topology) {
        case PrimitiveTopology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
    }

    inline D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(PrimitiveTopology topology) {
        switch (topology) {
        case PrimitiveTopology::PointList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopology::LineList:
        case PrimitiveTopology::LineStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::TriangleList:
        case PrimitiveTopology::TriangleStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        }
    }

    // Cull Mode Conversions

    inline D3D12_CULL_MODE to_d3d12_cull_mode(CullMode mode) {
        switch (mode) {
        case CullMode::None:
            return D3D12_CULL_MODE_NONE;
        case CullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        case CullMode::Back:
            return D3D12_CULL_MODE_BACK;
        default:
            return D3D12_CULL_MODE_BACK;
        }
    }

    // Fill Mode Conversions

    inline D3D12_FILL_MODE to_d3d12_fill_mode(FillMode mode) {
        switch (mode) {
        case FillMode::Solid:
            return D3D12_FILL_MODE_SOLID;
        case FillMode::Wireframe:
            return D3D12_FILL_MODE_WIREFRAME;
        default:
            return D3D12_FILL_MODE_SOLID;
        }
    }

    // Blend Conversions

    inline D3D12_BLEND to_d3d12_blend(Blend blend) {
        switch (blend) {
        case Blend::Zero:
            return D3D12_BLEND_ZERO;
        case Blend::One:
            return D3D12_BLEND_ONE;
        case Blend::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case Blend::InvSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case Blend::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case Blend::InvSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case Blend::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case Blend::InvDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case Blend::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case Blend::InvDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case Blend::SrcAlphaSat:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case Blend::BlendFactor:
            return D3D12_BLEND_BLEND_FACTOR;
        case Blend::InvBlendFactor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case Blend::Src1Color:
            return D3D12_BLEND_SRC1_COLOR;
        case Blend::InvSrc1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case Blend::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case Blend::InvSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            return D3D12_BLEND_ZERO;
        }
    }

    inline D3D12_BLEND_OP to_d3d12_blend_op(BlendOp op) {
        switch (op) {
        case BlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case BlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case BlendOp::RevSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case BlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case BlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            return D3D12_BLEND_OP_ADD;
        }
    }

    // Comparison Function Conversions

    inline D3D12_COMPARISON_FUNC to_d3d12_comparison_func(CompareFunc func) {
        switch (func) {
        case CompareFunc::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case CompareFunc::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case CompareFunc::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case CompareFunc::LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case CompareFunc::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case CompareFunc::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case CompareFunc::GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case CompareFunc::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            return D3D12_COMPARISON_FUNC_NEVER;
        }
    }

    // Stencil Op Conversions

    inline D3D12_STENCIL_OP to_d3d12_stencil_op(StencilOp op) {
        switch (op) {
        case StencilOp::Keep:
            return D3D12_STENCIL_OP_KEEP;
        case StencilOp::Zero:
            return D3D12_STENCIL_OP_ZERO;
        case StencilOp::Replace:
            return D3D12_STENCIL_OP_REPLACE;
        case StencilOp::IncrSat:
            return D3D12_STENCIL_OP_INCR_SAT;
        case StencilOp::DecrSat:
            return D3D12_STENCIL_OP_DECR_SAT;
        case StencilOp::Invert:
            return D3D12_STENCIL_OP_INVERT;
        case StencilOp::Incr:
            return D3D12_STENCIL_OP_INCR;
        case StencilOp::Decr:
            return D3D12_STENCIL_OP_DECR;
        default:
            return D3D12_STENCIL_OP_KEEP;
        }
    }

    // Texture Address Mode Conversions

    inline D3D12_TEXTURE_ADDRESS_MODE to_d3d12_texture_address_mode(TextureAddressMode mode) {
        switch (mode) {
        case TextureAddressMode::Wrap:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case TextureAddressMode::Mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case TextureAddressMode::Clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case TextureAddressMode::Border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case TextureAddressMode::MirrorOnce:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }

    // Filter Conversions

    inline D3D12_FILTER to_d3d12_filter(Filter filter) {
        switch (filter) {
        case Filter::MinMagMipPoint:
            return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case Filter::MinMagPointMipLinear:
            return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MinPointMagLinearMipPoint:
            return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MinPointMagMipLinear:
            return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MinLinearMagMipPoint:
            return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MinLinearMagPointMipLinear:
            return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MinMagLinearMipPoint:
            return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MinMagMipLinear:
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case Filter::Anisotropic:
            return D3D12_FILTER_ANISOTROPIC;
        case Filter::ComparisonMinMagMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        case Filter::ComparisonMinMagPointMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::ComparisonMinPointMagLinearMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::ComparisonMinPointMagMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::ComparisonMinLinearMagMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::ComparisonMinLinearMagPointMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::ComparisonMinMagLinearMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::ComparisonMinMagMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        case Filter::ComparisonAnisotropic:
            return D3D12_FILTER_COMPARISON_ANISOTROPIC;
        case Filter::MinimumMinMagMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
        case Filter::MinimumMinMagPointMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MinimumMinPointMagLinearMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MinimumMinPointMagMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MinimumMinLinearMagMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MinimumMinLinearMagPointMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MinimumMinMagLinearMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MinimumMinMagMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
        case Filter::MinimumAnisotropic:
            return D3D12_FILTER_MINIMUM_ANISOTROPIC;
        case Filter::MaximumMinMagMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
        case Filter::MaximumMinMagPointMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MaximumMinPointMagLinearMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MaximumMinPointMagMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MaximumMinLinearMagMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MaximumMinLinearMagPointMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MaximumMinMagLinearMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MaximumMinMagMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
        case Filter::MaximumAnisotropic:
            return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
        default:
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        }
    }

    // Command Queue Type Conversions

    inline D3D12_COMMAND_LIST_TYPE to_d3d12_command_list_type(CommandQueueType type) {
        switch (type) {
        case CommandQueueType::Direct:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case CommandQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case CommandQueueType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        default:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }

} // namespace sf::render::dx12
#endif