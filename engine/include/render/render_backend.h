#pragma once

#include "core/core.h"
#include "render_api.h"
#include "resource_types.h"
#include "i_graphics_device.h"

namespace sf::render {

// ============================================================================
// Render Backend Factory
// ============================================================================

class RenderBackend {
public:
    // Initialize the rendering backend (must be called before creating devices)
    static void initialize(RenderAPI api);

    // Get current API
    static RenderAPI get_api();

    // Check if backend is initialized
    static bool is_initialized();

    // Create graphics device
    static IGraphicsDevice* create_device(const SwapchainCreationDesc& desc);

    // Shutdown backend
    static void shutdown();

private:
    static RenderAPI s_CurrentAPI;
    static bool s_Initialized;
};

} // namespace sf::render
