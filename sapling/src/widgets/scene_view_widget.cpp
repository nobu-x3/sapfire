#include "widgets/scene_view_widget.h"
#include "editor_context.h"

#include <QImage>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWindow>
#include <SDL3/SDL.h>
#include <core/logger.h>
#include <stl/result.h>

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
    m_Initialized = true;
    auto load_contents_res = load_contents();
    if (!load_contents_res) {
        CLIENT_CRITICAL("Failed to load contents: {}.", load_contents_res.error().c_str());
        return;
    }
    CLIENT_INFO("SceneViewWidget initialized successfully!");
}

sf::stl::result<> SceneViewWidget::load_contents() {
    auto* device = EditorContext::instance().graphics_device();
    if (!device)
        return sf::stl::make_error("cannot load contents when graphics device is not initialized");

    auto& ctx = EditorContext::instance();
    auto* allocator = ctx.memory_allocator();
    if (!allocator)
        return sf::stl::make_error("memory allocator not initialized");

    auto pipeline_result = device->create_graphics_pipeline({
        .shader_module =
            {
                .vertex_shader_path = "assets/shaders/bindless_vulkan.vert.spv",
                .vertex_entry_point = "VS",
                .pixel_shader_path = "assets/shaders/bindless_vulkan.frag.spv",
                .pixel_entry_point = "PS",
            },
        .name = "Bindless Pipeline",
    });
    if (!pipeline_result)
        return sf::stl::make_error("failed to create bindless pipeline");
    m_PipelineState = std::move(*pipeline_result);

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
    // assets::SceneWriter writer{&m_ECManager, m_AssetManager.get()};
    // writer.deserealize("test_scene.scene", [&](sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths) {
    //     create_render_component(entity, resource_paths);
    // });
    return sf::stl::success;
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

void SceneViewWidget::update_frame(sf::f32 delta_time) {
    if (!m_Initialized) {
        return;
    }
    if (m_NeedsResize) {
        auto* device = EditorContext::instance().graphics_device();
        if (device) {
            auto resize_result = device->resize_window(width(), height());
            if (!resize_result) {
                CLIENT_ERROR("Failed to resize scene view: {}", resize_result.error().c_str());
                return;
            }
        }
        m_NeedsResize = false;
    }
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
    auto result = device->begin_frame();
    if (!result.has_value()) {
        CLIENT_CRITICAL("Failed to begin frame: {}", result.error().c_str());
        return;
    }
    auto& back_buffer = device->get_current_back_buffer();
    auto& editor_ctx = EditorContext::instance();
    auto& ctx = *editor_ctx.graphics_context();
    auto reset_res = ctx.reset();
    if (!reset_res) {
        CLIENT_CRITICAL("Failed to reset command buffer: {}", reset_res.error().c_str());
        return;
    }
    ctx.transition_barrier(back_buffer, sf::render::ResourceState::Present, sf::render::ResourceState::RenderTarget);
    ctx.execute_resource_barriers();
    ctx.begin_render_pass(back_buffer, &m_DepthTexture);
    static sf::stl::array<sf::f32, 4> clear_color{0.1f, 0.1f, 0.1f, 1.0f};
    ctx.clear_render_target_view(back_buffer, clear_color);
    ctx.clear_depth_stencil_view(m_DepthTexture);
    {
        ctx.set_pipeline_state(m_PipelineState);
        ctx.set_root_signature();
        ctx.set_viewport({
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<sf::f32>(width()),
            .height = static_cast<sf::f32>(height()),
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        });
        sf::stl::array<sf::render::IDescriptorHeap*, 2> heaps {editor_ctx.cbv_srv_uav_heap(), editor_ctx.sampler_heap()};
        ctx.set_descriptor_heaps(heaps);
        sf::math::frustum camera_frustum = sf::math::frustum::create_from_matrix(m_MainCamera.projection);
        sf::math::mat4 view = m_MainCamera.view();
        sf::math::mat4 inv_view = view.inversed();
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
                sf::components::CPUData cpu_data = comp.cpu_data();
                ctx.set_index_buffer(m_RTIndexBuffers[cpu_data.index_id]);
                auto* per_draw = comp.per_draw_constants();
                ctx.set_32_bit_constants(per_draw, sizeof(sf::components::PerDrawConstants) / sizeof(sf::u32));
                ctx.draw_indexed_instanced(cpu_data.indices_size, 1);
            }
        }
    }
    ctx.end_render_pass();
    ctx.transition_barrier(back_buffer, sf::render::ResourceState::RenderTarget, sf::render::ResourceState::Present);
    ctx.execute_resource_barriers();
    auto end_frame_res = device->end_frame(&ctx);
    if (!end_frame_res) {
        CORE_CRITICAL("Failed to end frame: {}", end_frame_res.error());
        return;
    }
    device->present(); // In headless mode, this just advances the frame index
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
