#pragma once

#include <QObject>
#include <QString>

class SplashScreen;

class ProjectManager : public QObject {
    Q_OBJECT

public:
    static ProjectManager& instance() {
        static ProjectManager s_Instance;
        return s_Instance;
    }

    bool create_project(const QString& projectPath, const QString& projectName, SplashScreen* splash = nullptr);
    bool load_project(const QString& projectPath, SplashScreen* splash = nullptr);

    QString current_project_path() const { return m_CurrentProjectPath; }
    QString current_project_name() const { return m_CurrentProjectName; }

signals:
    void project_loaded();
    void project_load_failed(const QString& error);

private:
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager() override = default;

    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;

    bool create_project_structure(const QString& projectPath, const QString& projectName);
    bool create_project_json(const QString& projectPath, const QString& projectName);
    bool validate_project(const QString& projectPath);

private:
    QString m_CurrentProjectPath;
    QString m_CurrentProjectName;
};
