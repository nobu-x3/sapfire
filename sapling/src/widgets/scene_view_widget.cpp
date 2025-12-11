#include "widgets/scene_view_widget.h"
#include <QImage>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWindow>
#include <SDL3/SDL.h>
#include <core/logger.h>
#include <stl/result.h>
#include "editor_context.h"
#include "memory/memory.h"
#include "render/bindless_resource_registry.h"
#include "render/i_graphics_device.h"
#include "render/i_pipeline_layout.h"
#include "render/i_render_pass.h"
#include "render/render_api.h"

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
    CLIENT_INFO("Initializing SceneViewWidget rendering...");
    // In headless mode, we still need an SDL window to create the Vulkan instance
    // but it's never shown or used for presentation
    m_SDLWindow = SDL_CreateWindow("Sapling Viewport (Hidden)", 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    if (!m_SDLWindow) {
        CLIENT_ERROR("Failed to create SDL window: {}", SDL_GetError());
        return;
    }
    auto& ctx = EditorContext::instance();
    ctx.initialize(m_SDLWindow, width(), height());
    auto* device = ctx.graphics_device();
    for (sf::u32 i = 0; i < sf::render::MAX_FRAMES_IN_FLIGHT; ++i) {
        auto fence_result = device->create_fence(true, "Frame Fence");
        if (!fence_result) {
            CLIENT_CRITICAL("Failed to create fence {}: {}", i, fence_result.error().c_str());
            continue;
        }
        m_FrameResources[i].m_InFlightFence = std::move(*fence_result);
        auto context_result = device->create_graphics_context();
        if (!context_result) {
            CLIENT_CRITICAL("Failed to create graphics context: {}", context_result.error().c_str());
            return;
        }
        m_FrameResources[i].m_GraphicsContext = std::move(*context_result);
        // Note: In headless mode, neither image available nor render finished semaphores are needed
        // - acquire_next_image doesn't signal image available semaphores
        // - present() is a no-op and doesn't wait on render finished semaphores
        // We only need fences for CPU/GPU synchronization
    }
    // Create graphics context for this widget
    m_Initialized = true;
    auto load_contents_res = load_contents();
    if (!load_contents_res) {
        CLIENT_CRITICAL("Failed to load contents: {}.", load_contents_res.error().c_str());
        return;
    }
    CLIENT_INFO("SceneViewWidget initialized successfully!");
}

sf::stl::result<sf::stl::unique_ptr<sf::render::IPipelineLayout>>
create_bindless_layout(sf::render::IGraphicsDevice* device, sf::stl::span<const sf::render::DescriptorSetLayout> layouts_span) {
    sf::stl::vector<sf::render::DescriptorSetLayout> layouts{sf::mem::MemTag::Temp};
    layouts.reserve(layouts_span.size());
    for (auto&& l : layouts_span) {
        layouts.push_back(l);
    }
    sf::render::PipelineLayoutDesc layout_desc{
        .descriptor_set_layouts = std::move(layouts),
        .name = "Scene View Bindless Pipeline Layout",
    };
    layout_desc.push_constant_ranges.push_back({
        .stages = sf::render::ShaderStage::AllGraphics,
        .offset = 0,
        .size = sizeof(sf::u32) * 64, // 256 bytes
    });
    return device->create_pipeline_layout(layout_desc);
}

