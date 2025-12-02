#pragma once

#include <QTreeWidget>
#include <QWidget>
#include "Sapfire.h"

// SceneHierarchyWidget: Displays the scene entity hierarchy as a tree
class SceneHierarchyWidget : public QWidget {
    Q_OBJECT

public:
    explicit SceneHierarchyWidget(QWidget* parent = nullptr);

    void refresh();

signals:
    void entity_selected(const sf::stl::optional<sf::Entity>& entity);

private slots:
    void on_item_selection_changed();
    void on_context_menu_requested(const QPoint& pos);
    void on_create_entity();
    void on_delete_entity();

private:
    void build_tree();
    QTreeWidgetItem* create_tree_item_for_entity(const sf::Entity& entity);
    void build_tree_recursive(const sf::stl::vector<sf::stl::generational_index>& indices, sf::u32 parent_index,
                              QTreeWidgetItem* parent_item);

private:
    QTreeWidget* m_TreeWidget{nullptr};
    sf::stl::optional<sf::Entity> m_SelectedEntity;
};
