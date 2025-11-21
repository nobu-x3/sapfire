#include "widgets/material_preview_settings.h"
#include "imgui.h"
#include "rtti_drawer.h"

using namespace sf;

namespace widgets {
	SMaterialPreviewSettings::SMaterialPreviewSettings(sf::components::RenderComponent* rc) : m_RenderComponent(*rc) {}

	bool SMaterialPreviewSettings::update(sf::f32 delta_time) {
		if (!m_IsVisible)
			return true;
		if (ImGui::Begin("Preview Settings")) {
			draw_rtti<components::RenderComponent>(&m_RenderComponent);
		}
		ImGui::End();
		return true;
	}
} // namespace widgets
