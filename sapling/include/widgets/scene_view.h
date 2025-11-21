#pragma once

#include "Sapfire.h"

#include "core/stl/shared_ptr.h"
#include "widgets/widget.h"

namespace widgets {

	class SSceneView final : public IWidget {
	public:
		explicit SSceneView(sf::stl::string_view name, sf::ECManager* ec_manager, sf::render::IGraphicsDevice* gfx_device);
		void add_render_component(sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths);
		bool update(sf::f32 delta_time) override;
		void render(sf::d3d::GraphicsContext& gfx_ctx) override;

	private:
		void update_pass_cb(sf::f32 delta_time);
		void update_materials();
		void update_transform_buffer();

	private:
		sf::ECManager& m_ECManager;
		sf::render::IGraphicsDevice* m_GraphicsDevice;
		sf::stl::unique_ptr<sf::physics::PhysicsEngine> m_PhysicsEngine;
		sf::stl::vector<sf::render::Buffer> m_RTIndexBuffers{};
		sf::stl::vector<sf::render::Buffer> m_VertexPosBuffers{};
		sf::stl::vector<sf::render::Buffer> m_VertexNormalBuffers{};
		sf::stl::vector<sf::render::Buffer> m_VertexTangentBuffers{};
		sf::stl::vector<sf::render::Buffer> m_VertexUVBuffers{};
		sf::stl::vector<sf::render::Buffer> m_TransformBuffers{};
		sf::stl::vector<sf::render::Texture> m_OffscreenTextures;
		sf::d3d::PipelineState m_PipelineState{};
		sf::render::Texture m_DepthTexture;
		sf::render::Buffer m_MainPassCB{};
		sf::Camera m_MainCamera;
		sf::stl::string m_WidgetName;
		bool m_Resizing{false};
	};
} // namespace widgets
