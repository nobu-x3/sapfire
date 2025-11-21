#include "engpch.h"
#include "render/dx12/dx12_pipeline_state.h"
#include "render/dx12/dx12_type_conversions.h"
#include "render/dx12/dx12_util.h"
#include "render/dx12/dx12_shader_compiler.h"
#include <d3dx12_core.h>

namespace sf::render::dx12 {

void DX12PipelineState::create_graphics(ID3D12Device* device, ID3D12RootSignature* root_sig, const GraphicsPipelineStateDesc& desc) {
    m_IsCompute = false;

    // Compile shaders
    auto vs_result = sf::render::dx12::compile(
        sf::render::dx12::ShaderType::Vertex,
        desc.shader_module.vertex_shader_path,
        desc.shader_module.vertex_entry_point
    );

    auto ps_result = sf::render::dx12::compile(
        sf::render::dx12::ShaderType::Pixel,
        desc.shader_module.pixel_shader_path,
        desc.shader_module.pixel_entry_point
    );

    // Build blend state
    D3D12_BLEND_DESC blend_desc = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = desc.blend_states.size() > 1 ? TRUE : FALSE,
    };

    for (size_t i = 0; i < desc.blend_states.size() && i < 8; ++i) {
        const BlendState& blend = desc.blend_states[i];
        blend_desc.RenderTarget[i] = {
            .BlendEnable = blend.blend_enable ? TRUE : FALSE,
            .LogicOpEnable = FALSE,
            .SrcBlend = to_d3d12_blend(blend.src_blend),
            .DestBlend = to_d3d12_blend(blend.dst_blend),
            .BlendOp = to_d3d12_blend_op(blend.blend_op),
            .SrcBlendAlpha = to_d3d12_blend(blend.src_blend_alpha),
            .DestBlendAlpha = to_d3d12_blend(blend.dst_blend_alpha),
            .BlendOpAlpha = to_d3d12_blend_op(blend.blend_op_alpha),
            .LogicOp = D3D12_LOGIC_OP_NOOP,
            .RenderTargetWriteMask = static_cast<UINT8>(blend.write_mask)
        };
    }

    // Build rasterizer state
    D3D12_RASTERIZER_DESC rasterizer_desc = {
        .FillMode = to_d3d12_fill_mode(desc.rasterizer.fill_mode),
        .CullMode = to_d3d12_cull_mode(desc.rasterizer.cull_mode),
        .FrontCounterClockwise = desc.rasterizer.front_counter_clockwise ? TRUE : FALSE,
        .DepthBias = desc.rasterizer.depth_bias,
        .DepthBiasClamp = desc.rasterizer.depth_bias_clamp,
        .SlopeScaledDepthBias = desc.rasterizer.slope_scaled_depth_bias,
        .DepthClipEnable = desc.rasterizer.depth_clip_enable ? TRUE : FALSE,
        .MultisampleEnable = desc.rasterizer.multisample_enable ? TRUE : FALSE,
        .AntialiasedLineEnable = desc.rasterizer.antialiased_line_enable ? TRUE : FALSE,
        .ForcedSampleCount = 0,
        .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
    };

    // Build depth-stencil state
    D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = {
        .DepthEnable = desc.depth_stencil.depth_enable ? TRUE : FALSE,
        .DepthWriteMask = desc.depth_stencil.depth_write_enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
        .DepthFunc = to_d3d12_comparison_func(desc.depth_stencil.depth_func),
        .StencilEnable = desc.depth_stencil.stencil_enable ? TRUE : FALSE,
        .StencilReadMask = desc.depth_stencil.stencil_read_mask,
        .StencilWriteMask = desc.depth_stencil.stencil_write_mask,
        .FrontFace = {
            .StencilFailOp = to_d3d12_stencil_op(desc.depth_stencil.front_face.fail_op),
            .StencilDepthFailOp = to_d3d12_stencil_op(desc.depth_stencil.front_face.depth_fail_op),
            .StencilPassOp = to_d3d12_stencil_op(desc.depth_stencil.front_face.pass_op),
            .StencilFunc = to_d3d12_comparison_func(desc.depth_stencil.front_face.stencil_func)
        },
        .BackFace = {
            .StencilFailOp = to_d3d12_stencil_op(desc.depth_stencil.back_face.fail_op),
            .StencilDepthFailOp = to_d3d12_stencil_op(desc.depth_stencil.back_face.depth_fail_op),
            .StencilPassOp = to_d3d12_stencil_op(desc.depth_stencil.back_face.pass_op),
            .StencilFunc = to_d3d12_comparison_func(desc.depth_stencil.back_face.stencil_func)
        }
    };

    // Build PSO desc
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = root_sig,
        .VS = CD3DX12_SHADER_BYTECODE(vs_result.shader_blob->GetBufferPointer(), vs_result.shader_blob->GetBufferSize()),
        .PS = CD3DX12_SHADER_BYTECODE(ps_result.shader_blob->GetBufferPointer(), ps_result.shader_blob->GetBufferSize()),
        .BlendState = blend_desc,
        .SampleMask = UINT32_MAX,
        .RasterizerState = rasterizer_desc,
        .DepthStencilState = depth_stencil_desc,
        .InputLayout = { .pInputElementDescs = nullptr, .NumElements = 0 },
        .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        .PrimitiveTopologyType = to_d3d12_primitive_topology_type(desc.primitive_topology),
        .NumRenderTargets = desc.rtv_count,
        .DSVFormat = to_dxgi_format(desc.depth_format),
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .NodeMask = 0,
        .CachedPSO = { .pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0 },
        .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
    };

    // Set RTVformats
    for (u32 i = 0; i < desc.rtv_count && i < 8; ++i) {
        pso_desc.RTVFormats[i] = to_dxgi_format(desc.rtv_formats[i]);
    }

    dx12_check(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&m_PipelineState)));

    if (!desc.name.empty()) {
        m_PipelineState->SetName(desc.name.data());
    }
}

void DX12PipelineState::create_compute(ID3D12Device* device, ID3D12RootSignature* root_sig, const ComputePipelineStateDesc& desc) {
    m_IsCompute = true;

    // Compile compute shader
    auto cs_result = sf::render::dx12::compile(
        sf::render::dx12::ShaderType::Compute,
        desc.shader_path,
        desc.entry_point
    );

    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = root_sig,
        .CS = {
            .pShaderBytecode = cs_result.shader_blob->GetBufferPointer(),
            .BytecodeLength = cs_result.shader_blob->GetBufferSize()
        },
        .NodeMask = 0,
        .CachedPSO = { .pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0 },
        .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
    };

    dx12_check(device->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&m_PipelineState)));

    if (!desc.name.empty()) {
        m_PipelineState->SetName(desc.name.data());
    }
}

} // namespace sf::render::dx12
