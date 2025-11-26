#include "widgets/scene_view_widget.h"
#include "editor_context.h"

#include <core/logger.h>
#include <stl/result.h>
#include <SDL3/SDL.h>
#include <QVBoxLayout>
#include <QWindow>
#include <QResizeEvent>

SceneViewWidget::SceneViewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 240);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

SceneViewWidget::~SceneViewWidget() {
    shutdown_rendering();
}

void SceneViewWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!m_Initialized) {
        initialize_rendering();
    }
}

void SceneViewWidget::initialize_rendering() {
    if (m_Initialized) {
        return;
    }
    CORE_INFO("Initializing SceneViewWidget rendering...");
    m_SDLWindow = SDL_CreateWindow("Sapling Viewport", width(), height(), SDL_WINDOW_VULKAN);
    if (!m_SDLWindow) {
        CORE_ERROR("Failed to create SDL window: {}", SDL_GetError());
        return;
    }
    auto& ctx = EditorContext::instance();
    ctx.initialize(m_SDLWindow, width(), height());
    m_Initialized = true;
    CORE_INFO("SceneViewWidget initialized successfully!");
}

void SceneViewWidget::shutdown_rendering() {
    if (!m_Initialized) {
        return;
    }
    CORE_INFO("Shutting down SceneViewWidget...");
    EditorContext::instance().shutdown();
    if (m_SDLWindow) {
        SDL_DestroyWindow(m_SDLWindow);
        m_SDLWindow = nullptr;
    }
    m_Initialized = false;
}

void SceneViewWidget::update_frame(sf::f32 delta_time) {
    if (!m_Initialized) {
        return;
    }
    if (m_NeedsResize) {
        auto* device = EditorContext::instance().graphics_device();
        if (device) {
            auto resize_result = device->resize_window(width(), height());
            if(!resize_result) {
                CLIENT_ERROR("Failed to resize scene view: {}", resize_result.error().c_str());
                return;
            }
        }
        m_NeedsResize = false;
    }
    render();
}

void SceneViewWidget::render() {
    if (!m_Initialized) {
        return;
    }
    auto* device = EditorContext::instance().graphics_device();
    if (!device) {
        return;
    }
    auto result = device->begin_frame();
    if (!result.has_value()) {
        CORE_CRITICAL("Failed to begin frame: {}", result.error().c_str());
        return;
    }
    auto& back_buffer = device->get_current_back_buffer();
    auto& ctx = device->get_current_graphics_context();
    ctx.set_render_target(back_buffer);
    // TODO: render commands go here
    device->end_frame();
    device->present();
}

void SceneViewWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_Initialized && m_SDLWindow) {
        SDL_SetWindowSize(m_SDLWindow, event->size().width(), event->size().height());
        m_NeedsResize = true;
    }
}
