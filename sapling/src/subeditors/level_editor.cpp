#include "subeditors/level_editor.h"
#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "subeditors/subeditor.h"
#include "widgets/asset_browser.h"
#include "widgets/entity_inspector.h"
#include "widgets/scene_hierarchy.h"
#include "widgets/scene_view.h"

using namespace sf;

SLevelEditor* SLevelEditor::s_Instance{nullptr};
SLevelEditor* SLevelEditor::level_editor() { return s_Instance; }

SLevelEditor::SLevelEditor(sf::render::IGraphicsDevice* gfx_device, sf::assets::AssetManager* am,
						   const sf::stl::string& scene_path, sf::stl::function<void()> asset_imported_callback) :
	SSubeditor("Level Editor"),
	m_ECManager(stl::make_unique<ECManager>(mem::ENUM::Editor)), m_AssetManager(*am) {
	s_Instance = this;
	m_Widgets.push_back(
		stl::make_unique<widgets::SSceneHierarchy>(mem::ENUM::Editor, m_ECManager.get(), BIND_EVENT_FN(SLevelEditor::on_entity_selected)));
	m_Widgets.push_back(stl::make_unique<widgets::SEntityInspector>(
		mem::ENUM::Editor, m_ECManager.get(), [&](sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths) {
			auto scene_view = static_cast<widgets::SSceneView*>(m_Widgets[ELevelEditorWidgetOrder::SceneView].get());
			scene_view->add_render_component(entity, resource_paths);
		}));
	auto* entity_inspector = static_cast<widgets::SEntityInspector*>(m_Widgets[ELevelEditorWidgetOrder::EntityInspector].get());
	m_EntitySelectedCallbacks.push_back(BIND_EVENT_FN_FOR_OBJ(entity_inspector, widgets::SEntityInspector::select_entity));
	m_Widgets.emplace_back(stl::make_unique<widgets::SAssetBrowser>(mem::ENUM::Editor, "Asset Browser"));
	auto asset_browser = static_cast<widgets::SAssetBrowser*>(m_Widgets[ELevelEditorWidgetOrder::AssetBrowser].get());
	asset_browser->register_asset_imported_events(asset_imported_callback);
	m_Widgets.push_back(stl::make_unique<widgets::SSceneView>(mem::ENUM::Editor, "Scene View", m_ECManager.get(), gfx_device));
	if (!scene_path.empty()) {
		assets::SceneWriter writer{m_ECManager.get(), &m_AssetManager};
		writer.deserealize(scene_path, [&](sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths) {
			auto scene_view = static_cast<widgets::SSceneView*>(m_Widgets[ELevelEditorWidgetOrder::SceneView].get());
			scene_view->add_render_component(entity, resource_paths);
		});
	}
}

void SLevelEditor::on_entity_selected(sf::stl::optional<sf::Entity> entity) {
	for (auto& f : m_EntitySelectedCallbacks) {
		f(entity);
	}
}

void SLevelEditor::draw_menu() {
	if (ImGui::BeginMenu("Level Editor")) {
		if (ImGui::MenuItem("New scene...")) {
			m_ECManager->reset();
			m_CurrentSceneName = "";
		}
		if (ImGui::MenuItem("Open scene...")) {
			IGFD::FileDialogConfig config{};
			config.path = sf::fs::FileSystem::root_directory();
			ImGuiFileDialog::Instance()->OpenDialog("OpenSceneFileDlg", "Open scene", ".scene", config);
		}
		ImGui::EndMenu();
	}
}

bool SLevelEditor::update(sf::f32 delta_time) {
	bool ret_val = SSubeditor::update(delta_time);
	draw_open_scene_dialog();
	return ret_val;
}

void SLevelEditor::draw_open_scene_dialog() {
	if (ImGuiFileDialog::Instance()->Display("OpenSceneFileDlg")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			stl::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
			if (!filepath.empty()) {
				m_CurrentSceneName = filepath;
				m_ECManager->reset();
				assets::SceneWriter writer{m_ECManager.get(), &m_AssetManager};
				writer.deserealize(filepath, [&](sf::Entity entity, const sf::RenderComponentResourcePaths& resource_paths) {
					auto widget = m_Widgets[ELevelEditorWidgetOrder::SceneView].get();
					static_cast<widgets::SSceneView*>(widget)->add_render_component(entity, resource_paths);
				});
			}
		}
		ImGuiFileDialog::Instance()->Close();
	}
}
