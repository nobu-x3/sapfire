#pragma once
#include <QFormLayout>
#include <QSpinBox>
#include <QWidget>
#include "Sapfire.h"

class RttiDrawer : public QWidget {
    Q_OBJECT
public:
    explicit RttiDrawer(QWidget* parent = nullptr);
    void draw_rtti(sf::rtti::rtti_object& rtti_obj);
    void clear();

private:
    void draw_field(sf::rtti::rtti_object& rtti_obj, sf::rtti::rtti_field& field);

private:
    QFormLayout* m_Layout{nullptr};
};
