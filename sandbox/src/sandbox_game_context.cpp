#include "sandbox_game_context.h"
#include "math/math.h"
#include "components/movement_component.h"
#include "components/render_component.h"
#include "components/test_custom_component.h"
#include "core/game_context.h"

using namespace sf;

SandboxGameContext::SandboxGameContext(const sf::GameContextCreationDesc& desc) : sf::GameContext(desc) {
	m_MainCamera = {CAMERA_FOV, static_cast<f32>(m_ClientExtent->width) / m_ClientExtent->height, 0.1f, 1000.f};
}

void SandboxGameContext::load_contents() {
    // TODO: refactor this to return stl::result
    auto pipeline_result = m_GraphicsDevice->create_graphics_pipeline({
		.shader_module =
			{
				.vertex_shader_path = L"bindless.hlsl",
				.vertex_entry_point = L"VS",
				.pixel_shader_path = L"bindless.hlsl",
				.pixel_entry_point = L"PS",
			},
		.name = L"Bindless Pipeline",
	});
    if(!pipeline_result) {
        CLIENT_CRITICAL("Failed to create bindless pipeline.");
        return;
    }
	m_PipelineState = std::move(*pipeline_result);
    auto cbv_result = m_GraphicsDevice->create_buffer(sf::render::BufferCreationDesc{
		.usage = sf::render::BufferUsage::Constant,
		.size_in_bytes = sizeof(PassConstants),
		.name = L"Main Pass Constant Buffer",
	});
    if(!cbv_result) {
        CLIENT_CRITICAL("Failed to create main pass constant buffer.");
        return;
    }
	m_MainPassCB = std::move(*cbv_result);
	// textures:
    auto depth_result = m_GraphicsDevice->create_texture({
		.usage = sf::render::TextureUsage::DepthStencil,
		.format = sf::render::Format::D32_FLOAT,
		.width = static_cast<u32>(m_ClientExtent->width),
		.height = static_cast<u32>(m_ClientExtent->height),
		.name = L"Depth Texture",
	});
    if(!depth_result) {
        CLIENT_CRITICAL("Failed to create depth texture.");
        return;
    }
	m_DepthTexture = std::move(*depth_result);
	assets::SceneWriter writer{&m_ECManager, m_AssetManager.get()};
	writer.deserealize("test_scene.scene", [&](sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths) {
		create_render_component(entity, resource_paths);
	});
}

void SandboxGameContext::update(f32 delta_time) {
	GameContext::update(delta_time);
	PROFILE_SCOPE("SandboxGameContext::update");
	m_MainCamera.update(delta_time);
	// Wait for render to happen
	m_GraphicsDevice->get_direct_queue()->wait_for_idle();
	// update buffers
	update_pass_cb(delta_time);
	update_materials(delta_time);
	udpate_transform_buffer(delta_time);
}

void SandboxGameContext::update_pass_cb(f32 delta_time) {
	m_PassConstants = PassConstants{};
	sf::math::mat4 view = m_MainCamera.view();
	sf::math::mat4 proj = m_MainCamera.projection;
	sf::math::mat4 viewProj = view * proj;
	sf::math::mat4 invView = view.inversed();
	sf::math::mat4 invProj = proj.inversed();
	sf::math::mat4 invViewProj = viewProj.inversed();
	m_PassConstants.view = view.transposed();
	m_PassConstants.inv_view = invView.transposed();
	m_PassConstants.proj = proj.transposed();
	m_PassConstants.inv_proj = invProj.transposed();
	m_PassConstants.view_proj = viewProj.transposed();
	m_PassConstants.inv_view_proj = invViewProj.transposed();
	sf::math::vec4 eye_pos = m_MainCamera.transform.position();
	m_PassConstants.EyePosW = sf::math::vec3(eye_pos.x, eye_pos.y, eye_pos.z);
	m_PassConstants.render_target_size = sf::math::vec2((float)m_ClientExtent->width, (float)m_ClientExtent->height);
	m_PassConstants.inv_render_target_size = sf::math::vec2(1.0f / m_ClientExtent->width, 1.0f / m_ClientExtent->height);
	m_PassConstants.near_z = 1.0f;
	m_PassConstants.far_z = 1000.0f;
	m_PassConstants.total_time = delta_time;
	m_PassConstants.delta_time = delta_time;
	m_PassConstants.ambient_light = {0.25f, 0.25f, 0.35f, 1.0f};
	m_PassConstants.Lights[0].direction = {0.57735f, -0.0f, 1.57735f};
	m_PassConstants.Lights[0].strength = {0.6f, 0.6f, 0.6f};
	m_PassConstants.Lights[1].direction = {-0.57735f, -0.57735f, 0.57735f};
	m_PassConstants.Lights[1].strength = {0.3f, 0.3f, 0.3f};
	m_PassConstants.Lights[2].direction = {0.0f, -0.707f, -0.707f};
	m_PassConstants.Lights[2].strength = {0.15f, 0.15f, 0.15f};
	m_MainPassCB.update(&m_PassConstants, sizeof(PassConstants));
}

