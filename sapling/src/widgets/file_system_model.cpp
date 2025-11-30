#include "widgets/file_system_model.h"
#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QRunnable>
#include <QThreadPool>
#include <core/logger.h>
#include <qnamespace.h>
#include <qvariant.h>

QSet<QString> ignored_extensions{".json", ".meta"};

SQFileSystemModel& SQFileSystemModel::instance() {
    static SQFileSystemModel model{};
    return model;
}

EAssetType get_assetpath_from_path(const QString& path) {
    if (path.endsWith(".jpg") || path.endsWith(".png") || path.endsWith(".dds") || path.endsWith(".bmp"))
        return EAssetType::Texture;
    if (path.endsWith(".obj") || path.endsWith(".fbx") || path.endsWith(".gltf") || path.endsWith(".glb"))
        return EAssetType::Mesh;
    if (path.endsWith(".mat"))
        return EAssetType::Material;
    if (path.endsWith(".spv"))
        return EAssetType::Shader;
    if (path.endsWith(".scene"))
        return EAssetType::Scene;
    return EAssetType::Unknown;
}

QImage create_thumbnail(const QString& path, EAssetType& type, QFileInfo& fi) {
    QImage thumbnail;
    if (type == EAssetType::Texture) {
        QImage img(path);
        if (!img.isNull()) {
            thumbnail = img.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }
    // For other types, create a default placeholder
    if (thumbnail.isNull()) {
        thumbnail = QImage(128, 128, QImage::Format_ARGB32);
        thumbnail.fill(QColor(60, 60, 60));

        QPainter p(&thumbnail);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 10));
        p.drawText(thumbnail.rect(), Qt::AlignCenter, "No Preview\n" + fi.suffix());
    }

    return thumbnail;
}

void create_default_meta_for_type(EAssetType type, const QString& meta_full_path) {
    if (type == EAssetType::Scene)
        return;
    QFile meta_file{meta_full_path};
    bool is_open = meta_file.open(QFile::OpenModeFlag::ReadWrite);
    switch (type) {
    case EAssetType::Texture:
        meta_file.write("{\"deps\":[]}");
        break;
    case EAssetType::Mesh:
        meta_file.write("{\"deps\":[]}");
        break;
    case EAssetType::Material:
        meta_file.write("{\"deps\":[]}");
        break;
    case EAssetType::Shader:
        meta_file.write("{\"deps\":[]}");
        break;
    default:
        break;
    }
    meta_file.close();
}

class LoadAssetModelTask final : public QRunnable {
public:
    SQFileSystemModel* model;
    QString project_path;

    LoadAssetModelTask(SQFileSystemModel* fs_model, QStringView project_path) :
        QRunnable(), model(fs_model), project_path(project_path.toString()) {}

    void run() override {
        QVector<QString> names, paths;
        QVector<EAssetType> types;
        QVector<QImage> thumbnails;
        QVector<QString> meta_paths;
        QDir dir = {project_path};
        dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        QDirIterator it = {dir, QDirIterator::Subdirectories};
        while (it.hasNext()) {
            QString path = it.next();
            EAssetType type = get_assetpath_from_path(path);
            QFileInfo fi = it.fileInfo();
            QString filename = fi.fileName();
            bool should_ignore = false;
            for (auto&& ext_to_ignore : ignored_extensions) {
                if (filename.endsWith(ext_to_ignore)) {
                    should_ignore = true;
                    break;
                }
            }
            if (should_ignore)
                continue;
            names.append(filename);
            QString abs_path = fi.absoluteFilePath();
            paths.append(abs_path);
            types.append(type);
            QString meta_path = QString("%1.meta").arg(abs_path);
            QFile meta_file{meta_path};
            if (!meta_file.exists()) {
                create_default_meta_for_type(type, meta_path);
            }
            meta_paths.append(meta_path);
            QImage thumbnail = create_thumbnail(path, type, fi);
            thumbnails.append(thumbnail);
        }
        SQFileSystemModel* m = model;
        QMetaObject::invokeMethod(
            QApplication::instance(),
            [m, names = std::move(names), paths = std::move(paths), types = std::move(types), thumbnails = std::move(thumbnails),
             meta_paths = std::move(meta_paths)]() {
                int row = m->rowCount();
                m->beginInsertRows(QModelIndex(), row, row + names.size() - 1);
                m->m_Names.append(names);
                m->m_Paths.append(paths);
                m->m_AssetTypes.append(types);
                m->m_MetaPaths.append(meta_paths);
                m->m_Thumbnails.reserve(m->m_Thumbnails.size() + thumbnails.size());
                for (const QImage& img : thumbnails) {
                    m->m_Thumbnails.append(QPixmap::fromImage(img));
                }
                m->endInsertRows();
                m->on_model_loading_finished();
            },
            Qt::QueuedConnection);
    }
};

