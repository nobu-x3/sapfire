#pragma once

#include <QString>
#include <QObject>

class SplashScreen;

class ProjectManager : public QObject {
    Q_OBJECT

public:
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager() override = default;

    bool create_project(const QString& projectPath, const QString& projectName, SplashScreen* splash = nullptr);
    bool load_project(const QString& projectPath, SplashScreen* splash = nullptr);

    QString current_project_path() const { return m_CurrentProjectPath; }
    QString current_project_name() const { return m_CurrentProjectName; }

signals:
    void project_loaded();
    void project_load_failed(const QString& error);

private:
    bool create_project_structure(const QString& projectPath, const QString& projectName);
    bool create_project_json(const QString& projectPath, const QString& projectName);
    bool validate_project(const QString& projectPath);

private:
    QString m_CurrentProjectPath;
    QString m_CurrentProjectName;
};
