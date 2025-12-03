#pragma once

#include "Sapfire.h"

#include <QWidget>

struct SDL_Window;
class QImage;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;

namespace sf::render {
    class IPipelineState;
}

struct PassConstants {
    sf::math::mat4 view = sf::math::mat4::identity();
    sf::math::mat4 inv_view = sf::math::mat4::identity();
    sf::math::mat4 proj = sf::math::mat4::identity();
    sf::math::mat4 inv_proj = sf::math::mat4::identity();
    sf::math::mat4 view_proj = sf::math::mat4::identity();
    sf::math::mat4 inv_view_proj = sf::math::mat4::identity();
    sf::math::vec3 EyePosW = {0.0f, 0.0f, 0.0f};
    sf::f32 cbPerObjectPad1 = 0.0f;
    sf::math::vec2 render_target_size = {0.0f, 0.0f};
    sf::math::vec2 inv_render_target_size = {0.0f, 0.0f};
    sf::f32 near_z = 0.0f;
    sf::f32 far_z = 0.0f;
    sf::f32 total_time = 0.0f;
    sf::f32 delta_time = 0.0f;
    sf::math::vec4 ambient_light = {0.0f, 0.0f, 0.0f, 1.0f};
    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of sf::render::MAX_LIGHTS per object.
    sf::render::Light Lights[sf::render::MAX_LIGHTS];
};

class SceneViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit SceneViewWidget(QWidget* parent = nullptr);
    ~SceneViewWidget() override;
    void update_frame(sf::f32 delta_time);

public slots:
    void on_render_component_added(sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initialize_rendering();
    sf::stl::result<> load_contents();
    void shutdown_rendering();
    void render();

private:
    sf::Camera m_MainCamera{};
    sf::render::IPipelineState* m_PipelineState{nullptr};
    sf::render::Buffer m_MainPassCB{};
    PassConstants m_PassConstants{};
    sf::render::Texture m_DepthTexture{};
    sf::stl::vector<sf::render::Buffer> m_RTIndexBuffers{sf::mem::MemTag::Render};
    sf::stl::vector<sf::render::Buffer> m_VertexPosBuffers{sf::mem::MemTag::Render};
    sf::stl::vector<sf::render::Buffer> m_VertexNormalBuffers{sf::mem::MemTag::Render};
    sf::stl::vector<sf::render::Buffer> m_VertexTangentBuffers{sf::mem::MemTag::Render};
    sf::stl::vector<sf::render::Buffer> m_VertexUVBuffers{sf::mem::MemTag::Render};
    sf::stl::vector<sf::render::Buffer> m_TransformBuffers{sf::mem::MemTag::Render};
    SDL_Window* m_SDLWindow{nullptr};
    QImage m_RenderedImage;
    bool m_Initialized{false};
    bool m_NeedsResize{false};
};
