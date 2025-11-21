#pragma once

#include "components/entity.h"
#include "core/core.h"
#include "widgets/widget.h"

namespace Sapfire {
	class ECManager;
}

namespace widgets {
	using AddRenderComponentCallback = sf::stl::function<void(sf::Entity, const sf::RenderComponentResourcePaths&)>;

	class SEntityInspector final : public IWidget {
	public:
		SEntityInspector(sf::ECManager* ec_manager, AddRenderComponentCallback callback);
		bool update(sf::f32 delta_time) override;
		void select_entity(sf::stl::optional<sf::Entity> maybe_entity);

	private:
		sf::ECManager& m_ECManager;
		sf::stl::optional<sf::Entity> m_SelectedEntity{};
		AddRenderComponentCallback m_AddRenderComponentCallback;
		bool m_ShowAddComponentContextMenu{false};
	};
} // namespace widgets
