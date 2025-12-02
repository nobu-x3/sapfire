#pragma once

#include <QModelIndexList>
#include <QWidget>

class QListView;
class QTableView;
class QLabel;
class QPushButton;
class QLineEdit;
class SQFileSystemModel;
class SQAssetFilterProxyModel;

class AssetBrowserWidget final : public QWidget {
    Q_OBJECT
public:
    explicit AssetBrowserWidget(SQFileSystemModel* model, QWidget* parent = nullptr);
    ~AssetBrowserWidget();

    enum class ViewType : char { List, Table, Loading };
    void set_view_type(ViewType type);
    QModelIndexList get_selected_assets() const;
    void import_new_asset();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void on_loading_finished();
    void on_search_changed(const QString& text);
    void on_filter_button_toggled();
    void on_import_new_action_pressed(bool);

private:
    void update_filter();

private:
    SQFileSystemModel* m_FSModel;
    SQAssetFilterProxyModel* m_FilterProxy;
    QListView* m_ListView;
    QTableView* m_TableView;
    QLabel* m_LoadingLabel;
    QPushButton* m_ViewSwitchButton;
    QLineEdit* m_SearchBox;
    QPushButton* m_TextureFilter;
    QPushButton* m_MeshFilter;
    QPushButton* m_MaterialFilter;
    QPushButton* m_ShaderFilter;
    QPushButton* m_SceneFilter;
    QAction* m_ImportAssetAction;
    bool m_HasFinishedLoading;
    ViewType m_CurrentViewType;
};