sf::stl::result<> SceneViewWidget::load_contents() {
    auto* device = EditorContext::instance().graphics_device();
    if (!device)
        return sf::stl::make_error("cannot load contents when graphics device is not initialized");
    auto& ctx = EditorContext::instance();
    auto* allocator = ctx.memory_allocator();
    if (!allocator)
        return sf::stl::make_error("memory allocator not initialized");
    // Create render pass
    auto pipeline_layout_res = create_bindless_layout(device, ctx.bindless_descriptor_set_layouts());
    if (!pipeline_layout_res) {
        return sf::stl::make_error("Failed to create bindless pipeline layout: {}.", pipeline_layout_res.error().c_str());
    }
    m_PipelineLayout = std::move(*pipeline_layout_res);
    sf::render::RenderPassDesc::AttachmentDesc depth_attachment{
        .format = sf::render::Format::D32_FLOAT,
        .load_op = sf::render::LoadOp::Clear,
        .store_op = sf::render::StoreOp::Store,
        .initial_layout = sf::render::ResourceState::Undefined,
        .final_layout = sf::render::ResourceState::DepthWrite,
    };
    auto render_pass_result = device->create_render_pass({
        .color_attachments = {sf::mem::MemTag::Temp,
                              1,
                              {
                                  .format = device->get_back_buffer(0).format,
                                  .load_op = sf::render::LoadOp::Clear,
                                  .store_op = sf::render::StoreOp::Store,
                                  .initial_layout = sf::render::ResourceState::Undefined,
                                  .final_layout = sf::render::ResourceState::RenderTarget,
                              }},
        .depth_attachment = depth_attachment,
        .name = "Editor Render Pass",
    });
    if (!render_pass_result)
        return sf::stl::make_error("Failed to create render pass: {}", render_pass_result.error().c_str());
    m_RenderPass = std::move(*render_pass_result);
    auto pipeline_result = device->create_graphics_pipeline({
        .layout = m_PipelineLayout.get(), // Using bindless layout
        .render_pass = m_RenderPass.get(),
        .vertex_shader =
            {
                .path = "assets/shaders/bindless-vulkan.vert.spv",
                .entry_point = "VS",
            },
        .pixel_shader =
            {
                .path = "assets/shaders/bindless-vulkan.frag.spv",
                .entry_point = "PS",
            },
    });
    if (!pipeline_result)
        return sf::stl::make_error("failed to create bindless pipeline");
    m_PipelineState = std::move(pipeline_result.value());
    auto cbv_result = allocator->allocate_buffer(sf::render::BufferCreationDesc{
        .usage = sf::render::BufferUsage::Constant,
        .size_in_bytes = sizeof(PassConstants),
        .name = "Main Pass Constant Buffer",
    });
    if (!cbv_result)
        return sf::stl::make_error("failed to create main pass constant buffer: {}", cbv_result.error().c_str());
    m_MainPassCB = std::move(*cbv_result);
    auto depth_result = allocator->allocate_texture({
        .usage = sf::render::TextureUsage::DepthStencil,
        .format = sf::render::Format::D32_FLOAT,
        .width = static_cast<sf::u32>(width()),
        .height = static_cast<sf::u32>(height()),
        .name = "Depth Texture",
    });
    if (!depth_result)
        return sf::stl::make_error("failed to create depth texture: {}", depth_result.error().c_str());
    m_DepthTexture = std::move(*depth_result);
    // Create framebuffers for each swapchain image
    for (sf::u32 i = 0; i < device->get_back_buffer_count(); ++i) {
        sf::stl::array<sf::render::Texture*, 1> color_attachments{&device->get_back_buffer(i)};
        auto framebuffer_result = device->create_framebuffer({
            .render_pass = m_RenderPass.get(),
            .color_attachments = color_attachments,
            .depth_attachment = &m_DepthTexture,
            .width = static_cast<sf::u32>(width()),
            .height = static_cast<sf::u32>(height()),
            .name = "Editor Framebuffer",
        });
        if (!framebuffer_result)
            return sf::stl::make_error("Failed to create framebuffer {}: {}", i, framebuffer_result.error().c_str());
        m_Framebuffers[i] = std::move(*framebuffer_result);
    }
    return sf::stl::success;
}

sf::stl::result<> SceneViewWidget::rebuild_framebuffers() {
    auto* device = EditorContext::instance().graphics_device();
    if (!device)
        return sf::stl::make_error("graphics device not initialized");
    auto& ctx = EditorContext::instance();
    auto* allocator = ctx.memory_allocator();
    if (!allocator)
        return sf::stl::make_error("memory allocator not initialized");
    // Destroy old framebuffers
    for (auto& fb : m_Framebuffers) {
        fb.reset();
    }
    // Recreate depth texture with new dimensions
    auto depth_result = allocator->allocate_texture({
        .usage = sf::render::TextureUsage::DepthStencil,
        .format = sf::render::Format::D32_FLOAT,
        .width = static_cast<sf::u32>(width()),
        .height = static_cast<sf::u32>(height()),
        .name = "Depth Texture",
    });
    if (!depth_result)
        return sf::stl::make_error("failed to create depth texture: {}", depth_result.error().c_str());
    m_DepthTexture = std::move(*depth_result);
    // Recreate framebuffers for each swapchain image
    for (sf::u32 i = 0; i < device->get_back_buffer_count(); ++i) {
        sf::stl::array<sf::render::Texture*, 1> color_attachments{&device->get_back_buffer(i)};
        auto framebuffer_result = device->create_framebuffer({
            .render_pass = m_RenderPass.get(),
            .color_attachments = color_attachments,
            .depth_attachment = &m_DepthTexture,
            .width = static_cast<sf::u32>(width()),
            .height = static_cast<sf::u32>(height()),
            .name = "Editor Framebuffer",
        });
        if (!framebuffer_result)
            return sf::stl::make_error("Failed to create framebuffer {}: {}", i, framebuffer_result.error().c_str());
        m_Framebuffers[i] = std::move(*framebuffer_result);
    }
    return sf::stl::success;
}

