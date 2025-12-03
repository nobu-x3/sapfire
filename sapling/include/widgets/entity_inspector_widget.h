#pragma once

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
#include "Sapfire.h"
#include "widgets/rtti_drawer.h"

// EntityInspectorWidget: Displays and edits properties of the selected entity
class EntityInspectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit EntityInspectorWidget(QWidget* parent = nullptr);

    void set_entity(const sf::stl::optional<sf::Entity>& entity);

private:
    void clear_inspector();
    void populate_inspector();
    void create_component_sections();
signals:
    void entity_component_changed(sf::Entity entity);
    void render_component_added(sf::Entity entity);

private slots:
    void on_add_component_clicked();
    void on_pick_component(const QString&);

private:
    QFormLayout* m_MainLayout{nullptr};
    sf::stl::optional<sf::Entity> m_CurrentEntity;
    QPushButton* m_AddButton{nullptr};
    QLabel* m_PlaceholderLabel{nullptr};
    sf::stl::vector<sf::stl::string> m_AvailableComponentTypeNames{sf::mem::MemTag::Logic};
    bool m_UpdatingUI{false};
};
