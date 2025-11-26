#pragma once

#include <QWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include "Sapfire.h"

// EntityInspectorWidget: Displays and edits properties of the selected entity
class EntityInspectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit EntityInspectorWidget(QWidget* parent = nullptr);

    void set_entity(const sf::stl::optional<sf::Entity>& entity);

private:
    void clear_inspector();
    void populate_inspector();
    void create_transform_section();
    void create_name_section();

private slots:
    void on_name_changed();
    void on_transform_changed();

private:
    QFormLayout* m_MainLayout{nullptr};
    sf::stl::optional<sf::Entity> m_CurrentEntity;
    QLineEdit* m_NameEdit{nullptr};
    QDoubleSpinBox* m_PosX{nullptr};
    QDoubleSpinBox* m_PosY{nullptr};
    QDoubleSpinBox* m_PosZ{nullptr};
    QDoubleSpinBox* m_RotX{nullptr};
    QDoubleSpinBox* m_RotY{nullptr};
    QDoubleSpinBox* m_RotZ{nullptr};
    QDoubleSpinBox* m_ScaleX{nullptr};
    QDoubleSpinBox* m_ScaleY{nullptr};
    QDoubleSpinBox* m_ScaleZ{nullptr};
    bool m_UpdatingUI{false}; // Prevent feedback loops
};