struct ObjectConstants {
    sf::math::mat4 World{sf::math::mat4::identity()};
};

void SceneViewWidget::on_render_component_added(sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths) {
    auto& editor = EditorContext::instance();
    auto* ec_mgr = editor.ec_manager();
    auto* asset_mgr = editor.asset_manager();
    auto* gfx_device = editor.graphics_device();
    auto* allocator = editor.memory_allocator();
    auto* bindless_registry = editor.bindless_registry();
    bool already_has_component = ec_mgr->has_engine_component<sf::components::RenderComponent>(entity);
    auto* mesh_asset =
        resource_paths.mesh_path.empty() ? sf::assets::MeshRegistry::default_mesh() : asset_mgr->get_mesh(resource_paths.mesh_path);
    auto* texture_asset = resource_paths.texture_path.empty() ? sf::assets::TextureRegistry::default_texture(allocator, bindless_registry)
                                                              : asset_mgr->get_texture(resource_paths.texture_path);
    auto* material_asset = resource_paths.material_path.empty()
        ? sf::assets::MaterialRegistry::default_material(allocator, bindless_registry)
        : asset_mgr->get_material(resource_paths.material_path);
    if (!mesh_asset) {
        asset_mgr->import_mesh(resource_paths.mesh_path);
        mesh_asset = asset_mgr->get_mesh(resource_paths.mesh_path);
    }
    if (!texture_asset || !asset_mgr->is_texture_loaded_for_runtime(texture_asset->uuid)) {
        asset_mgr->import_texture(resource_paths.texture_path);
        texture_asset = asset_mgr->get_texture(resource_paths.texture_path);
    }
    if (!material_asset || !asset_mgr->material_resource_exists(material_asset->uuid)) {
        asset_mgr->import_material(resource_paths.material_path);
        material_asset = asset_mgr->get_material(resource_paths.material_path);
    }
    if (mesh_asset && mesh_asset->data.has_value()) {
        assert(mesh_asset->data->indices32.size() > 0);
        assert(mesh_asset->data->positions.size() > 0);
        assert(mesh_asset->data->normals.size() > 0);
        assert(mesh_asset->data->texcs.size() > 0);
        if (!already_has_component) {
            auto cbv_result = allocator->allocate_buffer(sf::render::BufferCreationDesc{
                .usage = sf::render::BufferUsage::Constant,
                .size_in_bytes = sizeof(ObjectConstants),
                .name = "Object Constants",
            });
            if (!cbv_result) {
                CLIENT_ERROR("Failed to allocated object CBV: {}.", cbv_result.error().c_str());
                return;
            }
            m_TransformBuffers.push_back(std::move(*cbv_result));
        }
        bool should_add_tangent = false;
        const bool should_allocate_mesh = !asset_mgr->mesh_resource_exists(resource_paths.mesh_path);
        if (should_allocate_mesh) {
            const sf::stl::string name = {sf::mem::MemTag::Strings,
                                          mesh_asset->uuid == sf::assets::MeshRegistry::default_mesh()->uuid ? "Default Mesh"
                                                                                                             : resource_paths.mesh_path};
            const size_t index_buffer_size = sizeof(sf::u16) * mesh_asset->data->indices16().size();
            const size_t vertex_pos_buf_size = sizeof(sf::math::vec3) * mesh_asset->data->positions.size();
            const size_t vertex_norm_buf_size = sizeof(sf::math::vec3) * mesh_asset->data->normals.size();
            const size_t vertex_uv_buf_size = sizeof(sf::math::vec2) * mesh_asset->data->texcs.size();
            auto ib_res = allocator->allocate_buffer(sf::render::BufferCreationDesc{
                .usage = sf::render::BufferUsage::Index,
                .size_in_bytes = index_buffer_size,
                .name = "Index Buffer",
            });
            if (!ib_res) {
                CLIENT_ERROR("Failed to allocated index buffer: {}.", ib_res.error().c_str());
                return;
            }
            ib_res->update(mesh_asset->data->indices16().data(), index_buffer_size);
            m_RTIndexBuffers.push_back(std::move(*ib_res));
            auto vbp_res = allocator->allocate_buffer(sf::render::BufferCreationDesc{
                .usage = sf::render::BufferUsage::Structured,
                .size_in_bytes = vertex_pos_buf_size,
                .name = "Vertex Position Buffer",
            });
            vbp_res->srv_index = bindless_registry->register_buffer(*vbp_res);
            vbp_res->update(mesh_asset->data->positions.data(), vertex_pos_buf_size);
            m_VertexPosBuffers.push_back(std::move(*vbp_res));
            auto vbn_res = allocator->allocate_buffer(sf::render::BufferCreationDesc{
                .usage = sf::render::BufferUsage::Structured,
                .size_in_bytes = vertex_norm_buf_size,
                .name = "Vertex Normals Buffer",
            });
            vbn_res->srv_index = bindless_registry->register_buffer(*vbn_res);
            vbn_res->update(mesh_asset->data->normals.data(), vertex_norm_buf_size);
            m_VertexNormalBuffers.push_back(std::move(*vbn_res));
            if (mesh_asset->data->tangentus.size() > 0) {
                const size_t vertex_tan_buf_size = sizeof(sf::math::vec3) * mesh_asset->data->tangentus.size();
                auto vbt_res = allocator->allocate_buffer(sf::render::BufferCreationDesc{
                    .usage = sf::render::BufferUsage::Structured,
                    .size_in_bytes = vertex_tan_buf_size,
                    .name = "Vertex Tan Buffer",
                });
                vbt_res->srv_index = bindless_registry->register_buffer(*vbt_res);
                vbt_res->update(mesh_asset->data->tangentus.data(), vertex_tan_buf_size);
                m_VertexTangentBuffers.push_back(std::move(*vbt_res));
                should_add_tangent = true;
            }
            auto vbuv_res = allocator->allocate_buffer(sf::render::BufferCreationDesc{
                .usage = sf::render::BufferUsage::Structured,
                .size_in_bytes = vertex_uv_buf_size,
                .name = "Vertex UV Buffer",
            });
            vbuv_res->srv_index = bindless_registry->register_buffer(*vbuv_res);
            vbuv_res->update(mesh_asset->data->texcs.data(), vertex_uv_buf_size);
            m_VertexUVBuffers.push_back(std::move(*vbuv_res));
        }
        auto cpu_data = sf::components::CPUData{
            .indices_size = static_cast<sf::u32>(mesh_asset->data->indices32.size()),
            .index_id = static_cast<sf::u32>(m_RTIndexBuffers.size() - 1),
            .position_idx = static_cast<sf::u32>(m_VertexPosBuffers.size() - 1),
            .normal_idx = static_cast<sf::u32>(m_VertexNormalBuffers.size() - 1),
            .tangent_idx = static_cast<sf::u32>(should_add_tangent ? m_VertexTangentBuffers.size() - 1 : 0),
            .uv_idx = static_cast<sf::u32>(m_VertexUVBuffers.size() - 1),
            .transform_buffer_idx = already_has_component
                ? ec_mgr->engine_component<sf::components::RenderComponent>(entity).cpu_data().transform_buffer_idx
                : static_cast<sf::u32>(m_TransformBuffers.size() - 1),
        };
        sf::u32 material_cbuffer_idx = 0;
        if (material_asset->uuid == sf::assets::MaterialRegistry::default_material(allocator, bindless_registry)->uuid) {
            material_cbuffer_idx = material_asset->material.material_cb_index;
        } else {
            material_cbuffer_idx = asset_mgr->material_resource_exists(resource_paths.material_path)
                ? asset_mgr->get_material_resource(resource_paths.material_path).gpu_idx
                : sf::assets::MaterialRegistry::default_material(allocator, bindless_registry)->material.material_cb_index;
        }
        sf::u32 texture_cbuffer_idx = 0;
        if (texture_asset->uuid == sf::assets::TextureRegistry::default_texture(allocator, bindless_registry)->uuid) {
            texture_cbuffer_idx = texture_asset->data.srv_index;
        } else {
            texture_cbuffer_idx = asset_mgr->texture_resource_exists(resource_paths.texture_path)
                ? asset_mgr->get_texture_resource(resource_paths.texture_path).gpu_idx
                : texture_asset->data.srv_index;
        }
        auto gpu_data = sf::components::PerDrawConstants{
            .position_buffer_idx = m_VertexPosBuffers.back().srv_index,
            .normal_buffer_idx = m_VertexNormalBuffers.back().srv_index,
            .tangent_buffer_idx = should_add_tangent ? m_VertexTangentBuffers.back().srv_index : 0,
            .uv_buffer_idx = m_VertexUVBuffers.back().srv_index,
            .scene_cbuffer_idx = already_has_component
                ? ec_mgr->engine_component<sf::components::RenderComponent>(entity).per_draw_constants()->scene_cbuffer_idx
                : m_TransformBuffers.back().cbv_index,
            .pass_cbuffer_idx = m_MainPassCB.cbv_index,
            .material_cbuffer_idx = material_cbuffer_idx,
            .texture_cbuffer_idx = texture_cbuffer_idx,
        };
        if (!should_allocate_mesh) {
            cpu_data = asset_mgr->get_mesh_resource(mesh_asset->uuid).cpu_data;
            gpu_data = asset_mgr->get_mesh_resource(mesh_asset->uuid).gpu_data;
            gpu_data.scene_cbuffer_idx = already_has_component
                ? ec_mgr->engine_component<sf::components::RenderComponent>(entity).per_draw_constants()->scene_cbuffer_idx
                : m_TransformBuffers.back().cbv_index;
            gpu_data.material_cbuffer_idx = material_cbuffer_idx;
            gpu_data.texture_cbuffer_idx = texture_cbuffer_idx;
            cpu_data.transform_buffer_idx = already_has_component
                ? ec_mgr->engine_component<sf::components::RenderComponent>(entity).cpu_data().transform_buffer_idx
                : static_cast<sf::u32>(m_TransformBuffers.size() - 1);
        }
        asset_mgr->load_mesh_resource(resource_paths.mesh_path, {cpu_data, gpu_data});
        sf::components::RenderComponent render_component{mesh_asset->uuid,
                                                         texture_asset->uuid,
                                                         material_asset->uuid,
                                                         cpu_data,
                                                         gpu_data,
                                                         [this, entity, asset_mgr](sf::components::RenderComponent* component) {
                                                             if (component) {
                                                                 const auto old_cpu_data = component->cpu_data();
                                                                 const auto old_gpu_data = component->per_draw_constants();
                                                                 // This happens after the mesh uuid is set to the new one, so we're getting
                                                                 // the just assigned uuid
                                                                 const auto mesh_uuid = component->mesh_uuid();
                                                                 // The mesh we just assigned may not be allocated yet
                                                                 const auto texture_uuid = component->texture_uuid();
                                                                 const auto texture_path = asset_mgr->get_texture_path(texture_uuid);
                                                                 const auto mesh_path = asset_mgr->get_mesh_path(mesh_uuid);
                                                                 const auto material_uuid = component->material_uuid();
                                                                 const auto material_path = asset_mgr->get_material_path(material_uuid);
                                                                 if (!asset_mgr->mesh_resource_exists(mesh_path) ||
                                                                     !asset_mgr->material_resource_exists(material_path) ||
                                                                     !asset_mgr->texture_resource_exists(texture_path)) {
                                                                     sf::RenderComponentResourcePaths paths{.mesh_path = mesh_path,
                                                                                                            .texture_path = texture_path,
                                                                                                            .material_path = material_path};
                                                                     on_render_component_added(entity, paths);
                                                                     return;
                                                                 }
                                                                 auto data = asset_mgr->get_mesh_resource(mesh_path);
                                                                 data.gpu_data.scene_cbuffer_idx = old_gpu_data->scene_cbuffer_idx;
                                                                 component->cpu_data(data.cpu_data);
                                                                 component->per_draw_constants(data.gpu_data);
                                                             }
                                                         }};
        ec_mgr->add_engine_component<sf::components::RenderComponent>(entity, render_component);
    }
}

