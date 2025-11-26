#pragma once

#include <QWidget>
#include "Sapfire.h"

// SceneViewWidget: Central 3D viewport that embeds SDL3 window for rendering
class SceneViewWidget : public QWidget {
    Q_OBJECT

public:
    explicit SceneViewWidget(QWidget* parent = nullptr);
    ~SceneViewWidget() override;

    void update_frame(sf::f32 delta_time);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void initialize_rendering();
    void shutdown_rendering();
    void render();

private:
    bool m_Initialized{false};
    bool m_NeedsResize{false};
};
