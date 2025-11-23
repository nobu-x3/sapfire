#pragma once


#pragma warning(push, 0)
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include "core/core.h"

namespace spdlog {
	class logger;
}

namespace sf {
	class SFAPI Log {
	public:
		static void Init();

		static stl::shared_ptr<spdlog::logger>& get_core_logger();
		static stl::shared_ptr<spdlog::logger>& get_client_logger();

	private:
		static stl::shared_ptr<spdlog::logger> s_CoreLogger;
		static stl::shared_ptr<spdlog::logger> s_ClientLogger;
	};
} // namespace sf

// Core log macros
#define CORE_TRACE(...) ::sf::Log::get_core_logger()->trace(__VA_ARGS__)
#define CORE_INFO(...) ::sf::Log::get_core_logger()->info(__VA_ARGS__)
#define CORE_WARN(...) ::sf::Log::get_core_logger()->warn(__VA_ARGS__)
#define CORE_ERROR(...) ::sf::Log::get_core_logger()->error(__VA_ARGS__)
#define CORE_CRITICAL(...) ::sf::Log::get_core_logger()->critical(__VA_ARGS__)

// Client log macros
#define CLIENT_TRACE(...) ::sf::Log::get_client_logger()->trace(__VA_ARGS__)
#define CLIENT_INFO(...) ::sf::Log::get_client_logger()->info(__VA_ARGS__)
#define CLIENT_WARN(...) ::sf::Log::get_client_logger()->warn(__VA_ARGS__)
#define CLIENT_ERROR(...) ::sf::Log::get_client_logger()->error(__VA_ARGS__)
#define CLIENT_CRITICAL(...) ::sf::Log::get_client_logger()->critical(__VA_ARGS__)