void SceneViewWidget::shutdown_rendering() {
    if (!m_Initialized) {
        return;
    }
    CLIENT_INFO("Shutting down SceneViewWidget...");
    EditorContext::instance().shutdown();
    if (m_SDLWindow) {
        SDL_DestroyWindow(m_SDLWindow);
        m_SDLWindow = nullptr;
    }
    m_Initialized = false;
}

void SceneViewWidget::update_buffers(sf::f32 delta_time) {
    m_MainCamera.update(delta_time);
    m_PassConstants.delta_time = delta_time;
    m_PassConstants.total_time += delta_time;
    m_PassConstants.proj = m_MainCamera.projection;
    m_PassConstants.view = m_MainCamera.view();
    m_PassConstants.view_proj = m_PassConstants.view * m_PassConstants.proj;
    sf::math::vec4 pos = m_MainCamera.transform.position();
    m_PassConstants.EyePosW = {pos.x, pos.y, pos.z};
    m_PassConstants.near_z = m_MainCamera.near_plane;
    m_PassConstants.far_z = m_MainCamera.far_plane;
    m_PassConstants.inv_proj = m_PassConstants.proj.inversed();
    m_PassConstants.inv_view = m_PassConstants.view.inversed();
    m_PassConstants.inv_view_proj = m_PassConstants.view_proj.inversed();
    // TODO: add the rest of fields
    m_MainPassCB.update(&m_PassConstants, sizeof(m_PassConstants));
}

