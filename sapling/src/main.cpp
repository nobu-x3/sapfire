#include <QApplication>
#include <QMessageBox>

#include <SDL3/SDL.h>
#include <core/logger.h>

#include <memory/memory.h>
#include <render/render_backend.h>
#include "main_window.h"
#include "editor_context.h"
#include "project_launcher_dialog.h"
#include "splash_screen.h"
#include "project_manager.h"

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

    // Show project launcher
    ProjectLauncherDialog launcher;
    if (launcher.exec() != QDialog::Accepted) {
        SDL_Quit();
        return 0; // User cancelled
    }

    // Show splash screen
    SplashScreen splash;
    splash.show();
    splash.set_status("Initializing engine...");
    splash.set_progress(5);

    // Initialize render backend
#ifdef _WIN32
    sf::render::RenderBackend::initialize(sf::render::RenderAPI::DX12);
#else
    sf::render::RenderBackend::initialize(sf::render::RenderAPI::Vulkan);
#endif

    splash.set_status("Loading project...");
    splash.set_progress(15);

    // Handle project creation or loading
    ProjectManager projectManager;
    bool projectLoaded = false;

    if (launcher.should_create_new_project()) {
        projectLoaded = projectManager.create_project(
            launcher.selected_project_path(),
            launcher.new_project_name(),
            &splash
        );
    } else {
        projectLoaded = projectManager.load_project(
            launcher.selected_project_path(),
            &splash
        );
    }

    if (!projectLoaded) {
        splash.close();
        QMessageBox::critical(nullptr, "Error", "Failed to load project");
        sf::render::RenderBackend::shutdown();
        SDL_Quit();
        return -1;
    }

    splash.set_status("Opening editor...");
    splash.set_progress(95);

    int result = 0;
    {
        SaplingMainWindow main_window;
        splash.finish();
        main_window.show();
        result = app.exec();
    }
    sf::render::RenderBackend::shutdown();
    SDL_Quit();
    return result;
}
