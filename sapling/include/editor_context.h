#pragma once

#include <render/i_graphics_device.h>
#include <assets/asset_manager.h>
#include <components/ec_manager.h>

struct SDL_Window;
class QApplication;

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
    bool m_Initialized{false};
};
