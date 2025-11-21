#pragma once

#include "core/core.h"
#include "events/event.h"

namespace sf {

	enum class MouseButton { LMB, RMB, MMB };

	class MouseButtonEvent final : public Event {
	public:
		MouseButtonEvent(MouseButton button, bool isDown) : m_Button(button), m_IsDown(isDown) {}
		bool is_down() const { return m_IsDown; }
		MouseButton button() const { return m_Button; }

		stl::string to_string() const final {
			stl::stringstream ss;
			ss << "MouseButtonEvent: ";
			switch (m_Button) {
			case MouseButton::RMB:
				ss << "RMB";
				break;
			case MouseButton::MMB:
				ss << "MMB";
				break;
			default:
				ss << "LMB";
				break;
			}
			ss << ", is down: " << m_IsDown;
			return ss.str();
		}
		EVENT_CLASS_TYPE(MouseButton)
		EVENT_CLASS_CATEGORY(EventCategory::EventCategoryMouseButton)
	private:
		MouseButton m_Button;
		bool m_IsDown;
	};

	class MouseMovedEvent final : public Event {
	public:
		MouseMovedEvent(i32 new_x, i32 new_y) : m_X(new_x), m_Y(new_y) {}
		i32 x() const { return m_X; }
		i32 y() const { return m_Y; }
		stl::string to_string() const final {
			stl::stringstream ss;
			ss << "MouseMovedEvent: delta x: " << m_X << ", delta y: " << m_Y;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategory::EventCategoryMouse)
	private:
		i32 m_X;
		i32 m_Y;
	};
} // namespace sf
