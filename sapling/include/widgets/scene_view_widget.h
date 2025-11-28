#pragma once

#include "Sapfire.h"

#include <QWidget>

struct SDL_Window;
class QImage;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;

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
private:
    void initialize_rendering();
    void shutdown_rendering();
    void render();
private:
    QImage m_RenderedImage;
    SDL_Window* m_SDLWindow{nullptr};
    bool m_Initialized{false};
    bool m_NeedsResize{false};
};
