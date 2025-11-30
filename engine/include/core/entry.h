#include "engpch.h"

#include "components/component.h"
#include "memory/memory.h"

#include <exception>
#include "core/application.h"

#ifdef SF_PLATFORM_WINDOWS
#include <crtdbg.h>
#include "render/dx12/dx12_util.h"
#endif

extern sf::Application* sf::create_application();

int main(int argc, char* argv[]) {
#if (defined(DEBUG) | defined(_DEBUG)) && defined(SF_PLATFORM_WINDOWS)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    sf::mem::MemoryManager memory_manager{{}};
    try {
        sf::Log::Init();
        CORE_INFO("MemoryManager initialized with total budget: {} MB", sf::mem::Budgets{}.total() / (1024 * 1024));
        
        // Process all component registrations that were queued during static initialization
        sf::components::ComponentRegistry::process_queued_registrations();
        
        PROFILE_BEGIN_SESSION("Startup", "SapfireProfile_Startup.json");
        sf::Application* application = sf::create_application();
        PROFILE_END_SESSION();

        PROFILE_BEGIN_SESSION("Runtime", "SapfireProfile_Runtime.json");
        application->run();
        PROFILE_END_SESSION();
        PROFILE_BEGIN_SESSION("Shutdown", "SapfireProfile_Shutdown.json");
        delete application;
        PROFILE_END_SESSION();
    }
#ifdef SF_PLATFORM_WINDOWS
    catch (sf::render::dx12::DxException& e) {
        CORE_CRITICAL(sf::render::dx12::wstring_to_ansi(e.to_string()));
        MessageBoxW(nullptr, e.to_string().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
#endif
    catch (std::exception& e) {
        CORE_CRITICAL(e.what());
        return 0;
    }
    memory_manager.report(std::cout);
    return 0;
}
