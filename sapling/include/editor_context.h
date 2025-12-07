#pragma once

#include <assets/asset_manager.h>
#include <components/ec_manager.h>
#include <render/bindless_resource_registry.h>
#include <render/i_command_queue.h>
#include <render/i_context.h>
#include <render/i_descriptor_pool.h>
#include <render/i_graphics_device.h>
#include <render/i_memory_allocator.h>

struct SDL_Window;
class QApplication;
class ProjectManager;

namespace sf {
    struct RenderComponentResourcePaths;
}

// Singleton that manages shared editor state and engine systems
class EditorContext {
public:
    static EditorContext& instance() {
        static EditorContext s_Instance;
        return s_Instance;
    }

    static void set_theme(QApplication& app);

    void initialize(SDL_Window* sdl_window, sf::u32 width, sf::u32 height);

    void shutdown();

    sf::render::IGraphicsDevice* graphics_device() { return m_GraphicsDevice.get(); }
    sf::assets::AssetManager* asset_manager() { return m_AssetManager.get(); }
    sf::ECManager* ec_manager() { return m_ECManager.get(); }
    ProjectManager* project_manager();

    // Resource accessors
    sf::render::BindlessResourceRegistry* bindless_registry() { return m_BindlessRegistry.get(); }
    sf::render::ICommandQueue* direct_queue() { return m_DirectQueue.get(); }
    sf::render::IGraphicsContext* graphics_context() { return m_GraphicsContext.get(); }
    sf::render::IMemoryAllocator* memory_allocator() { return m_MemoryAllocator.get(); }
    sf::stl::span<const sf::render::DescriptorSetLayout> bindless_descriptor_set_layouts() const { return m_BindlessLayouts; }

    bool is_initialized() const { return m_Initialized; }

private:
    EditorContext() = default;
    ~EditorContext() { shutdown(); }

    EditorContext(const EditorContext&) = delete;
    EditorContext& operator=(const EditorContext&) = delete;

private:
    sf::stl::unique_ptr<sf::render::IGraphicsDevice> m_GraphicsDevice;
    sf::stl::unique_ptr<sf::assets::AssetManager> m_AssetManager;
    sf::stl::unique_ptr<sf::ECManager> m_ECManager;

    // Rendering resources (owned by EditorContext, created via device factories)
    sf::stl::unique_ptr<sf::render::IDescriptorPool> m_DescriptorPool;
    sf::stl::vector<sf::render::DescriptorSetLayout> m_BindlessLayouts{sf::mem::MemTag::Render};
    sf::stl::unique_ptr<sf::render::BindlessResourceRegistry> m_BindlessRegistry;
    sf::stl::unique_ptr<sf::render::ICommandQueue> m_DirectQueue;
    sf::stl::unique_ptr<sf::render::IGraphicsContext> m_GraphicsContext;
    sf::stl::unique_ptr<sf::render::IMemoryAllocator> m_MemoryAllocator;

    bool m_Initialized{false};
};
