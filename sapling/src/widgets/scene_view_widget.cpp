#include "widgets/scene_view_widget.h"
#include "editor_context.h"
#include "render/render_api.h"

#include <core/logger.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include <SDL3/SDL.h>

SceneViewWidget::SceneViewWidget(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(320, 240);
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
    void* native_handle = reinterpret_cast<void*>(winId());
    auto& ctx = EditorContext::instance();
    ctx.initialize(native_handle, width(), height());
    m_Initialized = true;
    CORE_INFO("SceneViewWidget initialized successfully!");
}

void SceneViewWidget::shutdown_rendering() {
    if (!m_Initialized) {
        return;
    }
    CORE_INFO("Shutting down SceneViewWidget...");
    EditorContext::instance().shutdown();
    m_Initialized = false;
}

void SceneViewWidget::update_frame(sf::f32 delta_time) {
    if (!m_Initialized) {
        return;
    }
    if (m_NeedsResize) {
        auto* device = EditorContext::instance().graphics_device();
        if (device) {
            device->resize_window(width(), height());
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
    device->begin_frame();
    auto& back_buffer = device->get_current_back_buffer();
    auto& ctx = device->get_current_graphics_context();
    ctx.transition_barrier(back_buffer, sf::render::ResourceState::Present, sf::render::ResourceState::RenderTarget);
    sf::f32 clear_color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    ctx.clear_render_target_view(back_buffer, sf::stl::span<sf::f32, 4>(clear_color));
    // TODO: Render scene entities here
    // auto* ec_manager = EditorContext::instance().ec_manager();
    ctx.transition_barrier(back_buffer, sf::render::ResourceState::RenderTarget, sf::render::ResourceState::Present);
    device->end_frame();
    device->present();
}

void SceneViewWidget::paintEvent(QPaintEvent* event) {
    // Don't call base class - we're handling rendering ourselves
    // This prevents Qt from clearing the widget
}

void SceneViewWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_Initialized) {
        m_NeedsResize = true;
    }
}

void SceneViewWidget::mousePressEvent(QMouseEvent* event) {
    // TODO: Forward to camera controller or gizmo system
    setFocus();
}

void SceneViewWidget::mouseReleaseEvent(QMouseEvent* event) {
    // TODO: Forward to camera controller or gizmo system
}

void SceneViewWidget::mouseMoveEvent(QMouseEvent* event) {
    // TODO: Forward to camera controller or gizmo system
}

void SceneViewWidget::wheelEvent(QWheelEvent* event) {
    // TODO: Forward to camera controller (zoom)
}

void SceneViewWidget::keyPressEvent(QKeyEvent* event) {
    // TODO: Forward to camera controller (WASD movement, etc.)
}

void SceneViewWidget::keyReleaseEvent(QKeyEvent* event) {
    // TODO: Forward to camera controller
}
