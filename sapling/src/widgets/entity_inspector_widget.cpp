#include "widgets/entity_inspector_widget.h"
#include "components/name_component.h"
#include "components/transform.h"
#include "editor_context.h"
#include "editor_utils.h"

#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

EntityInspectorWidget::EntityInspectorWidget(QWidget* parent) : QWidget(parent) {
    auto* scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);
    auto* content_widget = new QWidget();
    m_MainLayout = new QFormLayout(content_widget);
    m_MainLayout->setContentsMargins(5, 5, 5, 5);
    scroll_area->setWidget(content_widget);
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addWidget(scroll_area);
    clear_inspector();
}

void EntityInspectorWidget::set_entity(const sf::stl::optional<sf::Entity>& entity) {
    m_CurrentEntity = entity;
    populate_inspector();
}

void EntityInspectorWidget::clear_inspector() {
    while (m_MainLayout->count() > 0) {
        auto* item = m_MainLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_NameEdit = nullptr;
    m_PosX = m_PosY = m_PosZ = nullptr;
    m_RotX = m_RotY = m_RotZ = nullptr;
    m_ScaleX = m_ScaleY = m_ScaleZ = nullptr;
    auto* placeholder = new QLabel("No entity selected");
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("QLabel { color: gray; margin: 20px; }");
    m_MainLayout->addRow(placeholder);
}

void EntityInspectorWidget::populate_inspector() {
    clear_inspector();
    if (!m_CurrentEntity.has_value()) {
        return;
    }
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    m_UpdatingUI = true;
    create_name_section();
    create_transform_section();
    // TODO: Add more component sections (RenderComponent, etc.)
    m_UpdatingUI = false;
}

void EntityInspectorWidget::create_name_section() {
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager || !m_CurrentEntity.has_value()) {
        return;
    }
    auto& name_comp = ec_manager->engine_component<sf::components::NameComponent>(m_CurrentEntity.value());
    auto* group = new QGroupBox("Name");
    auto* group_layout = new QFormLayout(group);
    m_NameEdit = new QLineEdit(str2q(name_comp.name()));
    connect(m_NameEdit, &QLineEdit::textChanged, this, &EntityInspectorWidget::on_name_changed);
    group_layout->addRow("Name:", m_NameEdit);
    m_MainLayout->addRow(group);
}

void EntityInspectorWidget::create_transform_section() {
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager || !m_CurrentEntity.has_value()) {
        return;
    }
    auto& transform = ec_manager->engine_component<sf::components::Transform>(m_CurrentEntity.value());
    auto* group = new QGroupBox("Transform");
    auto* group_layout = new QFormLayout(group);
    auto create_spin_box = []() {
        auto* spin = new QDoubleSpinBox();
        spin->setRange(-100000.0, 100000.0);
        spin->setDecimals(3);
        spin->setSingleStep(0.1);
        return spin;
    };
    m_PosX = create_spin_box();
    m_PosY = create_spin_box();
    m_PosZ = create_spin_box();
    m_PosX->setValue(transform.position().x);
    m_PosY->setValue(transform.position().y);
    m_PosZ->setValue(transform.position().z);
    auto* pos_layout = new QHBoxLayout();
    pos_layout->addWidget(new QLabel("X:"));
    pos_layout->addWidget(m_PosX);
    pos_layout->addWidget(new QLabel("Y:"));
    pos_layout->addWidget(m_PosY);
    pos_layout->addWidget(new QLabel("Z:"));
    pos_layout->addWidget(m_PosZ);
    group_layout->addRow("Position:", pos_layout);
    m_RotX = create_spin_box();
    m_RotY = create_spin_box();
    m_RotZ = create_spin_box();
    m_RotX->setRange(-360.0, 360.0);
    m_RotY->setRange(-360.0, 360.0);
    m_RotZ->setRange(-360.0, 360.0);
    m_RotX->setValue(transform.rotation().x);
    m_RotY->setValue(transform.rotation().y);
    m_RotZ->setValue(transform.rotation().z);
    auto* rot_layout = new QHBoxLayout();
    rot_layout->addWidget(new QLabel("X:"));
    rot_layout->addWidget(m_RotX);
    rot_layout->addWidget(new QLabel("Y:"));
    rot_layout->addWidget(m_RotY);
    rot_layout->addWidget(new QLabel("Z:"));
    rot_layout->addWidget(m_RotZ);
    group_layout->addRow("Rotation:", rot_layout);
    m_ScaleX = create_spin_box();
    m_ScaleY = create_spin_box();
    m_ScaleZ = create_spin_box();
    m_ScaleX->setValue(transform.scale().x);
    m_ScaleY->setValue(transform.scale().y);
    m_ScaleZ->setValue(transform.scale().z);
    auto* scale_layout = new QHBoxLayout();
    scale_layout->addWidget(new QLabel("X:"));
    scale_layout->addWidget(m_ScaleX);
    scale_layout->addWidget(new QLabel("Y:"));
    scale_layout->addWidget(m_ScaleY);
    scale_layout->addWidget(new QLabel("Z:"));
    scale_layout->addWidget(m_ScaleZ);
    group_layout->addRow("Scale:", scale_layout);
    connect(m_PosX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_PosY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_PosZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_RotX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_RotY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_RotZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_ScaleX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_ScaleY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    connect(m_ScaleZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EntityInspectorWidget::on_transform_changed);
    m_MainLayout->addRow(group);
}

void EntityInspectorWidget::on_name_changed() {
    if (m_UpdatingUI || !m_CurrentEntity.has_value() || !m_NameEdit) {
        return;
    }
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    auto& name_comp = ec_manager->engine_component<sf::components::NameComponent>(m_CurrentEntity.value());
    name_comp.name(m_NameEdit->text().toLatin1().data());
}

void EntityInspectorWidget::on_transform_changed() {
    if (m_UpdatingUI || !m_CurrentEntity.has_value()) {
        return;
    }
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    auto& transform = ec_manager->engine_component<sf::components::Transform>(m_CurrentEntity.value());
    if (m_PosX && m_PosY && m_PosZ) {
        transform.position(sf::math::vec4{static_cast<sf::f32>(m_PosX->value()), static_cast<sf::f32>(m_PosY->value()),
                                          static_cast<sf::f32>(m_PosZ->value()), 1.0f});
    }
    if (m_RotX && m_RotY && m_RotZ) {
        transform.rotation(sf::math::quat::from_euler(static_cast<sf::f32>(m_RotX->value()), static_cast<sf::f32>(m_RotY->value()),
                                                      static_cast<sf::f32>(m_RotZ->value())));
    }
    if (m_ScaleX && m_ScaleY && m_ScaleZ) {
        transform.scale(sf::math::vec4{static_cast<sf::f32>(m_ScaleX->value()), static_cast<sf::f32>(m_ScaleY->value()),
                                       static_cast<sf::f32>(m_ScaleZ->value()), 1.0f});
    }
}
