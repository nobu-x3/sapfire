#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class SQFileSystemModel;

namespace EAssetFilterType {
    enum TYPE {
        Texture = 1 << 0,
        Mesh = 1 << 1,
        Material = 1 << 2,
        Shader = 1 << 3,
        Scene = 1 << 4,
        All = Texture | Mesh | Material | Shader | Scene,
    };
}

class SQAssetFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SQAssetFilterProxyModel(SQFileSystemModel* model, QObject* parent = nullptr);
    ~SQAssetFilterProxyModel();
    void set_asset_filter_type(EAssetFilterType::TYPE type);
    void set_search_filter(const QString& filter);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    EAssetFilterType::TYPE m_CurrentAssetTypeFilter;
    QString m_SearchFilter;
};
