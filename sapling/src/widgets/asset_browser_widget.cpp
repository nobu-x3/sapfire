#include "widgets/asset_browser_widget.h"
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <core/logger.h>
#include "editor_context.h"
#include "project_manager.h"
#include "widgets/asset_delegate.h"
#include "widgets/asset_filter_proxy.h"
#include "widgets/file_system_model.h"

AssetBrowserWidget::AssetBrowserWidget(SQFileSystemModel* model, QWidget* parent) :
    QWidget(parent), m_FSModel(model), m_FilterProxy(nullptr), m_HasFinishedLoading(false), m_CurrentViewType(ViewType::Loading) {
    m_FilterProxy = new SQAssetFilterProxyModel(m_FSModel, this);
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(5, 5, 5, 5);
    QHBoxLayout* search_layout = new QHBoxLayout();
    QLabel* search_label = new QLabel("Search:");
    m_SearchBox = new QLineEdit();
    m_SearchBox->setPlaceholderText("Filter by name...");
    connect(m_SearchBox, &QLineEdit::textChanged, this, &AssetBrowserWidget::on_search_changed);
    search_layout->addWidget(search_label);
    search_layout->addWidget(m_SearchBox);
    main_layout->addLayout(search_layout);
    QHBoxLayout* filter_layout = new QHBoxLayout();
    QLabel* filter_label = new QLabel("Types:");
    filter_layout->addWidget(filter_label);
    m_TextureFilter = new QPushButton();
    m_TextureFilter->setIcon(QIcon(":/icons/image_24.png"));
    m_TextureFilter->setToolTip("Texture");
    m_TextureFilter->setCheckable(true);
    m_TextureFilter->setChecked(true);
    m_TextureFilter->setFixedSize(32, 32);
    connect(m_TextureFilter, &QPushButton::toggled, this, &AssetBrowserWidget::on_filter_button_toggled);
    filter_layout->addWidget(m_TextureFilter);
    m_MeshFilter = new QPushButton();
    m_MeshFilter->setIcon(QIcon(":/icons/mesh_24.png"));
    m_MeshFilter->setToolTip("Mesh");
    m_MeshFilter->setCheckable(true);
    m_MeshFilter->setChecked(true);
    m_MeshFilter->setFixedSize(32, 32);
    connect(m_MeshFilter, &QPushButton::toggled, this, &AssetBrowserWidget::on_filter_button_toggled);
    filter_layout->addWidget(m_MeshFilter);
    m_MaterialFilter = new QPushButton("Mat");
    m_MaterialFilter->setToolTip("Material");
    m_MaterialFilter->setCheckable(true);
    m_MaterialFilter->setChecked(true);
    m_MaterialFilter->setFixedSize(48, 32);
    connect(m_MaterialFilter, &QPushButton::toggled, this, &AssetBrowserWidget::on_filter_button_toggled);
    filter_layout->addWidget(m_MaterialFilter);
    m_ShaderFilter = new QPushButton("Shd");
    m_ShaderFilter->setToolTip("Shader");
    m_ShaderFilter->setCheckable(true);
    m_ShaderFilter->setChecked(true);
    m_ShaderFilter->setFixedSize(48, 32);
    connect(m_ShaderFilter, &QPushButton::toggled, this, &AssetBrowserWidget::on_filter_button_toggled);
    filter_layout->addWidget(m_ShaderFilter);
    m_SceneFilter = new QPushButton("Scn");
    m_SceneFilter->setToolTip("Scene");
    m_SceneFilter->setCheckable(true);
    m_SceneFilter->setChecked(true);
    m_SceneFilter->setFixedSize(48, 32);
    connect(m_SceneFilter, &QPushButton::toggled, this, &AssetBrowserWidget::on_filter_button_toggled);
    filter_layout->addWidget(m_SceneFilter);
    filter_layout->addStretch();
    main_layout->addLayout(filter_layout);
    QHBoxLayout* views_layout = new QHBoxLayout();
    m_LoadingLabel = new QLabel("Loading...");
    m_ListView = new QListView();
    m_ListView->setViewMode(QListView::ViewMode::IconMode);
    m_ListView->setFlow(QListView::Flow::LeftToRight);
    m_ListView->setMovement(QListView::Static);
    m_ListView->setResizeMode(QListView::Adjust);
    m_ListView->setWrapping(true);
    m_ListView->setGridSize(QSize(128, 128));
    m_ListView->setSpacing(8);
    m_ListView->setUniformItemSizes(true);
    m_ListView->setItemDelegate(new SQAssetDelegate(m_ListView));
    m_ListView->setModelColumn(EFileModelField::NAME);
    m_ListView->setVisible(false);
    m_TableView = new QTableView();
    m_TableView->verticalHeader()->hide();
    m_TableView->hideColumn(EFileModelField::PATH);
    m_TableView->setShowGrid(false);
    m_TableView->setColumnHidden(EFileModelField::PATH, true);
    m_TableView->setColumnHidden(EFileModelField::THUMBNAIL, true);
    m_TableView->setColumnHidden(EFileModelField::META_PATH, true);
    m_TableView->setVisible(false);
    views_layout->addWidget(m_LoadingLabel);
    views_layout->addWidget(m_ListView);
    views_layout->addWidget(m_TableView);
    QHBoxLayout* button_layout = new QHBoxLayout();
    m_ViewSwitchButton = new QPushButton("L");
    m_ViewSwitchButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
    m_ViewSwitchButton->setFixedSize(32, 32);
    m_ViewSwitchButton->setVisible(false);
    button_layout->addWidget(m_ViewSwitchButton);
    button_layout->setAlignment(Qt::AlignmentFlag::AlignLeft);
    connect(m_ViewSwitchButton, &QPushButton::pressed, [this]() {
        switch (m_CurrentViewType) {
        case ViewType::List:
            set_view_type(ViewType::Table);
            break;
        case ViewType::Table:
            set_view_type(ViewType::List);
        case ViewType::Loading:
            break;
        }
    });
    button_layout->setSizeConstraint(QLayout::SetMinimumSize);
    QVBoxLayout* view_with_buttons_layout = new QVBoxLayout();
    view_with_buttons_layout->addLayout(views_layout);
    view_with_buttons_layout->addLayout(button_layout);
    main_layout->addLayout(view_with_buttons_layout);
    setLayout(main_layout);
    set_view_type(ViewType::Loading);
    if (m_FSModel->is_loaded())
        on_loading_finished();
    else
        connect(m_FSModel, &SQFileSystemModel::loading_finished, this, &AssetBrowserWidget::on_loading_finished);
    m_ImportAssetAction = new QAction{"Import new asset"};
    connect(m_ImportAssetAction, &QAction::triggered, this, &AssetBrowserWidget::on_import_new_action_pressed);
}

