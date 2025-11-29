#include "widgets/scene_view_widget.h"
#include "editor_context.h"

#include <QImage>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWindow>
#include <SDL3/SDL.h>
#include <core/logger.h>
#include <stl/result.h>

SceneViewWidget::SceneViewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 240);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

SceneViewWidget::~SceneViewWidget() { shutdown_rendering(); }

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
    // Note: Cannot use SDL_WINDOW_HIDDEN on Wayland - the compositor blocks vkQueuePresentKHR
    // on hidden surfaces, causing the application to hang. The window must be visible for
    // presentation to work, even if we're copying the framebuffer to a QImage later.
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
            if (!resize_result) {
                CLIENT_ERROR("Failed to resize scene view: {}", resize_result.error().c_str());
                return;
            }
        }
        m_NeedsResize = false;
    }
    render();
}

void SceneViewWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    if (m_RenderedImage.isNull())
        return;
    painter.drawImage(rect(), m_RenderedImage);
}

void SceneViewWidget::render() {
    CLIENT_TRACE("Render");
    if (!m_Initialized) {
        return;
    }
    auto* device = EditorContext::instance().graphics_device();
    if (!device) {
        return;
    }
    CLIENT_TRACE("PRE BEGIN");
    auto result = device->begin_frame();
    if (!result.has_value()) {
        CLIENT_CRITICAL("Failed to begin frame: {}", result.error().c_str());
        return;
    }
    CLIENT_TRACE("BEGIN");
    auto& back_buffer = device->get_current_back_buffer();
    auto& ctx = device->get_current_graphics_context();
    ctx.set_render_target(back_buffer);
    // TODO: render commands go here
    device->end_frame();
    device->present();
    CLIENT_TRACE("PRESENT");
    // sf::u32 pixel_count = back_buffer.width * back_buffer.height;
    // size_t buffer_size = pixel_count * 4;
    // if(m_RenderedImage.width() != static_cast<int>(back_buffer.width) || m_RenderedImage.height() != static_cast<int>(back_buffer.height)) {
    //     m_RenderedImage = QImage(back_buffer.width, back_buffer.height, QImage::Format_RGBA8888);
    // }
    // auto read_result = device->read_texture_pixels(back_buffer, m_RenderedImage.bits(), buffer_size);
    // if(!read_result) {
    //     CLIENT_CRITICAL("Failed to read texture pixels: {}", read_result.error().c_str());
    //     return;
    // }
    // update();
}

void SceneViewWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_Initialized && m_SDLWindow) {
        SDL_SetWindowSize(m_SDLWindow, event->size().width(), event->size().height());
        m_NeedsResize = true;
    }
}
