#pragma once

#include "Sapfire.h"
#include "widget.h"

namespace widgets {
	class SMaterialInspector final : public IWidget {
	public:
		bool update(sf::f32 delta_time) override;
		inline void current_material(sf::assets::MaterialAsset* mat) { m_CurrentMaterial = mat; }

	private:
		sf::assets::MaterialAsset* m_CurrentMaterial{nullptr};
	};
} // namespace widgets