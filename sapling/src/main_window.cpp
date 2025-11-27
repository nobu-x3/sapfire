#include "main_window.h"
#include "editor_context.h"
#include "memory/memory.h"
#include "widgets/asset_browser_widget.h"
#include "widgets/entity_inspector_widget.h"
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
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* newSceneAction = fileMenu->addAction(tr("&New Scene"));
    newSceneAction->setShortcut(QKeySequence::New);
    connect(newSceneAction, &QAction::triggered, this, &SaplingMainWindow::on_new_scene);
    auto* openSceneAction = fileMenu->addAction(tr("&Open Scene..."));
    openSceneAction->setShortcut(QKeySequence::Open);
    connect(openSceneAction, &QAction::triggered, this, &SaplingMainWindow::on_open_scene);
    fileMenu->addSeparator();
    auto* saveSceneAction = fileMenu->addAction(tr("&Save Scene"));
    saveSceneAction->setShortcut(QKeySequence::Save);
    connect(saveSceneAction, &QAction::triggered, this, &SaplingMainWindow::on_save_scene);
    auto* saveSceneAsAction = fileMenu->addAction(tr("Save Scene &As..."));
    saveSceneAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveSceneAsAction, &QAction::triggered, this, &SaplingMainWindow::on_save_scene_as);
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    auto* editMenu = menuBar()->addMenu(tr("&Edit"));
    auto* undoAction = editMenu->addAction(tr("&Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false); // TODO: Implement undo/redo
    auto* redoAction = editMenu->addAction(tr("&Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(false); // TODO: Implement undo/redo
    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    auto* aboutAction = helpMenu->addAction(tr("&About Sapling"));
    connect(aboutAction, &QAction::triggered, [this]() {
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
    addDockWidget(Qt::RightDockWidgetArea, inspector_dock);
    menuBar()->actions()[2]->menu()->addAction(inspector_dock->toggleViewAction());
    auto* asset_dock = new QDockWidget(tr("Asset Browser"), this);
    asset_dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_AssetBrowser = new AssetBrowserWidget(asset_dock);
    asset_dock->setWidget(m_AssetBrowser);
    addDockWidget(Qt::BottomDockWidgetArea, asset_dock);
    menuBar()->actions()[2]->menu()->addAction(asset_dock->toggleViewAction());
}

void SaplingMainWindow::on_update_timer() {
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
