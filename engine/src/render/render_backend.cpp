#include "engpch.h"
#include "render/render_backend.h"
#include "core/logger.h"

#ifdef SF_PLATFORM_WINDOWS
#include "render/dx12/dx12_graphics_device.h"
#endif

#include "render/vulkan/vk_graphics_device.h"

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

    const char* api_name = "Unknown";
    if (api == RenderAPI::DX12) api_name = "DirectX 12";
    else if (api == RenderAPI::Vulkan) api_name = "Vulkan";
    CORE_INFO("Render backend initialized: {}", api_name);
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
#ifdef SF_PLATFORM_WINDOWS
            return new dx12::DX12GraphicsDevice(desc);
#else
            CORE_ERROR("DX12 is only available on Windows");
            return nullptr;
#endif

        case RenderAPI::Vulkan:
            return new vk::VkGraphicsDevice(desc);

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
