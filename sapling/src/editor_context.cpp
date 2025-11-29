#include "editor_context.h"
#include "project_manager.h"
#include <core/logger.h>
#include <render/render_backend.h>

#include <QStyleHints>
#include <QPalette>
#include <QApplication>

void EditorContext::set_theme(QApplication& app) {
    app.setStyle("Fusion");
    QColor sapphire(15, 82, 186);
    QColor sapphire_light(30, 120, 220);
    QPalette dark_palette;
    // Window and base backgrounds - pure black
    dark_palette.setColor(QPalette::Window, QColor(0, 0, 0));
    dark_palette.setColor(QPalette::Base, QColor(10, 10, 10)); // Slightly lighter for input fields
    dark_palette.setColor(QPalette::AlternateBase, QColor(20, 20, 20));
    // All text colors - white
    dark_palette.setColor(QPalette::WindowText, Qt::white);
    dark_palette.setColor(QPalette::Text, Qt::white);
    dark_palette.setColor(QPalette::ButtonText, Qt::white);
    dark_palette.setColor(QPalette::BrightText, Qt::white);
    // Placeholder text (for search boxes, etc.) - light gray so it's readable
    dark_palette.setColor(QPalette::PlaceholderText, QColor(160, 160, 160));
    // Buttons - dark gray
    dark_palette.setColor(QPalette::Button, QColor(30, 30, 30));
    // Tooltips - dark background with white text
    dark_palette.setColor(QPalette::ToolTipBase, QColor(20, 20, 20));
    dark_palette.setColor(QPalette::ToolTipText, Qt::white);
    // Links and highlights - sapphire blue
    dark_palette.setColor(QPalette::Link, sapphire_light);
    dark_palette.setColor(QPalette::LinkVisited, sapphire);
    dark_palette.setColor(QPalette::Highlight, sapphire);
    dark_palette.setColor(QPalette::HighlightedText, Qt::white);
    // Disabled text - gray
    dark_palette.setColor(QPalette::Disabled, QPalette::Text, QColor(100, 100, 100));
    dark_palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(100, 100, 100));
    dark_palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(100, 100, 100));
    app.setPalette(dark_palette);
    CLIENT_INFO("Sapling theme set to dark mode!");
}

void EditorContext::initialize(SDL_Window* sdl_window, sf::u32 width, sf::u32 height) {
    if (m_Initialized) {
        CLIENT_WARN("EditorContext already initialized!");
        return;
    }
    CLIENT_INFO("Initializing EditorContext...");
    m_GraphicsDevice = sf::render::RenderBackend::create_device({
        .window_handle = sdl_window,
        .width = width,
        .height = height,
        .buffer_count = sf::render::MAX_FRAMES_IN_FLIGHT,
        .format = sf::render::Format::RGBA8_UNORM,
        .refresh_rate = 60,
        .headless = true, // Enable headless mode for editor viewport (offscreen rendering)
    });
    if (!m_GraphicsDevice) {
        CLIENT_ERROR("Failed to create graphics device!");
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
    CLIENT_INFO("EditorContext initialized successfully!");
}

void EditorContext::shutdown() {
    if (!m_Initialized) {
        return;
    }
    CLIENT_INFO("Shutting down EditorContext...");
    m_GraphicsDevice.reset();
    m_ECManager.reset();
    m_AssetManager.reset();
    m_Initialized = false;
    CLIENT_INFO("EditorContext shutdown complete.");
}

ProjectManager* EditorContext::project_manager() {
    return &ProjectManager::instance();
}
