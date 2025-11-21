#pragma once

#include "Sapfire.h"
#include "widget.h"

namespace widgets {
	class SMaterialPreviewSettings final : public IWidget {
	public:
		explicit SMaterialPreviewSettings(sf::components::RenderComponent* rc);
		bool update(sf::f32 delta_time) override;

	private:
		sf::components::RenderComponent& m_RenderComponent;
	};
} // namespace widgets
