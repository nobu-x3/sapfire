#pragma once

#include <assets/asset_manager.h>
#include <components/ec_manager.h>
#include <render/i_command_queue.h>
#include <render/i_context.h>
#include <render/i_descriptor_heap.h>
#include <render/i_graphics_device.h>
#include <render/i_memory_allocator.h>

struct SDL_Window;
class QApplication;
class ProjectManager;

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
    sf::render::IDescriptorHeap* cbv_srv_uav_heap() { return m_CbvSrvUavHeap.get(); }
    sf::render::IDescriptorHeap* sampler_heap() { return m_SamplerHeap.get(); }
    sf::render::ICommandQueue* direct_queue() { return m_DirectQueue.get(); }
    sf::render::IGraphicsContext* graphics_context() { return m_GraphicsContext.get(); }
    sf::render::IMemoryAllocator* memory_allocator() { return m_MemoryAllocator.get(); }

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
    sf::stl::unique_ptr<sf::render::IDescriptorHeap> m_CbvSrvUavHeap;
    sf::stl::unique_ptr<sf::render::IDescriptorHeap> m_SamplerHeap;
    sf::stl::unique_ptr<sf::render::ICommandQueue> m_DirectQueue;
    sf::stl::unique_ptr<sf::render::IGraphicsContext> m_GraphicsContext;
    sf::stl::unique_ptr<sf::render::IMemoryAllocator> m_MemoryAllocator;

    bool m_Initialized{false};
};
