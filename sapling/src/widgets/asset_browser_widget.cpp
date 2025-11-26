#include "widgets/asset_browser_widget.h"
#include "editor_context.h"
#include "editor_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

AssetBrowserWidget::AssetBrowserWidget(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(5, 5, 5, 5);
    auto* filter_layout = new QHBoxLayout();
    m_SearchBox = new QLineEdit();
    m_SearchBox->setPlaceholderText("Search assets...");
    connect(m_SearchBox, &QLineEdit::textChanged, this, &AssetBrowserWidget::on_filter_changed);
    m_TypeFilter = new QComboBox();
    m_TypeFilter->addItem("All");
    m_TypeFilter->addItem("Meshes");
    m_TypeFilter->addItem("Textures");
    m_TypeFilter->addItem("Materials");
    connect(m_TypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AssetBrowserWidget::on_asset_type_changed);
    m_ImportButton = new QPushButton("Import...");
    connect(m_ImportButton, &QPushButton::clicked, this, &AssetBrowserWidget::on_import_asset_clicked);
    filter_layout->addWidget(m_SearchBox);
    filter_layout->addWidget(m_TypeFilter);
    filter_layout->addWidget(m_ImportButton);
    main_layout->addLayout(filter_layout);
    m_AssetList = new QListWidget();
    m_AssetList->setViewMode(QListWidget::IconMode);
    m_AssetList->setIconSize(QSize(64, 64));
    m_AssetList->setResizeMode(QListWidget::Adjust);
    m_AssetList->setMovement(QListWidget::Static);
    connect(m_AssetList, &QListWidget::itemDoubleClicked,
            this, &AssetBrowserWidget::on_asset_double_clicked);
    main_layout->addWidget(m_AssetList);
    refresh();
}

void AssetBrowserWidget::refresh() {
    populate_asset_list();
}

void AssetBrowserWidget::populate_asset_list() {
    m_AssetList->clear();
    auto* asset_manager = EditorContext::instance().asset_manager();
    if (!asset_manager) {
        return;
    }
    // TODO: Populate from asset manager
    // For now, add placeholder items
    if (m_CurrentTypeFilter == 0 || m_CurrentTypeFilter == 1) {
        // TODO: Get mesh list from asset manager
        // auto& mesh_registry = asset_manager->mesh_registry();
        // for (const auto& [id, mesh] : mesh_registry) {
        //     auto* item = new QListWidgetItem(QString::fromStdString(mesh.name));
        //     item->setData(Qt::UserRole, id);
        //     m_AssetList->addItem(item);
        // }
    }
    if (m_CurrentTypeFilter == 0 || m_CurrentTypeFilter == 2) {
        // TODO: Get texture list from asset manager
        // Similar to meshes
    }
    if (m_AssetList->count() == 0) {
        auto* placeholder = new QListWidgetItem("No assets found. Click Import to add assets.");
        placeholder->setFlags(Qt::ItemIsEnabled); // Not selectable
        m_AssetList->addItem(placeholder);
    }
}

void AssetBrowserWidget::filter_assets() {
    for (int i = 0; i < m_AssetList->count(); ++i) {
        auto* item = m_AssetList->item(i);
        bool visible = true;
        if (!m_CurrentFilter.empty()) {
            QString item_text = item->text();
            visible = item_text.contains(str2q(m_CurrentFilter), Qt::CaseInsensitive);
        }
        item->setHidden(!visible);
    }
}

void AssetBrowserWidget::on_import_asset_clicked() {
    QStringList filenames = QFileDialog::getOpenFileNames(
        this,
        tr("Import Assets"),
        QString(), // TODO: Use project directory
        tr("3D Models (*.obj *.fbx *.gltf);;Textures (*.png *.jpg *.tga *.dds);;All Files (*)")
    );
    if (filenames.isEmpty()) {
        return;
    }
    auto* asset_manager = EditorContext::instance().asset_manager();
    if (!asset_manager) {
        QMessageBox::warning(this, "Error", "Asset manager not initialized!");
        return;
    }
    // TODO: Import assets using asset manager
    for (const QString& filename : filenames) {
        // Determine asset type from extension
        // Call appropriate import method on asset manager
        // asset_manager->import_mesh(filename.toStdString());
        // asset_manager->import_texture(filename.toStdString());
    }
    refresh();
}

void AssetBrowserWidget::on_filter_changed(const QString& text) {
    m_CurrentFilter = text.toStdString();
    filter_assets();
}

void AssetBrowserWidget::on_asset_type_changed(int index) {
    m_CurrentTypeFilter = index;
    refresh();
}

void AssetBrowserWidget::on_asset_double_clicked(QListWidgetItem* item) {
    // TODO: Open asset in appropriate editor
    // - Mesh: Show in scene view
    // - Texture: Open texture viewer
    // - Material: Open material editor
}
