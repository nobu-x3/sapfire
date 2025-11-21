#pragma once

#include "Sapfire.h"

#include "core/stl/shared_ptr.h"
#include "subeditors/subeditor.h"
#include "widgets/scene_hierarchy.h"

namespace Sapfire {
	class ECManager;
}

using on_entity_selected_callback = sf::stl::function<void(sf::stl::optional<sf::Entity>)>;

namespace ELevelEditorWidgetOrder {
	enum ENUM { SceneHierarchy = 0, EntityInspector, AssetBrowser, SceneView };
};

class SLevelEditor final : public SSubeditor {
public:
	SLevelEditor(sf::render::IGraphicsDevice* gfx_device, sf::assets::AssetManager* am, const sf::stl::string& scene_path,
				 sf::stl::function<void()> asset_imported_callback);
	static SLevelEditor* level_editor();
	sf::assets::AssetManager& asset_manager() { return m_AssetManager; }

private:
	void on_entity_selected(sf::stl::optional<sf::Entity> entity);
	void on_mesh_added();
	void draw_menu() override;
	bool update(sf::f32 delta_time) override;
	void draw_open_scene_dialog();

private:
	sf::stl::unique_ptr<sf::ECManager> m_ECManager;
	sf::stl::vector<on_entity_selected_callback> m_EntitySelectedCallbacks;
	sf::assets::AssetManager& m_AssetManager;
	sf::stl::string m_CurrentSceneName{};

	static SLevelEditor* s_Instance;
};
