#include "engpch.h"
#include "render/render_compat.h"
#include "render/render_backend.h"
#include "core/logger.h"

namespace Sapfire::d3d {

GraphicsDevice::GraphicsDevice(const SwapchainCreationDesc& desc) {
    // Initialize render backend if not already done
    if (!sf::render::RenderBackend::is_initialized()) {
        // Default to DX12 for backward compatibility
        sf::render::RenderBackend::initialize(sf::render::RenderAPI::DX12);
    }

    // Create device through factory
    m_Device = sf::render::RenderBackend::create_device(desc);

    if (!m_Device) {
        CORE_ERROR("Failed to create graphics device!");
    }
}

} // namespace Sapfire::d3d
