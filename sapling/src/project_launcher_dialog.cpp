#include "project_launcher_dialog.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QScreen>
#include <QVBoxLayout>

ProjectLauncherDialog::ProjectLauncherDialog(QWidget* parent) : QDialog(parent) {
    setup_ui();
    load_projects();
}

void ProjectLauncherDialog::setup_ui() {
    setWindowTitle("Sapfire - Project Launcher");
    setMinimumSize(900, 600);
    QScreen* screen = QApplication::primaryScreen();
    QRect screen_geometry = screen->geometry();
    int x = (screen_geometry.width() - width()) / 2;
    int y = (screen_geometry.height() - height()) / 2;
    move(x, y);
    QHBoxLayout* main_layout = new QHBoxLayout(this);
    QVBoxLayout* left_layout = new QVBoxLayout();
    QLabel* title_label = new QLabel("Recent Projects");
    QFont title_font = title_label->font();
    title_font.setPointSize(14);
    title_font.setBold(true);
    title_label->setFont(title_font);
    left_layout->addWidget(title_label);
    m_ProjectList = new QListWidget();
    m_ProjectList->setIconSize(QSize(64, 64));
    m_ProjectList->setViewMode(QListView::ListMode);
    m_ProjectList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ProjectList->setStyleSheet("QListWidget { background-color: #2b2b2b; border: 1px solid #3c3c3c; }"
                                 "QListWidget::item { padding: 10px; color: #cccccc; }"
                                 "QListWidget::item:selected { background-color: #094771; }"
                                 "QListWidget::item:hover { background-color: #3c3c3c; }");
    left_layout->addWidget(m_ProjectList);
    QHBoxLayout* button_layout = new QHBoxLayout();
    m_NewButton = new QPushButton("New Project");
    m_OpenButton = new QPushButton("Open Project");
    m_BrowseButton = new QPushButton("Browse...");
    m_NewButton->setStyleSheet(
        "QPushButton { background-color: #094771; color: white; padding: 8px 16px; border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #0d5a8f; }"
        "QPushButton:pressed { background-color: #073d5c; }");
    m_OpenButton->setStyleSheet(
        "QPushButton { background-color: #094771; color: white; padding: 8px 16px; border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #0d5a8f; }"
        "QPushButton:pressed { background-color: #073d5c; }");
    m_BrowseButton->setStyleSheet(
        "QPushButton { background-color: #3c3c3c; color: white; padding: 8px 16px; border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #4c4c4c; }"
        "QPushButton:pressed { background-color: #2c2c2c; }");
    m_OpenButton->setEnabled(false);
    button_layout->addWidget(m_NewButton);
    button_layout->addWidget(m_OpenButton);
    button_layout->addStretch();
    button_layout->addWidget(m_BrowseButton);
    left_layout->addLayout(button_layout);
    QVBoxLayout* right_layout = new QVBoxLayout();
    QLabel* details_title = new QLabel("Project Details");
    details_title->setFont(title_font);
    right_layout->addWidget(details_title);
    m_DetailsPanel = new QWidget();
    QVBoxLayout* details_layout = new QVBoxLayout(m_DetailsPanel);
    m_ProjectNameLabel = new QLabel("No project selected");
    m_ProjectNameLabel->setWordWrap(true);
    QFont name_font = m_ProjectNameLabel->font();
    name_font.setPointSize(12);
    name_font.setBold(true);
    m_ProjectNameLabel->setFont(name_font);
    m_ProjectPathLabel = new QLabel("");
    m_ProjectPathLabel->setWordWrap(true);
    m_ProjectPathLabel->setStyleSheet("color: #888888;");
    m_ProjectVersionLabel = new QLabel("");
    m_ProjectModifiedLabel = new QLabel("");
    details_layout->addWidget(m_ProjectNameLabel);
    details_layout->addSpacing(10);
    details_layout->addWidget(new QLabel("Path:"));
    details_layout->addWidget(m_ProjectPathLabel);
    details_layout->addSpacing(10);
    details_layout->addWidget(new QLabel("Engine Version:"));
    details_layout->addWidget(m_ProjectVersionLabel);
    details_layout->addSpacing(10);
    details_layout->addWidget(new QLabel("Last Modified:"));
    details_layout->addWidget(m_ProjectModifiedLabel);
    details_layout->addStretch();
    m_DetailsPanel->setStyleSheet("QWidget { background-color: #2b2b2b; border: 1px solid #3c3c3c; padding: 15px; }");
    right_layout->addWidget(m_DetailsPanel);
    main_layout->addLayout(left_layout, 2);
    main_layout->addLayout(right_layout, 1);
    setStyleSheet("QDialog { background-color: #1e1e1e; }"
                  "QLabel { color: #cccccc; }");
    connect(m_ProjectList, &QListWidget::itemDoubleClicked, this, &ProjectLauncherDialog::on_project_double_clicked);
    connect(m_ProjectList, &QListWidget::currentItemChanged, this, &ProjectLauncherDialog::on_project_selected);
    connect(m_OpenButton, &QPushButton::clicked, this, &ProjectLauncherDialog::on_open_clicked);
    connect(m_NewButton, &QPushButton::clicked, this, &ProjectLauncherDialog::on_new_project_clicked);
    connect(m_BrowseButton, &QPushButton::clicked, this, &ProjectLauncherDialog::on_browse_clicked);
}

void ProjectLauncherDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    load_projects();
}

void ProjectLauncherDialog::load_projects() {
    m_ProjectList->clear();
    m_Projects = scan_projects();

    for (const auto& project : m_Projects) {
        QListWidgetItem* item = new QListWidgetItem(project.name);
        item->setData(Qt::UserRole, project.path);
        m_ProjectList->addItem(item);
    }

    if (m_Projects.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No projects found");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setForeground(QBrush(QColor("#888888")));
        m_ProjectList->addItem(item);
    }
}

QString ProjectLauncherDialog::get_projects_directory() const {
    QDir projects_dir(QDir::currentPath() + "/projects");
    if (!projects_dir.exists()) {
        projects_dir.mkpath(".");
    }
    return projects_dir.absolutePath();
}

QVector<ProjectInfo> ProjectLauncherDialog::scan_projects() {
    QVector<ProjectInfo> projects;
    QString projects_path = get_projects_directory();
    QDir projects_dir(projects_path);
    QStringList entries = projects_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QString project_path = projects_dir.absoluteFilePath(entry);
        QFileInfo project_info(project_path);
        QFile project_file(project_path + "/project.json");
        ProjectInfo info;
        info.name = entry;
        info.path = project_path;
        info.last_modified = project_info.lastModified().toString("yyyy-MM-dd hh:mm:ss");
        info.engine_version = "0.1.0"; // Default
        if (project_file.exists() && project_file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(project_file.readAll());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("name")) {
                    info.name = obj["name"].toString();
                }
                if (obj.contains("engine_version")) {
                    info.engine_version = obj["engine_version"].toString();
                }
            }
            project_file.close();
        }
        projects.append(info);
    }
    return projects;
}

void ProjectLauncherDialog::update_project_details(const ProjectInfo& project) {
    m_ProjectNameLabel->setText(project.name);
    m_ProjectPathLabel->setText(project.path);
    m_ProjectVersionLabel->setText(project.engine_version);
    m_ProjectModifiedLabel->setText(project.last_modified);
}

void ProjectLauncherDialog::on_project_double_clicked(QListWidgetItem* item) {
    if (!item || item->flags() & Qt::ItemIsSelectable) {
        on_open_clicked();
    }
}

void ProjectLauncherDialog::on_project_selected(QListWidgetItem* current, QListWidgetItem* /*previous*/) {
    if (current && (current->flags() & Qt::ItemIsSelectable)) {
        m_OpenButton->setEnabled(true);
        QString project_path = current->data(Qt::UserRole).toString();
        for (const auto& project : m_Projects) {
            if (project.path == project_path) {
                update_project_details(project);
                break;
            }
        }
    } else {
        m_OpenButton->setEnabled(false);
        m_ProjectNameLabel->setText("No project selected");
        m_ProjectPathLabel->setText("");
        m_ProjectVersionLabel->setText("");
        m_ProjectModifiedLabel->setText("");
    }
}

void ProjectLauncherDialog::on_open_clicked() {
    QListWidgetItem* current = m_ProjectList->currentItem();
    if (current && (current->flags() & Qt::ItemIsSelectable)) {
        m_SelectedProjectPath = current->data(Qt::UserRole).toString();
        m_CreateNewProject = false;
        accept();
    }
}

void ProjectLauncherDialog::on_new_project_clicked() {
    bool ok;
    QString project_name = QInputDialog::getText(this, "New Project", "Project name:", QLineEdit::Normal, "MyProject", &ok);
    if (ok && !project_name.isEmpty()) {
        if (project_name.contains(QRegularExpression("[^a-zA-Z0-9_-]"))) {
            QMessageBox::warning(this, "Invalid Name", "Project name can only contain letters, numbers, underscores, and hyphens.");
            return;
        }
        QString project_path = get_projects_directory() + "/" + project_name;
        QDir project_dir(project_path);
        if (project_dir.exists()) {
            QMessageBox::warning(this, "Project Exists", "A project with this name already exists.");
            return;
        }
        m_NewProjectName = project_name;
        m_SelectedProjectPath = project_path;
        m_CreateNewProject = true;
        accept();
    }
}

void ProjectLauncherDialog::on_browse_clicked() {
    QString project_path = QFileDialog::getExistingDirectory(this, "Select Project Directory", get_projects_directory());
    if (!project_path.isEmpty()) {
        m_SelectedProjectPath = project_path;
        m_CreateNewProject = false;
        accept();
    }
}
