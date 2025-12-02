#include "core/logger.h"
#include "engpch.h"

#include "components/render_component.h"
#include "core/application.h"
#include "core/game_context.h"
#include "memory/memory.h"

namespace sf {
    GameContext::GameContext(const GameContextCreationDesc& desc) : m_ClientExtent(desc.client_extent), m_GraphicsDevice(nullptr) {
        if (!sf::render::RenderBackend::is_initialized()) {
#if defined(SF_PLATFORM_WINDOWS) && defined(SF_ENABLE_DX12)
            sf::render::RenderBackend::initialize(sf::render::RenderAPI::DX12);
#else
            sf::render::RenderBackend::initialize(sf::render::RenderAPI::Vulkan);
#endif
        }
        m_GraphicsDevice =
            sf::render::RenderBackend::create_device(sf::render::SwapchainCreationDesc{.window_handle = desc.window_handle,
                                                                                       .width = static_cast<u32>(m_ClientExtent->width),
                                                                                       .height = static_cast<u32>(m_ClientExtent->height),
                                                                                       .buffer_count = 3,
                                                                                       .format = sf::render::Format::RGBA16_FLOAT,
                                                                                       .refresh_rate = 120});

        // Create rendering resources using factory methods
        auto cbv_heap_result = m_GraphicsDevice->create_cbv_srv_uav_heap({
            .descriptor_count = 3000000,
            .name = "Game CBV/SRV/UAV Heap",
        });
        if (!cbv_heap_result) {
            CORE_ERROR("Failed to create CBV/SRV/UAV heap: {}", cbv_heap_result.error().c_str());
            return;
        }
        m_CbvSrvUavHeap = std::move(*cbv_heap_result);

        auto sampler_heap_result = m_GraphicsDevice->create_sampler_heap({
            .descriptor_count = 1000000,
            .name = "Game Sampler Heap",
        });
        if (!sampler_heap_result) {
            CORE_ERROR("Failed to create sampler heap: {}", sampler_heap_result.error().c_str());
            return;
        }
        m_SamplerHeap = std::move(*sampler_heap_result);

        // Allocate descriptor sets (Vulkan-specific, no-op on DX12)
        auto cbv_alloc_result = m_CbvSrvUavHeap->allocate_descriptor_set();
        if (!cbv_alloc_result) {
            CORE_ERROR("Failed to allocate CBV/SRV/UAV descriptor set: {}", cbv_alloc_result.error().c_str());
            return;
        }

        auto sampler_alloc_result = m_SamplerHeap->allocate_descriptor_set();
        if (!sampler_alloc_result) {
            CORE_ERROR("Failed to allocate sampler descriptor set: {}", sampler_alloc_result.error().c_str());
            return;
        }

        auto queue_result = m_GraphicsDevice->create_direct_queue("Game Direct Queue");
        if (!queue_result) {
            CORE_ERROR("Failed to create direct queue: {}", queue_result.error().c_str());
            return;
        }
        m_DirectQueue = std::move(*queue_result);

        auto context_result = m_GraphicsDevice->create_graphics_context();
        if (!context_result) {
            CORE_ERROR("Failed to create graphics context: {}", context_result.error().c_str());
            return;
        }
        m_GraphicsContext = std::move(*context_result);

        auto allocator_result = m_GraphicsDevice->create_memory_allocator();
        if (!allocator_result) {
            CORE_ERROR("Failed to create memory allocator: {}", allocator_result.error().c_str());
            return;
        }
        m_MemoryAllocator = std::move(*allocator_result);

        m_AssetManager = stl::make_unique<assets::AssetManager>(mem::MemTag::Logic,
                                                                assets::AssetManagerCreationDesc{
                                                                    .device = m_GraphicsDevice.get(),
                                                                    .mesh_registry_path = desc.mesh_registry_path,
                                                                    .texture_registry_path = desc.texture_registry_path,
                                                                    .material_registry_path = desc.material_registry_path,
                                                                });
        m_PhysicsEngine = stl::make_unique<physics::PhysicsEngine>(mem::MemTag::Physics, &m_ECManager);
    }

    void GameContext::init() { load_contents(); }

