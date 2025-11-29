#include "widgets/asset_filter_proxy.h"
#include "widgets/file_system_model.h"

SQAssetFilterProxyModel::SQAssetFilterProxyModel(SQFileSystemModel* model, QObject* parent) :
    QSortFilterProxyModel(parent), m_CurrentAssetTypeFilter(EAssetFilterType::All), m_SearchFilter("") {
    setSourceModel(model);
}

SQAssetFilterProxyModel::~SQAssetFilterProxyModel() {}

void SQAssetFilterProxyModel::set_asset_filter_type(EAssetFilterType::TYPE type) {
    m_CurrentAssetTypeFilter = type;
    invalidateFilter();
}

void SQAssetFilterProxyModel::set_search_filter(const QString& filter) {
    m_SearchFilter = filter;
    invalidateFilter();
}

bool SQAssetFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    // Check type filter
    bool type_match = true;
    if (m_CurrentAssetTypeFilter != EAssetFilterType::All) {
        QModelIndex type_index = sourceModel()->index(source_row, EFileModelField::TYPE, source_parent);
        if (!type_index.isValid())
            return false;
        EAssetType asset_type = type_index.data(SQFileSystemModel::RawDataRole).value<EAssetType>();
        switch (asset_type) {
        case EAssetType::Texture:
            type_match = (m_CurrentAssetTypeFilter & EAssetFilterType::Texture);
            break;
        case EAssetType::Mesh:
            type_match = (m_CurrentAssetTypeFilter & EAssetFilterType::Mesh);
            break;
        case EAssetType::Material:
            type_match = (m_CurrentAssetTypeFilter & EAssetFilterType::Material);
            break;
        case EAssetType::Shader:
            type_match = (m_CurrentAssetTypeFilter & EAssetFilterType::Shader);
            break;
        case EAssetType::Scene:
            type_match = (m_CurrentAssetTypeFilter & EAssetFilterType::Scene);
            break;
        default:
            type_match = false;
            break;
        }
    }

    if (!type_match)
        return false;

    // Check search filter
    if (!m_SearchFilter.isEmpty()) {
        QModelIndex name_index = sourceModel()->index(source_row, EFileModelField::NAME, source_parent);
        if (!name_index.isValid())
            return false;
        QString name = name_index.data(Qt::DisplayRole).toString();
        return name.contains(m_SearchFilter, Qt::CaseInsensitive);
    }

    return true;
}
