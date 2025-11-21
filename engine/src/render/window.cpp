#include "engpch.h"

#include <SDL3/SDL.h>
#include "events/application_event.h"
#include "events/input_event.h"
#include "events/keyboard_event.h"
#include "render/window.h"
#include "core/logger.h"

#ifdef SF_PLATFORM_WINDOWS
#include <SDL3/SDL_syswm.h>
#endif

namespace sf {
	Window::Window(const WindowParams& params) :
		m_WindowExtent({params.width, params.height}), mf_EventCallback(params.callback), m_Resizing(false) {

		// Initialize SDL video subsystem
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			CORE_CRITICAL("Failed to initialize SDL: {}", SDL_GetError());
			return;
		}

		// Create SDL window
		m_Window = SDL_CreateWindow(
			params.name.c_str(),
			static_cast<int>(params.width),
			static_cast<int>(params.height),
			SDL_WINDOW_RESIZABLE
		);

		if (!m_Window) {
			CORE_CRITICAL("Failed to create SDL window: {}", SDL_GetError());
			SDL_Quit();
			return;
		}

		m_WindowID = SDL_GetWindowID(m_Window);
		SDL_ShowWindow(m_Window);
	}

	Window::~Window() {
		if (m_Window) {
			SDL_DestroyWindow(m_Window);
			m_Window = nullptr;
		}
		SDL_Quit();
	}

	void* Window::native_handle() {
#ifdef SF_PLATFORM_WINDOWS
		SDL_PropertiesID props = SDL_GetWindowProperties(m_Window);
		return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(SF_PLATFORM_LINUX)
		return m_Window;
#else
		return m_Window;
#endif
	}

	void Window::pump_messages() {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			handle_sdl_event(event);
		}
	}

	void Window::handle_sdl_event(const SDL_Event& event) {
		switch (event.type) {
		case SDL_EVENT_QUIT:
			{
				WindowCloseEvent close_event;
				mf_EventCallback(close_event);
				break;
			}
		case SDL_EVENT_WINDOW_RESIZED:
			{
				if (event.window.windowID == m_WindowID) {
					int width, height;
					SDL_GetWindowSize(m_Window, &width, &height);
					m_WindowExtent.width = static_cast<u64>(width);
					m_WindowExtent.height = static_cast<u64>(height);
					WindowResizeEvent resize_event(width, height);
					if (width == 0 && height == 0) {
						m_Minimized = true;
					} else {
						m_Minimized = false;
					}
					mf_EventCallback(resize_event);
				}
				break;
			}
		case SDL_EVENT_WINDOW_MOVED:
			{
				// Window moved - could trigger resize finished event
				if (m_Resizing) {
					m_Resizing = false;
					WindowResizeFinishedEvent resize_finished;
					mf_EventCallback(resize_finished);
				}
				break;
			}
		case SDL_EVENT_WINDOW_MINIMIZED:
			{
				m_Minimized = true;
				WindowResizeFinishedEvent resize_finished;
				mf_EventCallback(resize_finished);
				break;
			}
		case SDL_EVENT_WINDOW_RESTORED:
		case SDL_EVENT_WINDOW_MAXIMIZED:
			{
				m_Minimized = false;
				WindowResizeFinishedEvent resize_finished;
				mf_EventCallback(resize_finished);
				break;
			}
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			{
				MouseButton button;
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					button = MouseButton::LMB;
					break;
				case SDL_BUTTON_RIGHT:
					button = MouseButton::RMB;
					break;
				case SDL_BUTTON_MIDDLE:
					button = MouseButton::MMB;
					break;
				default:
					return;
				}
				MouseButtonEvent mouse_event(button, true);
				mf_EventCallback(mouse_event);
				break;
			}
		case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				MouseButton button;
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					button = MouseButton::LMB;
					break;
				case SDL_BUTTON_RIGHT:
					button = MouseButton::RMB;
					break;
				case SDL_BUTTON_MIDDLE:
					button = MouseButton::MMB;
					break;
				default:
					return;
				}
				MouseButtonEvent mouse_event(button, false);
				mf_EventCallback(mouse_event);
				break;
			}
		case SDL_EVENT_MOUSE_MOTION:
			{
				MouseMovedEvent mouse_moved(
					static_cast<i32>(event.motion.x),
					static_cast<i32>(event.motion.y)
				);
				mf_EventCallback(mouse_moved);
				break;
			}
		case SDL_EVENT_KEY_DOWN:
			{
				KeyPressedEvent key_event(event.key.key);
				mf_EventCallback(key_event);
				break;
			}
		case SDL_EVENT_KEY_UP:
			{
				KeyReleasedEvent key_event(event.key.key);
				mf_EventCallback(key_event);
				break;
			}
		}
	}
} // namespace sf
