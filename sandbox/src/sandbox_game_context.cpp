#include "sandbox_game_context.h"
#include "components/render_component.h"
#include "core/game_context.h"
#include "core/logger.h"
#include "render/bindless_resource_registry.h"

using namespace sf;

SandboxGameContext::SandboxGameContext(const sf::GameContextCreationDesc& desc) : sf::GameContext(desc) {
    m_MainCamera = {CAMERA_FOV, static_cast<f32>(m_ClientExtent->width) / m_ClientExtent->height, 0.1f, 1000.f};

    // Create synchronization objects for stateless rendering API
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto fence_result = m_GraphicsDevice->create_fence(true, "Frame Fence");
        if (!fence_result) {
            CLIENT_CRITICAL("Failed to create fence {}: {}", i, fence_result.error().c_str());
            continue;
        }
        m_InFlightFences[i] = std::move(*fence_result);

        auto img_sem_result = m_GraphicsDevice->create_semaphore("Image Available Semaphore");
        if (!img_sem_result) {
            CLIENT_CRITICAL("Failed to create image available semaphore {}: {}", i, img_sem_result.error().c_str());
            continue;
        }
        m_ImageAvailableSemaphores[i] = std::move(*img_sem_result);

        auto render_sem_result = m_GraphicsDevice->create_semaphore("Render Finished Semaphore");
        if (!render_sem_result) {
            CLIENT_CRITICAL("Failed to create render finished semaphore {}: {}", i, render_sem_result.error().c_str());
            continue;
        }
        m_RenderFinishedSemaphores[i] = std::move(*render_sem_result);
    }
}

