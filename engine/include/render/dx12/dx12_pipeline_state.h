#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include "render/i_pipeline_state.h"
#include <wrl/client.h>
#include <d3d12.h>

namespace sf::render::dx12 {

class DX12PipelineState : public IPipelineState {
public:
    DX12PipelineState() = default;
    ~DX12PipelineState() override = default;

    // Create from graphics pipeline desc
    void create_graphics(ID3D12Device* device, ID3D12RootSignature* root_sig, const GraphicsPipelineStateDesc& desc);

    // Create from compute pipeline desc
    void create_compute(ID3D12Device* device, ID3D12RootSignature* root_sig, const ComputePipelineStateDesc& desc);

    // IPipelineState implementation
    bool is_compute() const override { return m_IsCompute; }
    bool is_graphics() const override { return !m_IsCompute; }
    void* get_native_pipeline() override { return m_PipelineState.Get(); }

    // DX12-specific
    ID3D12PipelineState* get_d3d12_pso() const { return m_PipelineState.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    bool m_IsCompute = false;
};

} // namespace sf::render::dx12
#endif