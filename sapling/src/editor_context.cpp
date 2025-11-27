#include "editor_context.h"
#include <core/logger.h>
#include <render/render_backend.h>

void EditorContext::initialize(SDL_Window* sdl_window, sf::u32 width, sf::u32 height) {
    if (m_Initialized) {
        CORE_WARN("EditorContext already initialized!");
        return;
    }
    CORE_INFO("Initializing EditorContext...");
    m_GraphicsDevice = sf::render::RenderBackend::create_device({
        .window_handle = sdl_window,
        .width = width,
        .height = height,
        .buffer_count = sf::render::MAX_FRAMES_IN_FLIGHT,
        .format = sf::render::Format::RGBA8_UNORM,
        .refresh_rate = 60,
    });
    if (!m_GraphicsDevice) {
        CORE_ERROR("Failed to create graphics device!");
        return;
    }
    m_AssetManager = sf::stl::make_unique<sf::assets::AssetManager>(
        sf::mem::MemTag::Application,
        sf::assets::AssetManagerCreationDesc{
            .device = m_GraphicsDevice.get(),
            .mesh_registry_path = sf::stl::string(sf::mem::MemTag::Mesh, "mesh_registry.db"),
            .texture_registry_path = sf::stl::string(sf::mem::MemTag::Texture, "texture_registry.db"),
        });
    m_ECManager = sf::stl::make_unique<sf::ECManager>(sf::mem::MemTag::Application);
    m_Initialized = true;
    CORE_INFO("EditorContext initialized successfully!");
}

void EditorContext::shutdown() {
    if (!m_Initialized) {
        return;
    }
    CORE_INFO("Shutting down EditorContext...");
    m_GraphicsDevice.reset();
    m_ECManager.reset();
    m_AssetManager.reset();
    m_Initialized = false;
    CORE_INFO("EditorContext shutdown complete.");
}
