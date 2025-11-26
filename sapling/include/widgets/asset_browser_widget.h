#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include "Sapfire.h"

// AssetBrowserWidget: Browse, import, and manage project assets
class AssetBrowserWidget : public QWidget {
    Q_OBJECT

public:
    explicit AssetBrowserWidget(QWidget* parent = nullptr);

    void refresh();

private slots:
    void on_import_asset_clicked();
    void on_filter_changed(const QString& text);
    void on_asset_type_changed(int index);
    void on_asset_double_clicked(QListWidgetItem* item);

private:
    void populate_asset_list();
    void filter_assets();

private:
    QListWidget* m_AssetList{nullptr};
    QLineEdit* m_SearchBox{nullptr};
    QComboBox* m_TypeFilter{nullptr};
    QPushButton* m_ImportButton{nullptr};
    sf::stl::string m_CurrentFilter;
    int m_CurrentTypeFilter{0}; // 0 = All, 1 = Meshes, 2 = Textures, etc.
};
