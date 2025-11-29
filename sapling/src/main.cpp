#include <QApplication>

#include <SDL3/SDL.h>
#include <core/logger.h>

#include <memory/memory.h>
#include <render/render_backend.h>
#include "main_window.h"
#include "editor_context.h"

namespace sf {
    std::unordered_map<const char*, components::ComponentType> components::ComponentRegistry::s_ComponentTypes = {};
    std::unordered_map<components::ComponentType, const char*> components::ComponentRegistry::s_ComponentTypeNameMap = {};
    std::unordered_map<const char*, stl::shared_ptr<components::IComponentList>> components::ComponentRegistry::s_EngineComponentLists = {};
    std::unordered_map<const char*, stl::shared_ptr<components::CustomComponentList>>
        components::ComponentRegistry::s_CustomComponentLists{};
    components::ComponentType components::ComponentRegistry::s_NextComponentTypeNumber = 0;
} // namespace sf

int main(int argc, char** argv) {
    sf::Log::Init();
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return -1;
    }
    sf::mem::Budgets budgets{};
    sf::mem::MemoryManager memory_manager(budgets);
    QApplication app(argc, argv);
    app.setOrganizationName("Sapfire");
    app.setApplicationName("Sapling");
    EditorContext::set_theme(app);
#ifdef _WIN32
    sf::render::RenderBackend::initialize(sf::render::RenderAPI::DX12);
#else
    sf::render::RenderBackend::initialize(sf::render::RenderAPI::Vulkan);
#endif
    int result = 0;
    {
        SaplingMainWindow main_window;
        main_window.show();
        result = app.exec();
    }
    sf::render::RenderBackend::shutdown();
    SDL_Quit();
    return result;
}