void SandboxGameContext::update_materials(f32 delta_time) {
	auto index = 0;
	for (auto&& [path, asset] : m_AssetManager->path_material_map()) {
		sf::render::MaterialConstants data{
			.diffuse_albedo = asset.material.diffuse_albedo,
			.fresnel_r0 = asset.material.fresnel_r0,
			.roughness = asset.material.roughness,
		};
		asset.material.material_buffer.update(&data, sizeof(sf::render::MaterialConstants));
		asset.material.material_cb_index = index;
		index++;
	}
}

void SandboxGameContext::udpate_transform_buffer(f32 delta_time) {
	auto& transforms = m_ECManager.engine_components<components::Transform>();
	for (u32 i = 0; i < m_TransformBuffers.size(); ++i) {
		sf::math::mat4 world = transforms[i].transform();
		ObjectConstants obj_constants;
		obj_constants.World = world.transposed();
		m_TransformBuffers[i].update(&obj_constants, sizeof(ObjectConstants));
	}
}

void SandboxGameContext::render() {
	PROFILE_FUNCTION();
	m_GraphicsDevice->begin_frame();
	auto& gfx_ctx = m_GraphicsDevice->get_current_graphics_context();
	auto& current_backbuffer = m_GraphicsDevice->get_current_back_buffer();
	gfx_ctx.transition_barrier(current_backbuffer, sf::render::ResourceState::Present, sf::render::ResourceState::RenderTarget);
	gfx_ctx.execute_resource_barriers();
	static stl::array<f32, 4> clear_color{0.3f, 0.4f, 0.6f, 1.0f};
	gfx_ctx.clear_render_target_view(current_backbuffer, clear_color);
	gfx_ctx.clear_depth_stencil_view(m_DepthTexture);
	// TODO: setup barriers for all passes
	gfx_ctx.set_pipeline_state(m_PipelineState);
	gfx_ctx.set_root_signature();
	gfx_ctx.set_render_target(current_backbuffer, &m_DepthTexture);
	gfx_ctx.set_viewport({
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<f32>(m_ClientExtent->width),
		.height = static_cast<f32>(m_ClientExtent->height),
		.min_depth = 0.0f,
		.max_depth = 1.0f,
	});
	// TODO: rendering
	{
		gfx_ctx.set_descriptor_heaps();
		gfx_ctx.set_primitive_topology(sf::render::PrimitiveTopology::TriangleList);
		sf::math::frustum camera_frustum = sf::math::frustum::create_from_matrix(m_MainCamera.projection);
		sf::math::mat4 view = m_MainCamera.view();
		sf::math::mat4 inv_view = view.inversed();
		auto& render_components = m_ECManager.engine_components<components::RenderComponent>();
		for (auto& comp : render_components) {
			components::Transform transform;
			if (!m_ECManager.get_other_engine_component<components::RenderComponent, components::Transform>(comp, transform))
				continue;
			const auto* mesh_asset = m_AssetManager->get_mesh(comp.mesh_uuid());
			sf::math::aabb aabb = mesh_asset->data->aabb;
			sf::math::mat4 world = transform.transform();
			sf::math::mat4 inv_world = world.inversed();
			// View space to object local space
			sf::math::mat4 view_to_local = inv_view * inv_world;
			// Transform camera frustum from view space to object's local space
			sf::math::frustum local_space_frustum = camera_frustum.transform(view_to_local);
			if (local_space_frustum.contains(aabb) != sf::math::ContainmentType::Disjoint) {
				components::CPUData cpu_data = comp.cpu_data();
				gfx_ctx.set_index_buffer(m_RTIndexBuffers[cpu_data.index_id]);
				auto* per_draw = comp.per_draw_constants();
				gfx_ctx.set_32_bit_constants(per_draw, sizeof(components::PerDrawConstants) / sizeof(u32));
				gfx_ctx.draw_indexed_instanced(cpu_data.indices_size, 1);
			}
		}
	}
	gfx_ctx.transition_barrier(current_backbuffer, sf::render::ResourceState::RenderTarget, sf::render::ResourceState::Present);
	gfx_ctx.execute_resource_barriers();
	gfx_ctx.close();
	m_GraphicsDevice->get_direct_queue()->execute_command_list(&gfx_ctx);
	m_GraphicsDevice->present();
	m_GraphicsDevice->end_frame();
}

void SandboxGameContext::resize_depth_texture() {
	m_MainCamera = {CAMERA_FOV, static_cast<f32>(m_ClientExtent->width) / m_ClientExtent->height, 0.1f, 1000.f};
	// Release old texture by reassigning
    // TODO: make this return stl::result
    auto depth_result = m_GraphicsDevice->create_texture({
		.usage = sf::render::TextureUsage::DepthStencil,
		.format = sf::render::Format::D32_FLOAT,
		.width = static_cast<u32>(m_ClientExtent->width),
		.height = static_cast<u32>(m_ClientExtent->height),
		.name = L"Depth Texture",
	});
    if(!depth_result) {
        CLIENT_CRITICAL("Failed to create depth texture while resizing.");
        return;
    }
	m_DepthTexture = std::move(*depth_result);
}
