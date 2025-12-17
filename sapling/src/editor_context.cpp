#include "editor_context.h"
#include <core/logger.h>
#include <render/render_backend.h>
#include "project_manager.h"
#include "render/bindless_resource_registry.h"
#include "render/i_pipeline_layout.h"

#include <QApplication>
#include <QPalette>
#include <QStyleHints>

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
    // Get GPU descriptor limits and create descriptor pool for bindless resources
    const auto& limits = m_GraphicsDevice->get_descriptor_limits();
    auto pool_result = m_GraphicsDevice->create_descriptor_pool({
        .max_sets = 10,
        .max_uniform_buffers = limits.max_uniform_buffers,
        .max_storage_buffers = limits.max_storage_buffers,
        .max_sampled_images = limits.max_sampled_images,
        .max_storage_images = limits.max_storage_images,
        .max_samplers = limits.max_samplers,
        .name = "Editor Descriptor Pool",
    });
    if (!pool_result) {
        CLIENT_ERROR("Failed to create descriptor pool: {}", pool_result.error().c_str());
        return;
    }
    m_DescriptorPool = std::move(*pool_result);
    m_BindlessLayouts = sf::render::BindlessResourceRegistry::default_descriptor_set_layout(limits);
    // Create bindless resource registry
    m_BindlessRegistry =
        sf::stl::make_unique<sf::render::BindlessResourceRegistry>(sf::mem::MemTag::Render, m_DescriptorPool.get(), m_BindlessLayouts);
    auto queue_result = m_GraphicsDevice->create_direct_queue("Editor Direct Queue");
    if (!queue_result) {
        CLIENT_ERROR("Failed to create direct queue: {}", queue_result.error().c_str());
        return;
    }
    m_DirectQueue = std::move(*queue_result);
    auto context_result = m_GraphicsDevice->create_graphics_context();
    if (!context_result) {
        CLIENT_ERROR("Failed to create graphics context: {}", context_result.error().c_str());
        return;
    }
    m_GraphicsContext = std::move(*context_result);
    auto allocator_result = m_GraphicsDevice->create_memory_allocator();
    if (!allocator_result) {
        CLIENT_ERROR("Failed to create memory allocator: {}", allocator_result.error().c_str());
        return;
    }
    m_MemoryAllocator = std::move(*allocator_result);
    m_AssetManager = sf::stl::make_unique<sf::assets::AssetManager>(
        sf::mem::MemTag::Application,
        sf::assets::AssetManagerCreationDesc{
            .device = m_GraphicsDevice.get(),
            .memory_allocator = m_MemoryAllocator.get(),
            .bindless_registry = m_BindlessRegistry.get(),
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
    m_MemoryAllocator.reset();
    m_GraphicsContext.reset();
    m_DirectQueue.reset();
    m_BindlessRegistry.reset();
    m_DescriptorPool.reset();
    m_GraphicsDevice.reset();
    m_ECManager.reset();
    m_AssetManager.reset();
    m_Initialized = false;
    CLIENT_INFO("EditorContext shutdown complete.");
}

ProjectManager* EditorContext::project_manager() { return &ProjectManager::instance(); }
