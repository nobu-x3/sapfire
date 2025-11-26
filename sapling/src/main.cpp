#include <QApplication>
#include <core/logger.h>
#include <memory/memory.h>
#include <render/render_backend.h>
#include "main_window.h"

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
    sf::mem::Budgets budgets{};
    sf::mem::MemoryManager memory_manager(budgets);
    QApplication app(argc, argv);
    app.setOrganizationName("Sapfire");
    app.setApplicationName("Sapling");
#ifdef _WIN32
    sf::render::RenderBackend::initialize(sf::render::RenderAPI::DX12);
#else
    sf::render::RenderBackend::initialize(sf::render::RenderAPI::Vulkan);
#endif
    SaplingMainWindow main_window;
    main_window.show();
    int result = app.exec();
    sf::render::RenderBackend::shutdown();
    return result;
}
