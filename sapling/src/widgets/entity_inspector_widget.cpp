#include "widgets/entity_inspector_widget.h"
#include "components/name_component.h"
#include "components/transform.h"
#include "editor_context.h"
#include "editor_utils.h"

#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
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
    m_PlaceholderLabel = new QLabel("No entity selected", this);
    m_PlaceholderLabel->setAlignment(Qt::AlignCenter);
    m_PlaceholderLabel->setStyleSheet("QLabel { color: gray; margin: 20px; }");
    auto* add_bar = new QWidget(this);
    auto* add_bar_layout = new QHBoxLayout(add_bar);
    add_bar_layout->setContentsMargins(4, 4, 4, 4);
    add_bar_layout->addStretch(1);
    m_AddButton = new QPushButton("+", add_bar);
    m_AddButton->setToolTip("Add component");
    add_bar_layout->addWidget(m_AddButton);
    main_layout->addWidget(add_bar);
    connect(m_AddButton, &QPushButton::clicked, this, &EntityInspectorWidget::on_add_component_clicked);
    m_AddButton->setVisible(false);
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
            // if this is the cached placeholder, keep it alive and just hide it
            if (item->widget() == m_PlaceholderLabel) {
                m_PlaceholderLabel->hide();
            } else {
                item->widget()->deleteLater();
            }
        }
        delete item;
    }
}

void EntityInspectorWidget::populate_inspector() {
    clear_inspector();
    if (!m_CurrentEntity.has_value()) {
        if (m_MainLayout->indexOf(m_PlaceholderLabel) == -1)
            m_MainLayout->addRow(m_PlaceholderLabel);
        m_PlaceholderLabel->show();
        if (m_AddButton)
            m_AddButton->setVisible(false);
        return;
    }
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager) {
        return;
    }
    m_UpdatingUI = true;
    create_component_sections();
    m_UpdatingUI = false;
}

void EntityInspectorWidget::create_component_sections() {
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager || !m_CurrentEntity.has_value())
        return;
    for (auto& [type_name, list] : sf::components::ComponentRegistry::s_EngineComponentLists) {
        if (!ec_manager->has_component(m_CurrentEntity.value(), sf::stl::string(type_name.c_str())))
            continue;
        auto* group = new QGroupBox(QString::fromLatin1(list->to_string().c_str()));
        auto* group_layout = new QFormLayout(group);
        auto* drawer = new RttiDrawer(group);
        connect(drawer, &RttiDrawer::rtti_changed, [this, e = m_CurrentEntity.value()]() { emit entity_component_changed(e); });
        sf::rtti::rtti_object* obj =
            EditorContext::instance().ec_manager()->rtti_for(m_CurrentEntity.value(), sf::stl::string(type_name.c_str()));
        if (obj)
            drawer->draw_rtti(*obj);
        group_layout->addRow(drawer);
        m_MainLayout->addRow(group);
    }
    auto custom_components = ec_manager->components(m_CurrentEntity.value());
    for (auto& comp : custom_components) {
        if (!comp)
            continue;
        auto* group = new QGroupBox(QString::fromLatin1(comp->to_string().c_str()));
        auto* group_layout = new QFormLayout(group);
        auto* drawer = new RttiDrawer(group);
        connect(drawer, &RttiDrawer::rtti_changed, [this, e = m_CurrentEntity.value()]() { emit entity_component_changed(e); });
        drawer->draw_rtti(comp->get_rtti());
        group_layout->addRow(drawer);
        m_MainLayout->addRow(group);
    }
    if (m_AddButton)
        m_AddButton->setVisible(true);
}

void EntityInspectorWidget::on_add_component_clicked() {
    if (!m_CurrentEntity.has_value())
        return;
    QDialog* dlg = new QDialog(this, Qt::Popup);
    dlg->setWindowTitle("Add Component");
    auto* layout = new QVBoxLayout(dlg);
    auto* search = new QLineEdit(dlg);
    search->setPlaceholderText("Search components...");
    layout->addWidget(search);
    auto* list = new QListWidget(dlg);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setUniformItemSizes(true);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(list);
    list->clear();
    for (auto& [type_name, comp_list] : sf::components::ComponentRegistry::s_EngineComponentLists) {
        QString display = QString::fromLatin1(comp_list->to_string().c_str());
        auto* item = new QListWidgetItem(display, list);
        item->setData(Qt::UserRole, QString::fromLatin1(type_name.c_str()));
    }
    for (auto& [type_name, comp_list] : sf::components::ComponentRegistry::s_CustomComponentLists) {
        QString display = QString::fromLatin1(comp_list->to_string().c_str());
        auto* item = new QListWidgetItem(display, list);
        item->setData(Qt::UserRole, QString::fromLatin1(type_name.c_str()));
    }
    connect(search, &QLineEdit::textChanged, [list](const QString& txt) {
        for (int i = 0; i < list->count(); ++i) {
            auto* item = list->item(i);
            bool show = item->text().contains(txt, Qt::CaseInsensitive);
            item->setHidden(!show);
        }
    });
    connect(list, &QListWidget::itemActivated, [this, dlg](QListWidgetItem* item) {
        if (!item)
            return;
        // TODO: maybe add proper filtering if this causes performance issues when there are a lot of components.
        QString type_name = item->data(Qt::UserRole).toString();
        on_pick_component(type_name);
        dlg->close();
    });
    QPoint pos = m_AddButton->mapToGlobal(QPoint(0, m_AddButton->height()));
    dlg->move(pos);
    dlg->setModal(false);
    dlg->show();
}

void EntityInspectorWidget::on_pick_component(const QString& qtype_name) {
    if (!m_CurrentEntity.has_value())
        return;
    auto type_name = sf::stl::string(sf::mem::MemTag::Temp, qtype_name.toLatin1().data());
    auto* ec_manager = EditorContext::instance().ec_manager();
    if (!ec_manager)
        return;
    if (ec_manager->has_component(m_CurrentEntity.value(), type_name)) {
        ec_manager->reset_component(m_CurrentEntity.value(), type_name);
    } else {
        ec_manager->add_component(m_CurrentEntity.value(), type_name);
    }
    populate_inspector();
}