    stl::result<> GameContext::create_render_component(Entity entity, const RenderComponentResourcePaths& resource_paths) {
        const bool already_has_component = m_ECManager.has_engine_component<components::RenderComponent>(entity);
        auto* mesh_asset =
            resource_paths.mesh_path.empty() ? assets::MeshRegistry::default_mesh() : m_AssetManager->get_mesh(resource_paths.mesh_path);
        auto* texture_asset = resource_paths.texture_path.empty()
            ? assets::TextureRegistry::default_texture(m_MemoryAllocator.get(), m_CbvSrvUavHeap.get())
            : m_AssetManager->get_texture(resource_paths.texture_path);
        auto* material_asset = resource_paths.material_path.empty()
            ? assets::MaterialRegistry::default_material(m_MemoryAllocator.get(), m_CbvSrvUavHeap.get())
            : m_AssetManager->get_material(resource_paths.material_path);
        if (!mesh_asset) {
            m_AssetManager->import_mesh(resource_paths.mesh_path);
            mesh_asset = m_AssetManager->get_mesh(resource_paths.mesh_path);
        }
        if (!texture_asset || !m_AssetManager->is_texture_loaded_for_runtime(texture_asset->uuid)) {
            m_AssetManager->import_texture(resource_paths.texture_path);
            texture_asset = m_AssetManager->get_texture(resource_paths.texture_path);
        }
        if (!material_asset || !m_AssetManager->material_resource_exists(material_asset->uuid)) {
            m_AssetManager->import_material(resource_paths.material_path);
            material_asset = m_AssetManager->get_material(resource_paths.material_path);
        }
        if (mesh_asset && mesh_asset->data.has_value()) {
            assert(mesh_asset->data->indices32.size() > 0);
            assert(mesh_asset->data->positions.size() > 0);
            assert(mesh_asset->data->normals.size() > 0);
            assert(mesh_asset->data->texcs.size() > 0);
            if (!already_has_component) {
                auto transform_result = m_MemoryAllocator->allocate_buffer({
                    .usage = sf::render::BufferUsage::Constant,
                    .size_in_bytes = sizeof(ObjectConstants),
                    .name = "Transform buffer " + std::string(resource_paths.mesh_path.begin(), resource_paths.mesh_path.end()),
                });
                if (!transform_result) {
                    return stl::make_error("Failed to create transform buffer: {}", transform_result.error().data());
                }
                transform_result->cbv_index = m_CbvSrvUavHeap->allocate_cbv(*transform_result);
                m_TransformBuffers.emplace_back(std::move(*transform_result));
            }
            bool should_add_tangent = false;
            const bool should_allocate_mesh = !m_AssetManager->mesh_resource_exists(resource_paths.mesh_path);
            if (should_allocate_mesh) {
                const std::string name = mesh_asset->uuid == assets::MeshRegistry::default_mesh()->uuid
                    ? "Default Mesh"
                    : std::string(resource_paths.mesh_path.begin(), resource_paths.mesh_path.end());

                // Create index buffer with data
                sf::render::BufferCreationDesc index_desc{
                    .usage = sf::render::BufferUsage::Index,
                    .size_in_bytes = mesh_asset->data->indices16().size() * sizeof(u16),
                    .name = "Index buffer " + name,
                };
                auto index_buffer_result = m_MemoryAllocator->allocate_buffer(index_desc);
                if (!index_buffer_result) {
                    return stl::make_error("Failed to create index buffer: {}", index_buffer_result.error().data());
                }
                m_RTIndexBuffers.push_back(std::move(*index_buffer_result));

                // Create vertex position buffer with data
                sf::render::BufferCreationDesc pos_desc{
                    .usage = sf::render::BufferUsage::Structured,
                    .size_in_bytes = mesh_asset->data->positions.size() * sizeof(sf::math::vec3),
                    .name = "Vertex Pos buffer " + name,
                };
                auto vertex_pos_buffer_result = m_MemoryAllocator->allocate_buffer(pos_desc);
                if (!vertex_pos_buffer_result) {
                    return stl::make_error("Failed to create vertex position buffer: {}", vertex_pos_buffer_result.error().data());
                }
                vertex_pos_buffer_result->srv_index = m_CbvSrvUavHeap->allocate_srv(*vertex_pos_buffer_result);
                m_VertexPosBuffers.push_back(std::move(*vertex_pos_buffer_result));

                // Create vertex normal buffer with data
                sf::render::BufferCreationDesc norm_desc{
                    .usage = sf::render::BufferUsage::Structured,
                    .size_in_bytes = mesh_asset->data->normals.size() * sizeof(sf::math::vec3),
                    .name = "Vertex Norm buffer " + name,
                };
                auto vertex_normal_buffer_result = m_MemoryAllocator->allocate_buffer(norm_desc);
                if (!vertex_normal_buffer_result) {
                    return stl::make_error("Failed to create vertex normal buffer: {}", vertex_normal_buffer_result.error().data());
                }
                vertex_normal_buffer_result->srv_index = m_CbvSrvUavHeap->allocate_srv(*vertex_normal_buffer_result);
                m_VertexNormalBuffers.push_back(std::move(*vertex_normal_buffer_result));

                if (mesh_asset->data->tangentus.size() > 0) {
                    sf::render::BufferCreationDesc tang_desc{
                        .usage = sf::render::BufferUsage::Structured,
                        .size_in_bytes = mesh_asset->data->tangentus.size() * sizeof(sf::math::vec3),
                        .name = "Vertex Tang buffer " + name,
                    };
                    auto tangentus_buffer_result = m_MemoryAllocator->allocate_buffer(tang_desc);
                    if (!tangentus_buffer_result) {
                        return stl::make_error("Failed to create vertex tangent buffer: {}", tangentus_buffer_result.error().data());
                    }
                    tangentus_buffer_result->srv_index = m_CbvSrvUavHeap->allocate_srv(*tangentus_buffer_result);
                    m_VertexTangentBuffers.push_back(std::move(*tangentus_buffer_result));
                    should_add_tangent = true;
                }

                sf::render::BufferCreationDesc uv_desc{
                    .usage = sf::render::BufferUsage::Structured,
                    .size_in_bytes = mesh_asset->data->texcs.size() * sizeof(sf::math::vec2),
                    .name = "Vertex UV buffer " + name,
                };
                auto uv_result = m_MemoryAllocator->allocate_buffer(uv_desc);
                if (!uv_result) {
                    return stl::make_error("Failed to create vertex UV buffer: {}", uv_result.error().data());
                }
                uv_result->srv_index = m_CbvSrvUavHeap->allocate_srv(*uv_result);
                m_VertexUVBuffers.push_back(std::move(*uv_result));
            }
            auto cpu_data = components::CPUData{
                .indices_size = static_cast<u32>(mesh_asset->data->indices32.size()),
                .index_id = static_cast<u32>(m_RTIndexBuffers.size() - 1),
                .position_idx = static_cast<u32>(m_VertexPosBuffers.size() - 1),
                .normal_idx = static_cast<u32>(m_VertexNormalBuffers.size() - 1),
                .tangent_idx = static_cast<u32>(should_add_tangent ? m_VertexTangentBuffers.size() - 1 : 0),
                .uv_idx = static_cast<u32>(m_VertexUVBuffers.size() - 1),
                .transform_buffer_idx = already_has_component
                    ? m_ECManager.engine_component<components::RenderComponent>(entity).cpu_data().transform_buffer_idx
                    : static_cast<u32>(m_TransformBuffers.size() - 1),
            };
            u32 material_cbuffer_idx = 0;
            if (material_asset->uuid == assets::MaterialRegistry::default_material(m_MemoryAllocator.get(), m_CbvSrvUavHeap.get())->uuid) {
                material_cbuffer_idx = material_asset->material.material_cb_index;
            } else {
                material_cbuffer_idx = m_AssetManager->material_resource_exists(resource_paths.material_path)
                    ? m_AssetManager->get_material_resource(resource_paths.material_path).gpu_idx
                    : assets::MaterialRegistry::default_material(m_MemoryAllocator.get(), m_CbvSrvUavHeap.get())
                          ->material.material_cb_index;
            }
            u32 texture_cbuffer_idx = 0;
            if (texture_asset->uuid == assets::TextureRegistry::default_texture(m_MemoryAllocator.get(), m_CbvSrvUavHeap.get())->uuid) {
                texture_cbuffer_idx = texture_asset->data.srv_index;
            } else {
                texture_cbuffer_idx = m_AssetManager->texture_resource_exists(resource_paths.texture_path)
                    ? m_AssetManager->get_texture_resource(resource_paths.texture_path).gpu_idx
                    : texture_asset->data.srv_index;
            }
            auto gpu_data = components::PerDrawConstants{
                .position_buffer_idx = m_VertexPosBuffers.back().srv_index,
                .normal_buffer_idx = m_VertexNormalBuffers.back().srv_index,
                .tangent_buffer_idx = should_add_tangent ? m_VertexTangentBuffers.back().srv_index : 0,
                .uv_buffer_idx = m_VertexUVBuffers.back().srv_index,
                .scene_cbuffer_idx = already_has_component
                    ? m_ECManager.engine_component<components::RenderComponent>(entity).per_draw_constants()->scene_cbuffer_idx
                    : m_TransformBuffers.back().cbv_index,
                .pass_cbuffer_idx = m_MainPassCB.cbv_index,
                .material_cbuffer_idx = material_cbuffer_idx,
                .texture_cbuffer_idx = texture_cbuffer_idx,
            };
            if (!should_allocate_mesh) {
                cpu_data = m_AssetManager->get_mesh_resource(mesh_asset->uuid).cpu_data;
                gpu_data = m_AssetManager->get_mesh_resource(mesh_asset->uuid).gpu_data;
                gpu_data.scene_cbuffer_idx = already_has_component
                    ? m_ECManager.engine_component<components::RenderComponent>(entity).per_draw_constants()->scene_cbuffer_idx
                    : m_TransformBuffers.back().cbv_index;
                gpu_data.material_cbuffer_idx = material_cbuffer_idx;
                gpu_data.texture_cbuffer_idx = texture_cbuffer_idx;
                cpu_data.transform_buffer_idx = already_has_component
                    ? m_ECManager.engine_component<components::RenderComponent>(entity).cpu_data().transform_buffer_idx
                    : static_cast<u32>(m_TransformBuffers.size() - 1);
            }
            m_AssetManager->load_mesh_resource(resource_paths.mesh_path, {cpu_data, gpu_data});
            const components::RenderComponent render_component{
                mesh_asset->uuid,
                texture_asset->uuid,
                material_asset->uuid,
                cpu_data,
                gpu_data,
                [this, entity](components::RenderComponent* component) {
                    if (component) {
                        const auto old_cpu_data = component->cpu_data();
                        const auto old_gpu_data = component->per_draw_constants();
                        // This happens after the mesh uuid is set to the new one, so we're getting the just assigned uuid
                        const auto mesh_uuid = component->mesh_uuid();
                        // The mesh we just assigned may not be allocated yet
                        const auto texture_uuid = component->texture_uuid();
                        const auto texture_path = m_AssetManager->get_texture_path(texture_uuid);
                        const auto mesh_path = m_AssetManager->get_mesh_path(mesh_uuid);
                        const auto material_uuid = component->material_uuid();
                        const auto material_path = m_AssetManager->get_material_path(material_uuid);
                        if (!m_AssetManager->mesh_resource_exists(mesh_path) || !m_AssetManager->material_resource_exists(material_path) ||
                            !m_AssetManager->texture_resource_exists(texture_path)) {
                            create_render_component(entity,
                                                    {.mesh_path = mesh_path, .texture_path = texture_path, .material_path = material_path});
                            return;
                        }
                        auto data = m_AssetManager->get_mesh_resource(mesh_path);
                        data.gpu_data.scene_cbuffer_idx = old_gpu_data->scene_cbuffer_idx;
                        component->cpu_data(data.cpu_data);
                        component->per_draw_constants(data.gpu_data);
                    }
                }};
            m_ECManager.add_engine_component<components::RenderComponent>(entity, render_component);
        }
        return stl::success;
    }

    void GameContext::on_window_resize() {
        if (m_GraphicsDevice) {
            auto resize_result =
                m_GraphicsDevice->resize_window(static_cast<u32>(m_ClientExtent->width), static_cast<u32>(m_ClientExtent->height));
            if (!resize_result) {
                CORE_ERROR("Failed to resize window: {}", resize_result.error().c_str());
            }
        }
    }

    void GameContext::update(f32 delta_time) {
        PROFILE_FUNCTION();
        m_PhysicsEngine->simulate(delta_time);
        auto& transforms = m_ECManager.engine_components<components::Transform>();
        for (auto& transform : transforms) {
            transform.update(transforms);
        }
        auto& entries = m_ECManager.entities();
        for (auto& entry : entries) {
            if (!entry.has_value())
                continue;
            auto& entity = entry->value;
            auto components = m_ECManager.components(entity);
            for (auto& component : components) {
                component->update(delta_time);
            }
        }
    }
} // namespace sf
