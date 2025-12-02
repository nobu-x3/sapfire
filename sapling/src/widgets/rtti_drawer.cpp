#include "widgets/rtti_drawer.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTimer>
#include <array>

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
    // copy these to safely capture in lambdas per-field
    sf::rtti::rtti_field local_field = field;
    sf::rtti::rtti_object* obj_ptr = &rtti_obj;
    QString field_name = QString::fromLatin1(local_field.name.data(), local_field.name.size());
    void* field_ptr = reinterpret_cast<void*>(reinterpret_cast<sf::u8*>(obj_ptr->head) + local_field.offset);
    switch (local_field.type) {
    case sf::rtti::rtti_type::BOOL:
        {
            auto* checkbox = new QCheckBox(this);
            checkbox->setChecked(*reinterpret_cast<bool*>(field_ptr));
            connect(checkbox, &QCheckBox::checkStateChanged, [this, obj_ptr, local_field, field_ptr](Qt::CheckState state) mutable {
                bool val = (state == Qt::Checked);
                sf::rtti::set_rtti_field_value(obj_ptr, const_cast<sf::rtti::rtti_field*>(&local_field), &val);
                // Only refresh the whole RTTI UI if the field has a setter (it may change other fields)
                if (local_field.setter)
                    QTimer::singleShot(0, this, [this, obj_ptr]() { this->draw_rtti(*obj_ptr); });
                emit rtti_changed();
            });
            m_Layout->addRow(field_name, checkbox);
            break;
        }
    case sf::rtti::rtti_type::I32:
        {
            auto* spinbox = new QSpinBox(this);
            spinbox->setRange(INT_MIN, INT_MAX);
            spinbox->setValue(*reinterpret_cast<sf::i32*>(field_ptr));
            connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), [this, obj_ptr, local_field](int val) mutable {
                sf::i32 v = static_cast<sf::i32>(val);
                sf::rtti::set_rtti_field_value(obj_ptr, const_cast<sf::rtti::rtti_field*>(&local_field), &v);
                if (local_field.setter)
                    QTimer::singleShot(0, this, [this, obj_ptr]() { this->draw_rtti(*obj_ptr); });
                emit rtti_changed();
            });
            m_Layout->addRow(field_name, spinbox);
            break;
        }
    case sf::rtti::rtti_type::U32:
        {
            auto* spinbox = new QSpinBox(this);
            spinbox->setRange(0, INT_MAX);
            spinbox->setValue(*reinterpret_cast<sf::u32*>(field_ptr));
            connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), [this, obj_ptr, local_field](int val) mutable {
                sf::u32 v = static_cast<sf::u32>(val);
                sf::rtti::set_rtti_field_value(obj_ptr, const_cast<sf::rtti::rtti_field*>(&local_field), &v);
                if (local_field.setter)
                    QTimer::singleShot(0, this, [this, obj_ptr]() { this->draw_rtti(*obj_ptr); });
                emit rtti_changed();
            });
            m_Layout->addRow(field_name, spinbox);
            break;
        }
    case sf::rtti::rtti_type::F32:
        {
            auto* spinbox = new QDoubleSpinBox(this);
            spinbox->setRange(-1000000.0, 1000000.0);
            spinbox->setDecimals(3);
            spinbox->setValue(*reinterpret_cast<sf::f32*>(field_ptr));
            connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, obj_ptr, local_field](double val) mutable {
                sf::f32 v = static_cast<sf::f32>(val);
                sf::rtti::set_rtti_field_value(obj_ptr, const_cast<sf::rtti::rtti_field*>(&local_field), &v);
                if (local_field.setter)
                    QTimer::singleShot(0, this, [this, obj_ptr]() { this->draw_rtti(*obj_ptr); });
                emit rtti_changed();
            });
            m_Layout->addRow(field_name, spinbox);
            break;
        }
    case sf::rtti::rtti_type::STRING:
        {
            auto* lineedit = new QLineEdit(this);
            auto* str_ptr = reinterpret_cast<sf::stl::string*>(field_ptr);
            lineedit->setText(QString::fromLatin1(str_ptr->data(), str_ptr->size()));
            // For string edits (common for NameComponent) avoid re-drawing every keystroke which steals focus.
            connect(lineedit, &QLineEdit::editingFinished, [this, obj_ptr, local_field, lineedit]() mutable {
                sf::stl::string tmp(lineedit->text().toLatin1().data());
                sf::rtti::set_rtti_field_value(obj_ptr, const_cast<sf::rtti::rtti_field*>(&local_field), &tmp);
                if (local_field.setter)
                    QTimer::singleShot(0, this, [this, obj_ptr]() { this->draw_rtti(*obj_ptr); });
                emit rtti_changed();
            });
            m_Layout->addRow(field_name, lineedit);
            break;
        }
    default:
        {
            auto* label = new QLabel(QString::fromLatin1("<unsupported type>"), this);
            m_Layout->addRow(field_name, label);
            break;
        }
    case sf::rtti::rtti_type::VEC3:
        {
            auto create_spin = [this]() {
                auto* s = new QDoubleSpinBox(this);
                s->setRange(-1000000.0, 1000000.0);
                s->setDecimals(3);
                return s;
            };

            auto* x = create_spin();
            auto* y = create_spin();
            auto* z = create_spin();

            auto* vec = reinterpret_cast<sf::math::vec3*>(field_ptr);
            x->setValue(vec->x);
            y->setValue(vec->y);
            z->setValue(vec->z);

            auto* h_layout = new QHBoxLayout();
            h_layout->addWidget(new QLabel("X:"));
            h_layout->addWidget(x);
            h_layout->addWidget(new QLabel("Y:"));
            h_layout->addWidget(y);
            h_layout->addWidget(new QLabel("Z:"));
            h_layout->addWidget(z);

            auto commit = [obj_ptr, local_field, x, y, z, this]() mutable {
                std::array<sf::f32, 3> arr{static_cast<sf::f32>(x->value()), static_cast<sf::f32>(y->value()),
                                           static_cast<sf::f32>(z->value())};
                sf::rtti::set_rtti_field_value(obj_ptr, const_cast<sf::rtti::rtti_field*>(&local_field), &arr);
                // Refresh UI to pick up any dependent changes made by setter
                if (local_field.setter)
                    QTimer::singleShot(0, this, [this, obj = obj_ptr]() { this->draw_rtti(*obj); });
                emit rtti_changed();
            };

            connect(x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), commit);
            connect(y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), commit);
            connect(z, QOverload<double>::of(&QDoubleSpinBox::valueChanged), commit);

            QWidget* container = new QWidget(this);
            container->setLayout(h_layout);
            m_Layout->addRow(field_name, container);
            break;
        }
    }
}
