#include "engpch.h"

#include "core/platform.h"
#include "math/math.h"
#ifdef SF_PLATFORM_WINDOWS
#include <windows.h>
#include <winuser.h>
#endif
#include "core/core.h"
#include "core/input.h"
#include "tools/profiling.h"

namespace sf::input {
    constexpr f32 MOUSE_THRESHOLD = 0.001f;
#define abs(x) (x >= 0 ? x : -x)
    std::unique_ptr<InputSystem> InputSystem::s_Instance{nullptr};
    InputComponent::InputComponent() {
        if (!InputSystem::is_init()) {
            InputSystem::init();
        }
        InputSystem::register_component(this);
    }
    InputComponent::~InputComponent() {
        if (InputSystem::is_init()) {
            InputSystem::unregister_component(this);
        }
    }
    InputComponent::InputComponent(const InputComponent& other) :
        input_axis(other.input_axis), sens_v(other.sens_v), sens_h(other.sens_h), mouse_delta_x(other.mouse_delta_x),
        mouse_delta_y(other.mouse_delta_y), mouse_state(other.mouse_state) {
        if (!InputSystem::is_init()) {
            InputSystem::init();
        }
        InputSystem::register_component(this);
    }
    InputComponent::InputComponent(InputComponent&& other) noexcept :
        input_axis(other.input_axis), sens_v(other.sens_v), sens_h(other.sens_h), mouse_delta_x(other.mouse_delta_x),
        mouse_delta_y(other.mouse_delta_y), mouse_state(other.mouse_state) {
        if (!InputSystem::is_init()) {
            InputSystem::init();
        }
        InputSystem::register_component(this);
    }
    InputComponent& InputComponent::operator=(const InputComponent& other) {
        if (this != &other) {
            input_axis = other.input_axis;
            sens_v = other.sens_v;
            sens_h = other.sens_h;
            mouse_delta_x = other.mouse_delta_x;
            mouse_delta_y = other.mouse_delta_y;
            mouse_state = other.mouse_state;
        }
        return *this;
    }
    InputComponent& InputComponent::operator=(InputComponent&& other) noexcept {
        if (this != &other) {
            input_axis = other.input_axis;
            sens_v = other.sens_v;
            sens_h = other.sens_h;
            mouse_delta_x = other.mouse_delta_x;
            mouse_delta_y = other.mouse_delta_y;
            mouse_state = other.mouse_state;
        }
        return *this;
    }

    void InputSystem::init() {
        if (!s_Instance)
            s_Instance = std::make_unique<InputSystem>();
    }

    bool InputSystem::is_init() { return s_Instance != nullptr; }

    void InputSystem::register_component(InputComponent* component) { s_Instance->m_InputComponents.push_back(component); }

    void InputSystem::unregister_component(InputComponent* component) {
        auto& components = s_Instance->m_InputComponents;
        auto it = std::find_if(components.begin(), components.end(), [component](const InputComponent* comp) { return comp == component; });
        if (it != components.end()) {
            components.erase(it);
        }
    }

    void InputSystem::mouse_position(MousePosition pos) {
        s_Instance->m_LastMousePosition = s_Instance->m_MousePosition;
        s_Instance->m_MousePosition = pos;
    }

    void InputSystem::mouse_state(MouseState state) {
        s_Instance->m_LastMouseState = s_Instance->m_MouseState;
        s_Instance->m_MouseState = state;
    }

    void InputSystem::keyboard_state(u64 state) { s_Instance->m_KeyboardState |= state; }
    MousePosition InputSystem::mouse_position() { return s_Instance->m_MousePosition; }
    MouseState InputSystem::mouse_state() { return s_Instance->m_MouseState; }
    u64 InputSystem::keyboard_state() { return s_Instance->m_KeyboardState; }

    bool InputSystem::is_key_down(i32 scan_code) {
#ifdef SF_PLATFORM_WINDOWS
        return GetAsyncKeyState(scan_code);
#else
        // TODO: Implement SDL3-based keyboard input for Linux/macOS
        return false;
#endif
    }

    void InputSystem::update() {
        PROFILE_FUNCTION();
        for (auto* comp : s_Instance->m_InputComponents) {
            if (!comp) {
                CORE_ERROR("Something went wrong with the input system - nullptr component registered");
                continue;
            }
            comp->mouse_state = s_Instance->m_MouseState;
            comp->mouse_delta_x = (s_Instance->m_MousePosition.x - s_Instance->m_LastMousePosition.x) * comp->sens_h;
            if (abs(comp->mouse_delta_x) < MOUSE_THRESHOLD) {
                comp->mouse_delta_x = 0.f;
            }
            comp->mouse_delta_y = (s_Instance->m_MousePosition.y - s_Instance->m_LastMousePosition.y) * comp->sens_v;
            if (abs(comp->mouse_delta_y) < MOUSE_THRESHOLD) {
                comp->mouse_delta_y = 0.f;
            }
            sf::math::vec4 input_axis{0.f, 0.f, 0.f, 0.f};
#ifdef SF_PLATFORM_WINDOWS
            // TODO: Replace with SDL3 keyboard state polling for cross-platform support
            if (is_key_down(VK_UP)) {
                input_axis.x += 1;
            }
            if (is_key_down(VK_DOWN)) {
                input_axis.x -= 1;
            }
            if (is_key_down(VK_RIGHT)) {
                input_axis.y += 1;
            }
            if (is_key_down(VK_LEFT)) {
                input_axis.y -= 1;
            }
#endif
            comp->input_axis = input_axis;
        }
        mouse_position(s_Instance->m_MousePosition);
    }
} // namespace sf::input
