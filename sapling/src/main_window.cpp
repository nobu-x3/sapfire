#include "main_window.h"
#include "editor_context.h"
#include "memory/memory.h"
#include "project_manager.h"
#include "widgets/asset_browser_widget.h"
#include "widgets/entity_inspector_widget.h"
#include "widgets/file_system_model.h"
#include "widgets/scene_hierarchy_widget.h"
#include "widgets/scene_view_widget.h"

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <SDL3/SDL.h>

SaplingMainWindow::SaplingMainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Sapling Editor");
    resize(1600, 900);
    create_menus();
    create_docks();
    read_settings();
    // Setup update timer (60 FPS)
    m_UpdateTimer = new QTimer(this);
    connect(m_UpdateTimer, &QTimer::timeout, this, &SaplingMainWindow::on_update_timer);
    m_UpdateTimer->start(16); // ~60 FPS
    m_DeltaTimer.start();
}

SaplingMainWindow::~SaplingMainWindow() { write_settings(); }

void SaplingMainWindow::create_menus() {
    auto* file_menu = menuBar()->addMenu(tr("&File"));
    auto* new_scene_action = file_menu->addAction(tr("&New Scene"));
    new_scene_action->setShortcut(QKeySequence::New);
    connect(new_scene_action, &QAction::triggered, this, &SaplingMainWindow::on_new_scene);
    auto* open_scene_action = file_menu->addAction(tr("&Open Scene..."));
    open_scene_action->setShortcut(QKeySequence::Open);
    connect(open_scene_action, &QAction::triggered, this, &SaplingMainWindow::on_open_scene);
    file_menu->addSeparator();
    auto* save_scene_action = file_menu->addAction(tr("&Save Scene"));
    save_scene_action->setShortcut(QKeySequence::Save);
    connect(save_scene_action, &QAction::triggered, this, &SaplingMainWindow::on_save_scene);
    auto* save_scene_as_action = file_menu->addAction(tr("Save Scene &As..."));
    save_scene_as_action->setShortcut(QKeySequence::SaveAs);
    connect(save_scene_as_action, &QAction::triggered, this, &SaplingMainWindow::on_save_scene_as);
    file_menu->addSeparator();
    auto* exit_action = file_menu->addAction(tr("E&xit"));
    exit_action->setShortcut(QKeySequence::Quit);
    connect(exit_action, &QAction::triggered, this, &QWidget::close);
    auto* edit_menu = menuBar()->addMenu(tr("&Edit"));
    auto* undo_action = edit_menu->addAction(tr("&Undo"));
    undo_action->setShortcut(QKeySequence::Undo);
    undo_action->setEnabled(false); // TODO: Implement undo/redo
    auto* redo_action = edit_menu->addAction(tr("&Redo"));
    redo_action->setShortcut(QKeySequence::Redo);
    redo_action->setEnabled(false); // TODO: Implement undo/redo
    auto* view_menu = menuBar()->addMenu(tr("&View"));
    auto* help_menu = menuBar()->addMenu(tr("&Help"));
    auto* about_action = help_menu->addAction(tr("&About Sapling"));
    connect(about_action, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("About Sapling"), tr("Sapling Editor\n\nA game editor built with Qt and Sapfire Engine."));
    });
}

void SaplingMainWindow::create_docks() {
    m_SceneView = new SceneViewWidget(this);
    setCentralWidget(m_SceneView);
    auto* hierarchy_dock = new QDockWidget(tr("Scene Hierarchy"), this);
    hierarchy_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_SceneHierarchy = new SceneHierarchyWidget(hierarchy_dock);
    hierarchy_dock->setWidget(m_SceneHierarchy);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchy_dock);
    menuBar()->actions()[2]->menu()->addAction(hierarchy_dock->toggleViewAction());
    connect(m_SceneHierarchy, &SceneHierarchyWidget::entity_selected, this, &SaplingMainWindow::on_entity_selected);
    auto* inspector_dock = new QDockWidget(tr("Entity Inspector"), this);
    inspector_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_EntityInspector = new EntityInspectorWidget(inspector_dock);
    inspector_dock->setWidget(m_EntityInspector);
    // When components change in the inspector (eg. name changes), refresh the hierarchy
    connect(m_EntityInspector, &EntityInspectorWidget::entity_component_changed, [this](sf::Entity) {
        if (m_SceneHierarchy)
            m_SceneHierarchy->refresh();
    });
    addDockWidget(Qt::RightDockWidgetArea, inspector_dock);
    menuBar()->actions()[2]->menu()->addAction(inspector_dock->toggleViewAction());
    auto* asset_dock = new QDockWidget(tr("Asset Browser"), this);
    asset_dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_AssetBrowser = new AssetBrowserWidget(&SQFileSystemModel::instance(), asset_dock);
    asset_dock->setWidget(m_AssetBrowser);
    addDockWidget(Qt::BottomDockWidgetArea, asset_dock);
    menuBar()->actions()[2]->menu()->addAction(asset_dock->toggleViewAction());
}

void SaplingMainWindow::on_update_timer() {
    // Process SDL events to keep the hidden SDL window responsive (critical for Wayland)
    SDL_PumpEvents();
    sf::f32 delta_time = m_DeltaTimer.restart() / 1000.0f;
    if (m_SceneView) {
        m_SceneView->update_frame(delta_time);
    }
    sf::mem::MemoryManager::get()->reset(sf::mem::MemTag::Temp);
}

void SaplingMainWindow::on_new_scene() {
    // TODO: Prompt to save current scene if modified
    auto* ctx = &EditorContext::instance();
    if (ctx->ec_manager()) {
        ctx->ec_manager()->reset();
    }
    m_CurrentScenePath.clear();
    setWindowTitle("Sapling Editor - New Scene*");
    if (m_SceneHierarchy) {
        m_SceneHierarchy->refresh();
    }
}

void SaplingMainWindow::on_open_scene() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Open Scene"),
                                                    QString(), // TODO: Use project directory
                                                    tr("Scene Files (*.scene);;All Files (*)"));
    if (filename.isEmpty()) {
        return;
    }
    // TODO: Load scene using SceneWriter
    m_CurrentScenePath = filename;
    setWindowTitle(QString("Sapling Editor - %1").arg(QFileInfo(filename).fileName()));
    if (m_SceneHierarchy) {
        m_SceneHierarchy->refresh();
    }
}

void SaplingMainWindow::on_save_scene() {
    if (m_CurrentScenePath.isEmpty()) {
        on_save_scene_as();
        return;
    }
    // TODO: Save scene using SceneWriter
    setWindowTitle(QString("Sapling Editor - %1").arg(QFileInfo(m_CurrentScenePath).fileName()));
}

void SaplingMainWindow::on_save_scene_as() {
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Scene As"),
                                                    QString(), // TODO: Use project directory
                                                    tr("Scene Files (*.scene);;All Files (*)"));
    if (filename.isEmpty()) {
        return;
    }
    m_CurrentScenePath = filename;
    on_save_scene();
}

void SaplingMainWindow::on_entity_selected(const sf::stl::optional<sf::Entity>& entity) {
    if (m_EntityInspector) {
        m_EntityInspector->set_entity(entity);
    }
}

void SaplingMainWindow::read_settings() {
    QSettings settings("Sapfire", "Sapling");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void SaplingMainWindow::write_settings() {
    QSettings settings("Sapfire", "Sapling");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void SaplingMainWindow::closeEvent(QCloseEvent* event) {
    // TODO: Prompt to save scene if modified
    write_settings();
    event->accept();
}
