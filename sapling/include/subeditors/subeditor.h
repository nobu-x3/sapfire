#pragma once

#include "Sapfire.h"
#include "widgets/widget.h"

class SSubeditor {
public:
	SSubeditor(sf::stl::string_view editor_name);
	virtual ~SSubeditor() = default;
	virtual bool update(sf::f32 delta_time);
	virtual void render(sf::d3d::GraphicsContext&);
	virtual void draw_menu() {};
	inline const sf::stl::string& name() const { return m_Name; }

protected:
	sf::stl::vector<sf::stl::unique_ptr<IWidget>> m_Widgets;
	sf::stl::string m_Name;
};
