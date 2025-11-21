#pragma once

#include "Sapfire.h"
#include "subeditor.h"

namespace EWidgetOrder {
	enum ENUM {
		MaterialInspector = 0,
        SceneView,
        PreviewSettings,
		AssetBrowser
	};
}

class SMaterialEditor final : public SSubeditor {
public:
	SMaterialEditor(sf::assets::AssetManager* am, sf::render::IGraphicsDevice* device);
	bool update(sf::f32 delta_time) override;
	void draw_menu() override;
	void draw_dialogs();

private:
	sf::assets::AssetManager& m_AssetManager;
	sf::assets::MaterialAsset* m_OpenedMaterial{nullptr};
	sf::stl::unique_ptr<sf::ECManager> m_ECManager{nullptr};
	sf::Entity m_MaterialEntity;
};
