#pragma once

#include "Sapfire.h"
#include "widgets/widget.h"

namespace widgets {
	enum class EAssetType : sf::u8 { Unknown, Mesh, Texture, Material };

	using event_fn = sf::stl::function<void()>;

	struct AssetDragAndDropPayload {
		sf::UUID uuid;
		EAssetType type;
	};

	class SAssetBrowser final : public IWidget {
	public:
		explicit SAssetBrowser(sf::stl::string_view name);
		static void register_asset_imported_events(event_fn fn);
		bool update(sf::f32 delta_time) override;
		void on_mouse_button_event(sf::MouseButtonEvent&) override;

	private:
		void execute_asset_imported_events();

	private:
		EAssetType m_CurrentAssetTypeFilter{EAssetType::Mesh};
		sf::stl::string m_WidgetName;
		bool m_ShowContextMenu{false};
	};
} // namespace widgets
