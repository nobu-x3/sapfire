#pragma once

#include <QAbstractItemModel>
#include <QRunnable>

class LoadAssetModelTask;

enum class EAssetType {
    Unknown,
    Texture,
    Mesh,
    Material,
    Shader,
    Scene,
};

namespace EFileModelField {
enum TYPE {
    NAME,
    PATH,
    TYPE,
    THUMBNAIL,
    META_PATH,
    COUNT,
};
}

// setModel to this class should only be done once OnFinishedLoading is fired
class SQFileSystemModel : public QAbstractItemModel {
    Q_OBJECT
    friend LoadAssetModelTask;

  public:
    enum CustomRoles {
        ThumbnailRole = Qt::UserRole + 1,
        RawDataRole = Qt::UserRole + 2,
    };

    static SQFileSystemModel &instance();
    explicit SQFileSystemModel(QObject *parent = nullptr);
    ~SQFileSystemModel();
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void init_model(QStringView project_path);
    void init_model_async(QStringView project_path);
    bool import_asset(const QString &path);

    inline bool is_loaded() const { return m_IsLoadingFinished; }

  signals:
    void loading_finished();

  public slots:
    void on_model_loading_finished();

  private:
    QVector<QString> m_Names;
    QVector<QString> m_Paths;
    QVector<EAssetType> m_AssetTypes;
    QVector<QPixmap> m_Thumbnails;
    QVector<QString> m_MetaPaths;
    bool m_IsLoadingFinished;
};
