#pragma once

#include <functional>
#include "core/core.h"
#include "events/event.h"

// Forward declare SDL types
struct SDL_Window;
union SDL_Event;

namespace sf {

    using EventCallbackFn = std::function<void(Event&)>;

    struct SFAPI WindowParams {
        u64 width = 0;
        u64 height = 0;
        stl::string name;
        EventCallbackFn callback{nullptr};
    };

    struct SFAPI WindowExtent {
        u64 width = 0;
        u64 height = 0;
    };

    class SFAPI Window {
    public:
        Window(const WindowParams& params);
        ~Window();
        void pump_messages();
        void event_callback(Event& event) { mf_EventCallback(event); }
        void is_resizing(bool val) { m_Resizing = val; }
        bool is_resizing() const { return m_Resizing; }
        void is_minimized(bool val) { m_Minimized = val; }
        bool is_minimized() const { return m_Minimized; }

        // Get native window handle
        SDL_Window* sdl_handle() { return m_Window; }
        void* native_handle(); // Returns HWND on Windows, X11 Window on Linux, etc.

    private:
        void handle_sdl_event(const SDL_Event& event);

        WindowExtent m_WindowExtent;
        EventCallbackFn mf_EventCallback;
        bool m_Resizing = false;
        bool m_Minimized = false;
        SDL_Window* m_Window = nullptr;
        u32 m_WindowID = 0;
    };
} // namespace sf
