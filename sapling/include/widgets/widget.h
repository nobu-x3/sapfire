#pragma once

#include "events/input_event.h"
#include "render/graphics_context.h"

class IWidget {
public:
	virtual ~IWidget(){};
	virtual bool update(sf::f32 delta_time) = 0;
	virtual void render(sf::d3d::GraphicsContext&) {};
	virtual void on_mouse_button_event(sf::MouseButtonEvent&) {}
	virtual void on_mouse_moved_event(sf::MouseMovedEvent&) {}
	inline bool is_visible() const { return m_IsVisible; }
	inline void set_visible(bool val) { m_IsVisible = val; }

protected:
	bool m_IsVisible{true};
};