void SceneViewWidget::update_frame(sf::f32 delta_time) {
    if (!m_Initialized) {
        return;
    }
    if (m_NeedsResize) {
        auto* device = EditorContext::instance().graphics_device();
        if (device) {
            auto resize_result = device->resize_swapchain(width(), height());
            if (!resize_result) {
                CLIENT_ERROR("Failed to resize scene view: {}", resize_result.error().c_str());
                return;
            }
            // After resize, we need to recreate framebuffers and depth texture
            // since they reference the old swapchain images
            auto rebuild_result = rebuild_framebuffers();
            if (!rebuild_result) {
                CLIENT_ERROR("Failed to rebuild framebuffers after resize: {}", rebuild_result.error().c_str());
                return;
            }
        }
        m_NeedsResize = false;
    }
    update_buffers(delta_time);
    render();
}

void SceneViewWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    if (m_RenderedImage.isNull())
        return;
    painter.drawImage(rect(), m_RenderedImage);
}

void SceneViewWidget::render() {
    if (!m_Initialized) {
        return;
    }
    auto* device = EditorContext::instance().graphics_device();
    if (!device) {
        return;
    }
    auto* ec_mgr = EditorContext::instance().ec_manager();
    if (!ec_mgr) {
        CLIENT_ERROR("Entity Component Manager is not initialized, will not render.");
        return;
    }
    auto* asset_mgr = EditorContext::instance().asset_manager();
    if (!asset_mgr) {
        CLIENT_ERROR("Asset Manager is not initialized, will not render.");
        return;
    }
    auto& fence = m_FrameResources[m_CurrentFrame].m_InFlightFence;
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
    // In headless we're just indexing offscreen render targets
    sf::u32 image_index = m_CurrentFrame;
    auto& ctx = *m_FrameResources[m_CurrentFrame].m_GraphicsContext;
    auto reset_res = ctx.reset();
    if (!reset_res) {
        CLIENT_CRITICAL("Failed to reset command buffer: {}", reset_res.error().c_str());
        return;
    }
    ctx.transition_image_layout(device->get_back_buffer(image_index), sf::render::ResourceState::Undefined,
                                sf::render::ResourceState::RenderTarget);
    // Begin render pass with framebuffer
    ctx.begin_render_pass(m_RenderPass.get(), m_Framebuffers[image_index].get());
    ctx.set_viewport({
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<sf::f32>(width()),
        .height = static_cast<sf::f32>(height()),
        .min_depth = 0.0f,
        .max_depth = 1.0f,
    });
    ctx.set_scissor({0, 0, width(), height()});
    // Clear operations (must be inside render pass)
    static sf::stl::array<sf::f32, 4> clear_color{0.1f, 0.1f, 0.1f, 1.0f};
    ctx.clear_render_target(0, clear_color);
    ctx.clear_depth_stencil();
    {
        ctx.bind_pipeline(m_PipelineState.get());
        // Bind bindless descriptor sets
        auto* registry = EditorContext::instance().bindless_registry();
        for (sf::u32 i = 0; i < registry->get_descriptor_set_count(); ++i) {
            ctx.bind_descriptor_set(i, registry->get_descriptor_set(i));
        }
        // TODO: With bindless, descriptor heaps are bound at descriptor set level, not per-draw
        // sf::stl::array<sf::render::IDescriptorHeap*, 2> heaps{editor_ctx.bindless_registry(), nullptr};
        // ctx.set_descriptor_heaps(heaps);
        sf::math::frustum camera_frustum = sf::math::frustum::create_from_matrix(m_MainCamera.projection);
        const sf::math::mat4& view = m_PassConstants.view;
        const sf::math::mat4& inv_view = m_PassConstants.inv_view;
        auto& render_components = ec_mgr->engine_components<sf::components::RenderComponent>();
        for (auto& comp : render_components) {
            sf::components::Transform transform;
            if (!ec_mgr->get_other_engine_component<sf::components::RenderComponent, sf::components::Transform>(comp, transform))
                continue;
            const auto* mesh_asset = asset_mgr->get_mesh(comp.mesh_uuid());
            sf::math::aabb aabb = mesh_asset->data->aabb;
            sf::math::mat4 world = transform.transform();
            sf::math::mat4 inv_world = world.inversed();
            // View space to object local space
            sf::math::mat4 view_to_local = inv_view * inv_world;
            // Transform camera frustum from view space to object's local space
            sf::math::frustum local_space_frustum = camera_frustum.transform(view_to_local);
            if (local_space_frustum.contains(aabb) != sf::math::ContainmentType::Disjoint) {
                // TODO: make this actually thread safe. We currently might run into situation where m_RTIIndexBuffers.size() <
                // cpu_data.index.id because on_render_component_added() is called AFTER we added a render component.
                sf::components::CPUData cpu_data = comp.cpu_data();
                if (cpu_data.index_id >= m_RTIndexBuffers.size())
                    continue;
                ctx.bind_index_buffer(m_RTIndexBuffers[cpu_data.index_id], sf::render::Format::R16_UINT);
                auto* per_draw = comp.per_draw_constants();
                ctx.push_constants(per_draw, sizeof(sf::components::PerDrawConstants));
                ctx.draw_indexed(cpu_data.indices_size, 1);
            }
        }
    }
    ctx.end_render_pass();
    ctx.transition_image_layout(device->get_back_buffer(image_index), sf::render::ResourceState::RenderTarget,
                                sf::render::ResourceState::CopySource);
    // Close and submit the command buffer with synchronization
    auto close_result = ctx.close();
    if (!close_result) {
        CLIENT_CRITICAL("Failed to close graphics context: {}", close_result.error().c_str());
        return;
    }
    // Submit to queue with fence only (no semaphores in headless mode)
    // In headless mode:
    // - No wait semaphores (no image acquisition)
    // - No signal semaphores (present() doesn't wait on them)
    // - Only use fence for CPU/GPU sync
    sf::stl::array<sf::render::IContext*, 1> contexts{&ctx};
    sf::render::QueueSubmitDesc submit_desc{};
    submit_desc.wait_semaphores = {};
    submit_desc.command_contexts = contexts;
    submit_desc.signal_semaphores = {}; // No signal semaphores in headless mode
    submit_desc.signal_fence = fence.get();
    auto submit_result = EditorContext::instance().direct_queue()->submit(submit_desc);
    if (!submit_result) {
        CLIENT_CRITICAL("Failed to submit to queue: {}", submit_result.error().c_str());
        return;
    }
    // In headless mode, present() is a no-op, so just manually advance the frame index
    m_CurrentFrame = (m_CurrentFrame + 1) % sf::render::MAX_FRAMES_IN_FLIGHT;
    // @TODO: Implement proper texture readback using ICopyContext and staging buffer
    // The old device->read_texture_pixels() method has been removed as part of the stateless API refactor.
    // Proper implementation requires:
    // 1. Create a staging buffer with the texture size (allocator->allocate_buffer with BufferUsage::Staging)
    // 2. Use ICopyContext::copy_texture_to_buffer to copy back_buffer to staging buffer
    // 3. Execute the copy command on the copy queue
    // 4. Wait for the copy to complete (fence/synchronization)
    // 5. Map the staging buffer and copy data to m_RenderedImage.bits()
    // 6. Unmap the staging buffer
    // For now, skip the readback to unblock compilation
    update(); // Trigger Qt paintEvent to display the image
}

void SceneViewWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_Initialized && m_SDLWindow) {
        SDL_SetWindowSize(m_SDLWindow, event->size().width(), event->size().height());
        m_NeedsResize = true;
    }
}
