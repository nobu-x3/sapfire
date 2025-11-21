#pragma once

#include "Sapfire.h"
#include "core/game_context.h"

struct PassConstants {
	sf::math::mat4 view = sf::math::mat4::identity();
	sf::math::mat4 inv_view = sf::math::mat4::identity();
	sf::math::mat4 proj = sf::math::mat4::identity();
	sf::math::mat4 inv_proj = sf::math::mat4::identity();
	sf::math::mat4 view_proj = sf::math::mat4::identity();
	sf::math::mat4 inv_view_proj = sf::math::mat4::identity();
	sf::math::vec3 EyePosW = {0.0f, 0.0f, 0.0f};
	float cbPerObjectPad1 = 0.0f;
	sf::math::vec2 render_target_size = {0.0f, 0.0f};
	sf::math::vec2 inv_render_target_size = {0.0f, 0.0f};
	float near_z = 0.0f;
	float far_z = 0.0f;
	float total_time = 0.0f;
	float delta_time = 0.0f;
	sf::math::vec4 ambient_light = {0.0f, 0.0f, 0.0f, 1.0f};
	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of sf::render::MAX_LIGHTS per object.
	sf::render::Light Lights[sf::render::MAX_LIGHTS];
};

class SandboxGameContext final : public sf::GameContext {
public:
	SandboxGameContext(const sf::GameContextCreationDesc& desc);
	void load_contents() override;
	void render() override;
	void update(sf::f32 delta_time) override;
	void resize_depth_texture();

private:
	void update_pass_cb(sf::f32 delta_time);
	void update_materials(sf::f32 delta_time);
	void udpate_transform_buffer(sf::f32 delta_time);

private:
	sf::render::Texture m_DepthTexture{};
	sf::render::IPipelineState* m_PipelineState = nullptr;
    PassConstants m_PassConstants{};
};
