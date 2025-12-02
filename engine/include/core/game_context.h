#pragma once

#include "assets/asset_manager.h"
#include "components/ec_manager.h"
#include "core/core.h"
#include "math/math.h"
#include "physics/physics_engine.h"
#include "render/camera.h"
#include "render/material.h"
#include "render/render_backend.h"
#include "render/i_descriptor_heap.h"
#include "render/i_command_queue.h"
#include "render/i_context.h"
#include "render/i_memory_allocator.h"

namespace sf {

    struct ObjectConstants {
        sf::math::mat4 World = sf::math::mat4::identity();
    };

    namespace physics {
        class PhysicsEngine;
    }

    namespace render {
        class IGraphicsDevice;
    }

    class Application;
    class Window;
    struct ClientExtent;

    struct GameContextCreationDesc {
        ClientExtent* client_extent;
        void* window_handle; // Native window handle (HWND on Windows, X11 Window on Linux, etc.)
        stl::string mesh_registry_path = stl::string(mem::MemTag::Mesh, "mesh_registry.db");
        stl::string texture_registry_path = stl::string(mem::MemTag::Texture, "texture_registry.db");
        stl::string material_registry_path = stl::string(mem::MemTag::Material, "material_registry.db");
    };

    struct SFAPI RenderComponentResourcePaths {
        stl::string mesh_path;
        stl::string texture_path;
        stl::string material_path;
    };

    class SFAPI GameContext {
    public:
        GameContext(const GameContextCreationDesc& desc);
        virtual ~GameContext() {}
        void init();
        sf::stl::result<> create_render_component(Entity entity, const RenderComponentResourcePaths& resource_paths);
        virtual void on_window_resize();
        virtual void load_contents() = 0;
        virtual void update(f32 delta_time);
        virtual void render() = 0;

    protected:
        ClientExtent* m_ClientExtent;
        stl::unique_ptr<sf::render::IGraphicsDevice> m_GraphicsDevice;
        stl::unique_ptr<physics::PhysicsEngine> m_PhysicsEngine;

        // Rendering resources (owned by GameContext, created via device factories)
        stl::unique_ptr<render::IDescriptorHeap> m_CbvSrvUavHeap;
        stl::unique_ptr<render::IDescriptorHeap> m_SamplerHeap;
        stl::unique_ptr<render::ICommandQueue> m_DirectQueue;
        stl::unique_ptr<render::IGraphicsContext> m_GraphicsContext;
        stl::unique_ptr<render::IMemoryAllocator> m_MemoryAllocator;

        sf::stl::vector<sf::render::Buffer> m_RTIndexBuffers{};
        sf::stl::vector<sf::render::Buffer> m_VertexPosBuffers{};
        sf::stl::vector<sf::render::Buffer> m_VertexNormalBuffers{};
        sf::stl::vector<sf::render::Buffer> m_VertexTangentBuffers{};
        sf::stl::vector<sf::render::Buffer> m_VertexUVBuffers{};
        sf::stl::vector<sf::render::Buffer> m_TransformBuffers{};
        sf::render::Buffer m_MainPassCB{};
        stl::unique_ptr<assets::AssetManager> m_AssetManager;
        ECManager m_ECManager{};
        Camera m_MainCamera{};
    };
} // namespace sf
