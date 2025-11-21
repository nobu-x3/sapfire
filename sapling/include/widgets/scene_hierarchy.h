#pragma once

#include "components/entity.h"
#include "core/core.h"
#include "widgets/widget.h"

namespace Sapfire {
	class ECManager;
}

namespace widgets {

	using EntitySelectedCallback = sf::stl::function<void(sf::stl::optional<sf::Entity>)>;

	class SSceneHierarchy final : public IWidget {
	public:
		SSceneHierarchy(sf::ECManager* ec_manager, EntitySelectedCallback callback);
		bool update(sf::f32 delta_time) override;

		void on_mouse_button_event(sf::MouseButtonEvent&) override;

	private:
		void build_tree_for_entity(const sf::stl::vector<sf::stl::generational_index>& indices, sf::u32 valid_parent_index);
		void draw_entity_context_menu(const sf::Entity& parent_entity);

	private:
		sf::ECManager& m_ECManager;
		bool m_ShowContextMenu{false};
		bool m_SelectEntity{false};
		EntitySelectedCallback m_EntitySelectedCallback;
		sf::stl::optional<sf::Entity> m_SelectedEntity{};
	};
} // namespace widgets
