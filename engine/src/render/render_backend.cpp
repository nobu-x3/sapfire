#include "engpch.h"
#include "render/render_backend.h"
#include "render/dx12/dx12_graphics_device.h"
#include "core/logger.h"

namespace sf::render {

RenderAPI RenderBackend::s_CurrentAPI = RenderAPI::None;
bool RenderBackend::s_Initialized = false;

void RenderBackend::initialize(RenderAPI api) {
    if (s_Initialized) {
        CORE_WARN("RenderBackend already initialized");
        return;
    }

    s_CurrentAPI = api;
    s_Initialized = true;

    CORE_INFO("Render backend initialized: {}", api == RenderAPI::DX12 ? "DirectX 12" : "Unknown");
}

RenderAPI RenderBackend::get_api() {
    return s_CurrentAPI;
}

bool RenderBackend::is_initialized() {
    return s_Initialized;
}

IGraphicsDevice* RenderBackend::create_device(const SwapchainCreationDesc& desc) {
    if (!s_Initialized) {
        CORE_ERROR("RenderBackend not initialized! Call RenderBackend::initialize() first.");
        return nullptr;
    }

    switch (s_CurrentAPI) {
        case RenderAPI::DX12:
            return new dx12::DX12GraphicsDevice(desc);

        case RenderAPI::Vulkan:
            CORE_ERROR("Vulkan backend not implemented yet");
            return nullptr;

        default:
            CORE_ERROR("Unknown render API");
            return nullptr;
    }
}

void RenderBackend::shutdown() {
    s_Initialized = false;
    s_CurrentAPI = RenderAPI::None;
}

} // namespace sf::render