AssetBrowserWidget::~AssetBrowserWidget() {}

void AssetBrowserWidget::on_loading_finished() {
    m_HasFinishedLoading = true;
    m_ViewSwitchButton->setVisible(true);
    m_ListView->setModel(m_FilterProxy);
    m_TableView->setModel(m_FilterProxy);
    m_TableView->resizeColumnsToContents();
    m_TableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_TableView->setColumnHidden(EFileModelField::PATH, true);
    m_TableView->setColumnHidden(EFileModelField::THUMBNAIL, true);
    m_TableView->setColumnHidden(EFileModelField::META_PATH, true);
    set_view_type(ViewType::List);
}

void AssetBrowserWidget::set_view_type(ViewType type) {
    if (!m_HasFinishedLoading)
        return;
    m_LoadingLabel->setVisible(type == ViewType::Loading);
    m_CurrentViewType = type;
    m_TableView->setVisible(type == ViewType::Table);
    m_ListView->setVisible(type == ViewType::List);
    m_ViewSwitchButton->setText(type == ViewType::Table ? "L" : "T");
}

QModelIndexList AssetBrowserWidget::get_selected_assets() const {
    if (!m_HasFinishedLoading)
        return {};
    if (m_CurrentViewType == ViewType::Loading)
        return {};
    QVector<QModelIndex> selected_indices{};
    if (m_CurrentViewType == ViewType::Table) {
        return m_TableView->selectionModel()->selectedIndexes();
    }
    return m_ListView->selectionModel()->selectedIndexes();
}

void AssetBrowserWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu context_menu{this};
    context_menu.addAction(m_ImportAssetAction);
    context_menu.exec(mapToGlobal(event->pos()));
}

void AssetBrowserWidget::import_new_asset() {
    CLIENT_TRACE("Import new asset requested");
    QString project_path = EditorContext::instance().project_manager()->current_project_path();
    if (project_path.isEmpty()) {
        CLIENT_WARN("No project loaded, cannot import asset");
        return;
    }
    QString from_path = QFileDialog::getOpenFileName(this, tr("Import asset"), "",
                                                     "Textures (*.png *.bmp *.jpg *.dds);;Meshes (*.obj *.fbx *.gltf *.glb);;Materials "
                                                     "(*.mat);;Shaders (*.spv);;Scenes (*.scene);;All Files (*)");
    if (from_path.isEmpty()) {
        return;
    }
    QString to_path = QFileDialog::getSaveFileName(this, "Copy asset", project_path + "/assets",
                                                   "Textures (*.png *.bmp *.jpg *.dds);;Meshes (*.obj *.fbx *.gltf *.glb);;Materials "
                                                   "(*.mat);;Shaders (*.spv);;Scenes (*.scene);;All Files (*)");
    if (to_path.isEmpty()) {
        return;
    }
    if (!QFile::copy(from_path, to_path)) {
        CLIENT_ERROR("Failed to copy asset from {} to {}", from_path.toStdString(), to_path.toStdString());
        return;
    }
    m_FSModel->import_asset(to_path);
}

void AssetBrowserWidget::on_search_changed(const QString& text) {
    if (m_FilterProxy) {
        m_FilterProxy->set_search_filter(text);
    }
}

void AssetBrowserWidget::on_filter_button_toggled() { update_filter(); }

void AssetBrowserWidget::on_import_new_action_pressed(bool) {
    import_new_asset();
}

void AssetBrowserWidget::update_filter() {
    if (!m_FilterProxy)
        return;

    int filter_type = 0;
    if (m_TextureFilter->isChecked())
        filter_type |= EAssetFilterType::Texture;
    if (m_MeshFilter->isChecked())
        filter_type |= EAssetFilterType::Mesh;
    if (m_MaterialFilter->isChecked())
        filter_type |= EAssetFilterType::Material;
    if (m_ShaderFilter->isChecked())
        filter_type |= EAssetFilterType::Shader;
    if (m_SceneFilter->isChecked())
        filter_type |= EAssetFilterType::Scene;

    // If no filters are checked, show all
    if (filter_type == 0) {
        filter_type = EAssetFilterType::All;
    }

    m_FilterProxy->set_asset_filter_type(static_cast<EAssetFilterType::TYPE>(filter_type));
}
