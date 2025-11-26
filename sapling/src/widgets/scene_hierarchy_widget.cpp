#include "widgets/scene_hierarchy_widget.h"
#include "editor_context.h"
#include "components/name_component.h"
#include "components/transform.h"
#include "editor_utils.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QAction>

SceneHierarchyWidget::SceneHierarchyWidget(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_TreeWidget = new QTreeWidget(this);
    m_TreeWidget->setHeaderLabel("Entities");
    m_TreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_TreeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_TreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_TreeWidget);
    connect(m_TreeWidget, &QTreeWidget::itemSelectionChanged,
            this, &SceneHierarchyWidget::on_item_selection_changed);
    connect(m_TreeWidget, &QTreeWidget::customContextMenuRequested,
            this, &SceneHierarchyWidget::on_context_menu_requested);
    refresh();
}

void SceneHierarchyWidget::refresh() {
    m_TreeWidget->clear();
    build_tree();
}

void SceneHierarchyWidget::build_tree() {
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    auto valid_indices = ec_manager->get_all_valid_entities();
    for (sf::u32 i = 0; i < valid_indices.size(); ++i) {
        auto maybe_entity = ec_manager->entity(valid_indices[i]);
        if (!maybe_entity.has_value()) {
            continue;
        }
        const auto& entity = maybe_entity.value();
        auto& transform = ec_manager->engine_component<sf::components::Transform>(entity);
        if (transform.parent() == static_cast<sf::u32>(-1)) {
            auto* item = create_tree_item_for_entity(entity);
            m_TreeWidget->addTopLevelItem(item);
            build_tree_recursive(valid_indices, i, item);
        }
    }
    m_TreeWidget->expandAll();
}

void SceneHierarchyWidget::build_tree_recursive(const sf::stl::vector<sf::stl::generational_index>& indices,
                                                sf::u32 parent_index,
                                                QTreeWidgetItem* parent_item) {
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    auto parent_entity_opt = ec_manager->entity(indices[parent_index]);
    if (!parent_entity_opt.has_value()) {
        return;
    }
    const auto& parent_entity = parent_entity_opt.value();
    sf::u32 parent_id = indices[parent_index].index;
    for (sf::u32 i = 0; i < indices.size(); ++i) {
        auto maybe_entity = ec_manager->entity(indices[i]);
        if (!maybe_entity.has_value()) {
            continue;
        }
        const auto& entity = maybe_entity.value();
        auto& transform = ec_manager->engine_component<sf::components::Transform>(entity);
        if (transform.parent() == parent_id) {
            auto* child_item = create_tree_item_for_entity(entity);
            parent_item->addChild(child_item);
            build_tree_recursive(indices, i, child_item);
        }
    }
}

QTreeWidgetItem* SceneHierarchyWidget::create_tree_item_for_entity(const sf::Entity& entity) {
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return new QTreeWidgetItem();
    }
    auto& name_component = ec_manager->engine_component<sf::components::NameComponent>(entity);
    auto* item = new QTreeWidgetItem();
    item->setText(0, str2q(name_component.name()));
    item->setData(0, Qt::UserRole, QVariant::fromValue(entity.id().index));
    item->setData(0, Qt::UserRole + 1, QVariant::fromValue(entity.id().generation));
    return item;
}

void SceneHierarchyWidget::on_item_selection_changed() {
    auto selected_items = m_TreeWidget->selectedItems();
    if (selected_items.isEmpty()) {
        m_SelectedEntity.reset();
        emit entity_selected(m_SelectedEntity);
        return;
    }
    auto* item = selected_items.first();
    sf::u32 index = item->data(0, Qt::UserRole).toUInt();
    sf::u32 generation = item->data(0, Qt::UserRole + 1).toUInt();
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (ec_manager) {
        sf::stl::generational_index gen_index{index, generation};
        auto maybe_entity = ec_manager->entity(gen_index);
        if (maybe_entity.has_value()) {
            m_SelectedEntity = maybe_entity.value();
            emit entity_selected(m_SelectedEntity);
        }
    }
}

void SceneHierarchyWidget::on_context_menu_requested(const QPoint& pos) {
    QMenu context_menu(this);
    auto* createAction = context_menu.addAction("Create Entity");
    connect(createAction, &QAction::triggered, this, &SceneHierarchyWidget::on_create_entity);
    auto* deleteAction = context_menu.addAction("Delete Entity");
    deleteAction->setEnabled(!m_TreeWidget->selectedItems().isEmpty());
    connect(deleteAction, &QAction::triggered, this, &SceneHierarchyWidget::on_delete_entity);
    context_menu.exec(m_TreeWidget->mapToGlobal(pos));
}

void SceneHierarchyWidget::on_create_entity() {
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    auto new_entity = ec_manager->create_entity();
    if (m_SelectedEntity.has_value()) {
        auto& transform = ec_manager->engine_component<sf::components::Transform>(new_entity);
        transform.parent(m_SelectedEntity.value().id().index);
    }
    refresh();
    m_SelectedEntity = new_entity;
    emit entity_selected(m_SelectedEntity);
}

void SceneHierarchyWidget::on_delete_entity() {
    if (!m_SelectedEntity.has_value()) {
        return;
    }
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    ec_manager->destroy_entity(m_SelectedEntity.value());
    m_SelectedEntity.reset();
    refresh();
    emit entity_selected(m_SelectedEntity);
}
