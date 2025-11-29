#pragma once

#include <QDialog>
#include <QString>
#include <QVector>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

struct ProjectInfo {
    QString name;
    QString path;
    QString lastModified;
    QString engineVersion;
};

class ProjectLauncherDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProjectLauncherDialog(QWidget* parent = nullptr);
    ~ProjectLauncherDialog() override = default;

    QString selected_project_path() const { return m_SelectedProjectPath; }
    bool should_create_new_project() const { return m_CreateNewProject; }
    QString new_project_name() const { return m_NewProjectName; }

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void on_project_double_clicked(QListWidgetItem* item);
    void on_project_selected(QListWidgetItem* current, QListWidgetItem* previous);
    void on_open_clicked();
    void on_new_project_clicked();
    void on_browse_clicked();

private:
    void setup_ui();
    void load_projects();
    void update_project_details(const ProjectInfo& project);
    QString get_projects_directory() const;
    QVector<ProjectInfo> scan_projects();

private:
    QListWidget* m_ProjectList{nullptr};
    QPushButton* m_OpenButton{nullptr};
    QPushButton* m_NewButton{nullptr};
    QPushButton* m_BrowseButton{nullptr};
    QLabel* m_ProjectNameLabel{nullptr};
    QLabel* m_ProjectPathLabel{nullptr};
    QLabel* m_ProjectVersionLabel{nullptr};
    QLabel* m_ProjectModifiedLabel{nullptr};
    QWidget* m_DetailsPanel{nullptr};

    QString m_SelectedProjectPath;
    bool m_CreateNewProject{false};
    QString m_NewProjectName;
    QVector<ProjectInfo> m_Projects;
};
