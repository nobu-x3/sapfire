#include <QApplication>
#include <QMessageBox>

#include <SDL3/SDL.h>
#include <core/logger.h>

#include <memory/memory.h>
#include <render/render_backend.h>
#include "editor_context.h"
#include "main_window.h"
#include "project_launcher_dialog.h"
#include "project_manager.h"
#include "splash_screen.h"

int main(int argc, char** argv) {
    sf::Log::Init();
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return -1;
    }
    sf::mem::Budgets budgets{};
    sf::mem::MemoryManager memory_manager(budgets);
    sf::components::ComponentRegistry::process_queued_registrations();
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
    ProjectManager& project_manager = ProjectManager::instance();
    bool is_loaded = false;

    if (launcher.should_create_new_project()) {
        is_loaded = project_manager.create_project(launcher.selected_project_path(), launcher.new_project_name(), &splash);
    } else {
        is_loaded = project_manager.load_project(launcher.selected_project_path(), &splash);
    }

    if (!is_loaded) {
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