SQFileSystemModel::SQFileSystemModel(QObject* parent) : QAbstractItemModel(parent), m_IsLoadingFinished(false) {
    m_Names.reserve(64);
    m_Paths.reserve(64);
    m_AssetTypes.reserve(64);
    m_Thumbnails.reserve(64);
    m_MetaPaths.reserve(64);
}

SQFileSystemModel::~SQFileSystemModel() {}

void SQFileSystemModel::init_model(QStringView project_path) {
    QDir dir = {project_path.toLatin1()};
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QDirIterator it = {dir, QDirIterator::Subdirectories};
    while (it.hasNext()) {
        QString path = it.next();
        EAssetType type = get_assetpath_from_path(path);
        QFileInfo fi = it.fileInfo();
        QString filename = fi.fileName();
        bool should_ignore = false;
        for (auto&& ext_to_ignore : ignored_extensions) {
            if (filename.endsWith(ext_to_ignore)) {
                should_ignore = true;
                break;
            }
        }
        if (should_ignore)
            continue;
        m_Names.append(filename);
        QString abs_path = fi.absoluteFilePath();
        m_Paths.append(abs_path);
        m_AssetTypes.append(type);
        QString meta_path = QString("%1.meta").arg(abs_path);
        QFile meta_file{meta_path};
        if (!meta_file.exists()) {
            create_default_meta_for_type(type, meta_path);
        }
        m_MetaPaths.append(meta_path);
        QImage thumbnail = create_thumbnail(path, type, fi);
        m_Thumbnails.append(QPixmap::fromImage(thumbnail));
    }
    m_IsLoadingFinished = true;
    emit loading_finished();
}

void SQFileSystemModel::init_model_async(QStringView project_path) {
    LoadAssetModelTask* load_task = new LoadAssetModelTask(this, project_path);
    QThreadPool::globalInstance()->start(load_task);
}

bool SQFileSystemModel::import_asset(const QString& path) {
    QFileInfo fi{path};
    EAssetType type = get_assetpath_from_path(fi.fileName());
    auto&& path_it = std::find(m_Paths.begin(), m_Paths.end(), fi.absoluteFilePath());
    int row = rowCount();
    if (path_it == m_Paths.end()) {
        insertRow(row);
    } else {
        row = std::distance(m_Paths.begin(), path_it);
    }
    QModelIndex name_id = index(row, EFileModelField::NAME);
    QModelIndex path_id = index(row, EFileModelField::PATH);
    QModelIndex type_id = index(row, EFileModelField::TYPE);
    QModelIndex thumbnail_id = index(row, EFileModelField::THUMBNAIL);
    QModelIndex meta_path_id = index(row, EFileModelField::META_PATH);
    QString abs_path = fi.absoluteFilePath();
    QString meta_path = QString("%1.meta").arg(abs_path);
    QFile meta_file{meta_path};
    if (!meta_file.exists()) {
        create_default_meta_for_type(type, meta_path);
    }
    bool result = setData(name_id, fi.fileName());
    result &= setData(path_id, abs_path);
    result &= setData(type_id, QVariant::fromValue(type));
    QImage img = create_thumbnail(path, type, fi);
    result &= setData(thumbnail_id, QPixmap::fromImage(img));
    result &= setData(meta_path_id, meta_path);
    if (!result) {
        removeRow(row);
        CLIENT_ERROR("Failed to import asset at path: {}", fi.absoluteFilePath().toStdString());
        return false;
    }
    return true;
}

