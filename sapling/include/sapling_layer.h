#pragma once

#include "Sapfire.h"
#include "subeditors/subeditor.h"

namespace widgets {
	class SSceneHierarchy;
}

namespace ESubeditor {
	enum TYPE {
		LevelEditor = 0,
		MaterialEditor = 1,
		COUNT,
	};
}

static sf::stl::array<sf::stl::string, ESubeditor::COUNT> g_SubeditorNames = {"Level Editor", "Material Editor"};

class SaplingLayer final : public sf::Layer {
public:
	SaplingLayer();
	~SaplingLayer() final = default;
	void on_attach() final;
	void on_detach() final;
	void on_update(sf::f32 delta_time) final;
	void on_event(sf::Event& e) final;
	void on_render() final;

	sf::assets::AssetManager* asset_manager() { return m_AssetManager.get(); }
	sf::render::IGraphicsDevice* gfx_device() { return m_GraphicsDevice.get(); }

private:
	void serialize();
	void draw_menu_bar();
	void update_pass_cb(sf::f32 delta_time);
	bool on_window_resize_finished(sf::WindowResizeFinishedEvent&);
	bool on_window_resize(sf::WindowResizeEvent& e);
	bool is_subeditor_active(ESubeditor::TYPE type);
	SSubeditor* subeditor_factory(ESubeditor::TYPE type, bool is_callback = false);
	sf::stl::bitset<ESubeditor::COUNT> m_ShouldExecuteSubeditorCreationCallback{};

private:
	sf::stl::array<sf::stl::unique_ptr<SSubeditor>, 2> m_Subeditors{};
	sf::stl::unique_ptr<sf::render::IGraphicsDevice> m_GraphicsDevice{};
	sf::stl::unique_ptr<sf::assets::AssetManager> m_AssetManager{};
	sf::d3d::PipelineState m_PipelineState{};
	sf::render::Texture m_DepthTexture{};
	sf::u8 m_ActiveSubeditors{0};
	sf::stl::string m_ProjectPath;
	sf::stl::string m_ProjectName;
};