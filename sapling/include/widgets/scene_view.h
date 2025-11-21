#pragma once

#include "Sapfire.h"

#include "core/stl/shared_ptr.h"
#include "widgets/widget.h"

namespace widgets {

	class SSceneView final : public IWidget {
	public:
		explicit SSceneView(Sapfire::stl::string_view name, Sapfire::ECManager* ec_manager, Sapfire::render::IGraphicsDevice* gfx_device);
		void add_render_component(Sapfire::Entity entity, const Sapfire::RenderComponentResourcePaths& resource_paths);
		bool update(Sapfire::f32 delta_time) override;
		void render(Sapfire::d3d::GraphicsContext& gfx_ctx) override;

	private:
		void update_pass_cb(Sapfire::f32 delta_time);
		void update_materials();
		void update_transform_buffer();

	private:
		Sapfire::ECManager& m_ECManager;
		Sapfire::render::IGraphicsDevice* m_GraphicsDevice;
		Sapfire::stl::unique_ptr<Sapfire::physics::PhysicsEngine> m_PhysicsEngine;
		Sapfire::stl::vector<Sapfire::sf::render::Buffer> m_RTIndexBuffers{};
		Sapfire::stl::vector<Sapfire::sf::render::Buffer> m_VertexPosBuffers{};
		Sapfire::stl::vector<Sapfire::sf::render::Buffer> m_VertexNormalBuffers{};
		Sapfire::stl::vector<Sapfire::sf::render::Buffer> m_VertexTangentBuffers{};
		Sapfire::stl::vector<Sapfire::sf::render::Buffer> m_VertexUVBuffers{};
		Sapfire::stl::vector<Sapfire::sf::render::Buffer> m_TransformBuffers{};
		Sapfire::stl::vector<Sapfire::sf::render::Texture> m_OffscreenTextures;
		Sapfire::d3d::PipelineState m_PipelineState{};
		Sapfire::sf::render::Texture m_DepthTexture;
		Sapfire::sf::render::Buffer m_MainPassCB{};
		Sapfire::Camera m_MainCamera;
		Sapfire::stl::string m_WidgetName;
		bool m_Resizing{false};
	};
} // namespace widgets
