#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>

#include "Sapfire.h"

// Forward declarations
class SceneViewWidget;
class SceneHierarchyWidget;
class EntityInspectorWidget;
class AssetBrowserWidget;
class SQFileSystemModel;

class SaplingMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit SaplingMainWindow(QWidget* parent = nullptr);
    ~SaplingMainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_update_timer();
    void on_new_scene();
    void on_open_scene();
    void on_save_scene();
    void on_save_scene_as();
    void on_entity_selected(const sf::stl::optional<sf::Entity>& entity);

private:
    void create_menus();
    void create_docks();
    void read_settings();
    void write_settings();

private:
    SceneViewWidget* m_SceneView{nullptr};
    SceneHierarchyWidget* m_SceneHierarchy{nullptr};
    EntityInspectorWidget* m_EntityInspector{nullptr};
    AssetBrowserWidget* m_AssetBrowser{nullptr};
    QTimer* m_UpdateTimer{nullptr};
    QElapsedTimer m_DeltaTimer;
    QString m_CurrentScenePath;
};