QVariant SQFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Orientation::Horizontal) {
        if (role == Qt::DisplayRole) {
            if (section >= EFileModelField::COUNT || section < 0)
                return QVariant();
            switch (section) {
            case EFileModelField::NAME:
                return QVariant("Name");
            case EFileModelField::PATH:
                return QVariant("Path");
            case EFileModelField::TYPE:
                return QVariant("Type");
            case EFileModelField::THUMBNAIL:
                return QVariant("Thumbnail");
            case EFileModelField::META_PATH:
                return QVariant("Meta path");
            }
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant get_type_variant(EAssetType type) {
    switch (type) {
    case EAssetType::Texture:
        return "Texture";
    case EAssetType::Mesh:
        return "Mesh";
    case EAssetType::Material:
        return "Material";
    case EAssetType::Scene:
        return "Scene";
    case EAssetType::Shader:
        return "Shader";
    default:
        return "Unknown";
    }
}

QVariant SQFileSystemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.column() >= EFileModelField::COUNT || index.column() < 0)
        return QVariant();
    int col = index.column();
    int row = index.row();
    if (role == Qt::DisplayRole) {
        switch (col) {
        case EFileModelField::TYPE:
            return get_type_variant(m_AssetTypes[row]);
        case EFileModelField::PATH:
            return m_Paths[row];
        case EFileModelField::NAME:
            return m_Names[row];
        case EFileModelField::META_PATH:
            return m_MetaPaths[row];
        }
    } else if (role == ThumbnailRole) {
        if (row < m_Thumbnails.size()) {
            return m_Thumbnails[row];
        }
    } else if (role == Qt::ToolTipRole) {
        return m_Paths[row];
    } else if (role == Qt::EditRole || role == RawDataRole) {
        switch (col) {
        case EFileModelField::TYPE:
            return QVariant::fromValue(m_AssetTypes[row]);
        case EFileModelField::NAME:
            return m_Names[row];
        case EFileModelField::PATH:
            return m_Paths[row];
        case EFileModelField::THUMBNAIL:
            if (row < m_Thumbnails.size()) {
                return m_Thumbnails[row];
            }
            break;
        case EFileModelField::META_PATH:
            return m_MetaPaths[row];
        }
    }
    return QVariant();
}

bool SQFileSystemModel::insertRows(int row, int count, const QModelIndex& parent) {
    beginInsertRows(QModelIndex(), row, row + count - 1);
    auto&& names_it = m_Names.begin() + row;
    auto&& paths_it = m_Paths.begin() + row;
    auto&& types_it = m_AssetTypes.begin() + row;
    auto&& thumbnails_it = m_Thumbnails.begin() + row;
    auto&& meta_it = m_MetaPaths.begin() + row;
    for (int i = 0; i < count; ++i) {
        m_Names.insert(names_it, "");
        m_Paths.insert(paths_it, "");
        m_AssetTypes.insert(types_it, EAssetType::Unknown);
        m_Thumbnails.insert(thumbnails_it, QPixmap());
        m_MetaPaths.insert(meta_it, "");
    }
    endInsertRows();
    return true;
}

QModelIndex SQFileSystemModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() || // No child items
        row < 0 || row >= rowCount() || column < 0 || column >= columnCount()) {
        return QModelIndex(); // Invalid index
    }
    return createIndex(row, column);
}

QModelIndex SQFileSystemModel::parent(const QModelIndex& index) const { return QModelIndex(); }

int SQFileSystemModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) // Child items have 0 rows
        return 0;
    return m_Names.count();
}

int SQFileSystemModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) // Child items have 0 columns
        return 0;
    return EFileModelField::COUNT;
}

bool SQFileSystemModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid())
        return false;
    int column = index.column();
    int row = index.row();
    if (column >= EFileModelField::COUNT || column < 0 || row > rowCount() || row < 0)
        return false;
    bool changed = false;
    if (column == EFileModelField::NAME) {
        m_Names[row] = value.toString();
        changed = true;
    } else if (column == EFileModelField::PATH) {
        m_Paths[row] = value.toString();
        changed = true;
    } else if (column == EFileModelField::TYPE) {
        m_AssetTypes[row] = value.value<EAssetType>();
        changed = true;
    } else if (column == EFileModelField::THUMBNAIL) {
        m_Thumbnails[row] = value.value<QPixmap>();
        changed = true;
    } else if (column == EFileModelField::META_PATH) {
        m_MetaPaths[row] = value.toString();
        changed = true;
    }
    if (changed)
        emit dataChanged(index, index, {role});
    return changed;
}

void SQFileSystemModel::on_model_loading_finished() {
    m_IsLoadingFinished = true;
    emit loading_finished();
}
