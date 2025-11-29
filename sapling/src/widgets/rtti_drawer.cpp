#include "widgets/rtti_drawer.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>

RttiDrawer::RttiDrawer(QWidget* parent) : QWidget(parent) {
    m_Layout = new QFormLayout(this);
    m_Layout->setContentsMargins(0, 0, 0, 0);
}

void RttiDrawer::draw_rtti(sf::rtti::rtti_object& rtti_obj) {
    clear();
    for (auto& field : rtti_obj.fields) {
        draw_field(rtti_obj, field);
    }
}

void RttiDrawer::clear() {
    while (m_Layout->count() > 0) {
        auto* item = m_Layout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void RttiDrawer::draw_field(sf::rtti::rtti_object& rtti_obj, sf::rtti::rtti_field& field) {
    QString field_name = QString::fromLatin1(field.name.data(), field.name.size());
    void* field_ptr = reinterpret_cast<void*>(reinterpret_cast<sf::u8*>(rtti_obj.head) + field.offset);
    switch (field.type) {
    case sf::rtti::rtti_type::BOOL:
        {
            auto* checkbox = new QCheckBox(this);
            checkbox->setChecked(*reinterpret_cast<bool*>(field_ptr));
            connect(checkbox, &QCheckBox::checkStateChanged,
                    [field_ptr](Qt::CheckState state) { *reinterpret_cast<bool*>(field_ptr) = (state == Qt::Checked); });
            m_Layout->addRow(field_name, checkbox);
            break;
        }
    case sf::rtti::rtti_type::I32:
        {
            auto* spinbox = new QSpinBox(this);
            spinbox->setRange(INT_MIN, INT_MAX);
            spinbox->setValue(*reinterpret_cast<sf::i32*>(field_ptr));
            connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
                    [field_ptr](int val) { *reinterpret_cast<sf::i32*>(field_ptr) = val; });
            m_Layout->addRow(field_name, spinbox);
            break;
        }
    case sf::rtti::rtti_type::U32:
        {
            auto* spinbox = new QSpinBox(this);
            spinbox->setRange(0, INT_MAX);
            spinbox->setValue(*reinterpret_cast<sf::u32*>(field_ptr));
            connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
                    [field_ptr](int val) { *reinterpret_cast<sf::u32*>(field_ptr) = static_cast<sf::u32>(val); });
            m_Layout->addRow(field_name, spinbox);
            break;
        }
    case sf::rtti::rtti_type::F32:
        {
            auto* spinbox = new QDoubleSpinBox(this);
            spinbox->setRange(-1000000.0, 1000000.0);
            spinbox->setDecimals(3);
            spinbox->setValue(*reinterpret_cast<sf::f32*>(field_ptr));
            connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    [field_ptr](double val) { *reinterpret_cast<sf::f32*>(field_ptr) = static_cast<sf::f32>(val); });
            m_Layout->addRow(field_name, spinbox);
            break;
        }
    case sf::rtti::rtti_type::STRING:
        {
            auto* lineedit = new QLineEdit(this);
            auto* str_ptr = reinterpret_cast<sf::stl::string*>(field_ptr);
            lineedit->setText(QString::fromLatin1(str_ptr->data(), str_ptr->size()));
            connect(lineedit, &QLineEdit::textChanged,
                    [str_ptr](const QString& text) { *str_ptr = sf::stl::string(sf::mem::MemTag::Strings, text.toLatin1().data()); });
            m_Layout->addRow(field_name, lineedit);
            break;
        }
    default:
        {
            auto* label = new QLabel(QString::fromLatin1("<unsupported type>"), this);
            m_Layout->addRow(field_name, label);
            break;
        }
    }
}