void SandboxGameContext::load_contents() {
    // Create render pass
    render::RenderPassDesc::AttachmentDesc depth_attachment{
        .format = render::Format::D32_FLOAT,
        .load_op = render::LoadOp::Clear,
        .store_op = render::StoreOp::DontCare,
        .initial_layout = render::ResourceState::Undefined,
        .final_layout = render::ResourceState::DepthWrite,
    };
    auto render_pass_result = m_GraphicsDevice->create_render_pass({
        .color_attachments = {mem::MemTag::Temp, 1, {
            .format = m_GraphicsDevice->get_back_buffer(0).format,
            .load_op = render::LoadOp::Clear,
            .store_op = render::StoreOp::Store,
            .initial_layout = render::ResourceState::Undefined,
            .final_layout = render::ResourceState::Present,
        }},
        .depth_attachment = depth_attachment,
        .name = "Main Render Pass",
    });
    if (!render_pass_result) {
        CLIENT_CRITICAL("Failed to create render pass: {}", render_pass_result.error().c_str());
        return;
    }
    m_RenderPass = std::move(*render_pass_result);

    // TODO: refactor this to return stl::result
    auto pipeline_result = m_GraphicsDevice->create_graphics_pipeline({
        .layout = nullptr,  // Using bindless layout
        .render_pass = m_RenderPass.get(),
        .vertex_shader =
            {
                .path = "bindless.hlsl",
                .entry_point = "VS",
            },
        .pixel_shader =
            {
                .path = "bindless.hlsl",
                .entry_point = "PS",
            },
    });
    if (!pipeline_result) {
        CLIENT_CRITICAL("Failed to create bindless pipeline.");
        return;
    }
    m_PipelineState = std::move(pipeline_result.value());

    auto cbv_result = m_MemoryAllocator->allocate_buffer(sf::render::BufferCreationDesc{
        .usage = sf::render::BufferUsage::Constant,
        .size_in_bytes = sizeof(PassConstants),
        .name = "Main Pass Constant Buffer",
    });
    if (!cbv_result) {
        CLIENT_CRITICAL("Failed to create main pass constant buffer: {}", cbv_result.error().c_str());
        return;
    }
    cbv_result->cbv_index = m_BindlessRegistry->register_buffer(*cbv_result);
    m_MainPassCB = std::move(*cbv_result);

    auto depth_result = m_MemoryAllocator->allocate_texture({
        .usage = sf::render::TextureUsage::DepthStencil,
        .format = sf::render::Format::D32_FLOAT,
        .width = static_cast<u32>(m_ClientExtent->width),
        .height = static_cast<u32>(m_ClientExtent->height),
        .name = "Depth Texture",
    });
    if (!depth_result) {
        CLIENT_CRITICAL("Failed to create depth texture: {}", depth_result.error().c_str());
        return;
    }
    m_DepthTexture = std::move(*depth_result);

    // Create framebuffers for each swapchain image
    for (u32 i = 0; i < m_GraphicsDevice->get_back_buffer_count(); ++i) {
        stl::array<render::Texture*, 1> color_attachments{&m_GraphicsDevice->get_back_buffer(i)};
        auto framebuffer_result = m_GraphicsDevice->create_framebuffer({
            .render_pass = m_RenderPass.get(),
            .color_attachments = color_attachments,
            .depth_attachment = &m_DepthTexture,
            .width = static_cast<u32>(m_ClientExtent->width),
            .height = static_cast<u32>(m_ClientExtent->height),
            .name = "Main Framebuffer",
        });
        if (!framebuffer_result) {
            CLIENT_CRITICAL("Failed to create framebuffer {}: {}", i, framebuffer_result.error().c_str());
            return;
        }
        m_Framebuffers[i] = std::move(*framebuffer_result);
    }

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
    m_DirectQueue->wait_for_idle();
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

    auto* fence = m_InFlightFences[m_CurrentFrame].get();
    auto wait_result = fence->wait(UINT64_MAX);
    if (!wait_result) {
        CLIENT_ERROR("Failed to wait on fence: {}", wait_result.error().c_str());
        return;
    }
    auto reset_result = fence->reset();
    if (!reset_result) {
        CLIENT_ERROR("Failed to reset fence: {}", reset_result.error().c_str());
        return;
    }

    // Acquire next swapchain image
    u32 image_index = m_GraphicsDevice->acquire_next_image(m_ImageAvailableSemaphores[m_CurrentFrame].get());
    auto& gfx_ctx = *m_GraphicsContext;

    // Begin render pass with framebuffer
    gfx_ctx.begin_render_pass(m_RenderPass.get(), m_Framebuffers[image_index].get());

    // Clear operations (must be inside render pass)
    static stl::array<f32, 4> clear_color{0.3f, 0.4f, 0.6f, 1.0f};
    gfx_ctx.clear_render_target(0, clear_color);
    gfx_ctx.clear_depth_stencil();

    // Bind pipeline
    gfx_ctx.bind_pipeline(m_PipelineState.get());
    gfx_ctx.set_viewport({
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<f32>(m_ClientExtent->width),
        .height = static_cast<f32>(m_ClientExtent->height),
        .min_depth = 0.0f,
        .max_depth = 1.0f,
    });
    // Rendering
    {
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
                gfx_ctx.bind_index_buffer(m_RTIndexBuffers[cpu_data.index_id]);
                auto* per_draw = comp.per_draw_constants();
                gfx_ctx.push_constants(per_draw, sizeof(components::PerDrawConstants));
                gfx_ctx.draw_indexed(cpu_data.indices_size, 1);
            }
        }
    }
    gfx_ctx.end_render_pass();

    // Close and submit the command buffer with synchronization
    auto close_result = gfx_ctx.close();
    if (!close_result) {
        CORE_CRITICAL("Failed to close graphics context: {}", close_result.error().c_str());
        return;
    }

    // Submit to queue with wait/signal semaphores and fence
    stl::array<render::ISemaphore*, 1> wait_semaphores{m_ImageAvailableSemaphores[m_CurrentFrame].get()};
    stl::array<render::ISemaphore*, 1> signal_semaphores{m_RenderFinishedSemaphores[m_CurrentFrame].get()};
    stl::array<render::IContext*, 1> contexts{&gfx_ctx};

    render::QueueSubmitDesc submit_desc{};
    submit_desc.wait_semaphores = wait_semaphores;
    submit_desc.command_contexts = contexts;
    submit_desc.signal_semaphores = signal_semaphores;
    submit_desc.signal_fence = m_InFlightFences[m_CurrentFrame].get();

    m_DirectQueue->submit(submit_desc);

    // Present with render finished semaphore
    stl::array<render::ISemaphore*, 1> present_wait{m_RenderFinishedSemaphores[m_CurrentFrame].get()};
    auto present_result = m_GraphicsDevice->present(present_wait);
    if (!present_result) {
        CORE_CRITICAL("Failed to present: {}", present_result.error().c_str());
        return;
    }

    // Advance to next frame
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void SandboxGameContext::resize_depth_texture() {
    m_MainCamera = {CAMERA_FOV, static_cast<f32>(m_ClientExtent->width) / m_ClientExtent->height, 0.1f, 1000.f};
    // Release old texture by reassigning
    // TODO: make this return stl::result
    auto depth_result = m_MemoryAllocator->allocate_texture({
        .usage = sf::render::TextureUsage::DepthStencil,
        .format = sf::render::Format::D32_FLOAT,
        .width = static_cast<u32>(m_ClientExtent->width),
        .height = static_cast<u32>(m_ClientExtent->height),
        .name = "Depth Texture",
    });
    if (!depth_result) {
        CLIENT_CRITICAL("Failed to create depth texture while resizing: {}", depth_result.error().c_str());
        return;
    }
    m_DepthTexture = std::move(*depth_result);
}
