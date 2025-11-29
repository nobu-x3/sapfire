#include "project_launcher_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

ProjectLauncherDialog::ProjectLauncherDialog(QWidget* parent)
    : QDialog(parent) {
    setup_ui();
    load_projects();
}

void ProjectLauncherDialog::setup_ui() {
    setWindowTitle("Sapfire - Project Launcher");
    setMinimumSize(900, 600);
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    QVBoxLayout* leftLayout = new QVBoxLayout();
    QLabel* titleLabel = new QLabel("Recent Projects");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    leftLayout->addWidget(titleLabel);
    m_ProjectList = new QListWidget();
    m_ProjectList->setIconSize(QSize(64, 64));
    m_ProjectList->setViewMode(QListView::ListMode);
    m_ProjectList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ProjectList->setStyleSheet(
        "QListWidget { background-color: #2b2b2b; border: 1px solid #3c3c3c; }"
        "QListWidget::item { padding: 10px; color: #cccccc; }"
        "QListWidget::item:selected { background-color: #094771; }"
        "QListWidget::item:hover { background-color: #3c3c3c; }"
    );
    leftLayout->addWidget(m_ProjectList);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_NewButton = new QPushButton("New Project");
    m_OpenButton = new QPushButton("Open Project");
    m_BrowseButton = new QPushButton("Browse...");
    m_NewButton->setStyleSheet(
        "QPushButton { background-color: #094771; color: white; padding: 8px 16px; border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #0d5a8f; }"
        "QPushButton:pressed { background-color: #073d5c; }"
    );
    m_OpenButton->setStyleSheet(
        "QPushButton { background-color: #094771; color: white; padding: 8px 16px; border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #0d5a8f; }"
        "QPushButton:pressed { background-color: #073d5c; }"
    );
    m_BrowseButton->setStyleSheet(
        "QPushButton { background-color: #3c3c3c; color: white; padding: 8px 16px; border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #4c4c4c; }"
        "QPushButton:pressed { background-color: #2c2c2c; }"
    );
    m_OpenButton->setEnabled(false);
    buttonLayout->addWidget(m_NewButton);
    buttonLayout->addWidget(m_OpenButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_BrowseButton);
    leftLayout->addLayout(buttonLayout);
    QVBoxLayout* rightLayout = new QVBoxLayout();
    QLabel* detailsTitle = new QLabel("Project Details");
    detailsTitle->setFont(titleFont);
    rightLayout->addWidget(detailsTitle);
    m_DetailsPanel = new QWidget();
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_DetailsPanel);
    m_ProjectNameLabel = new QLabel("No project selected");
    m_ProjectNameLabel->setWordWrap(true);
    QFont nameFont = m_ProjectNameLabel->font();
    nameFont.setPointSize(12);
    nameFont.setBold(true);
    m_ProjectNameLabel->setFont(nameFont);
    m_ProjectPathLabel = new QLabel("");
    m_ProjectPathLabel->setWordWrap(true);
    m_ProjectPathLabel->setStyleSheet("color: #888888;");
    m_ProjectVersionLabel = new QLabel("");
    m_ProjectModifiedLabel = new QLabel("");
    detailsLayout->addWidget(m_ProjectNameLabel);
    detailsLayout->addSpacing(10);
    detailsLayout->addWidget(new QLabel("Path:"));
    detailsLayout->addWidget(m_ProjectPathLabel);
    detailsLayout->addSpacing(10);
    detailsLayout->addWidget(new QLabel("Engine Version:"));
    detailsLayout->addWidget(m_ProjectVersionLabel);
    detailsLayout->addSpacing(10);
    detailsLayout->addWidget(new QLabel("Last Modified:"));
    detailsLayout->addWidget(m_ProjectModifiedLabel);
    detailsLayout->addStretch();
    m_DetailsPanel->setStyleSheet(
        "QWidget { background-color: #2b2b2b; border: 1px solid #3c3c3c; padding: 15px; }"
    );
    rightLayout->addWidget(m_DetailsPanel);
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 1);
    setStyleSheet(
        "QDialog { background-color: #1e1e1e; }"
        "QLabel { color: #cccccc; }"
    );
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
    QDir projectsDir(QDir::currentPath() + "/projects");
    if (!projectsDir.exists()) {
        projectsDir.mkpath(".");
    }
    return projectsDir.absolutePath();
}

QVector<ProjectInfo> ProjectLauncherDialog::scan_projects() {
    QVector<ProjectInfo> projects;
    QString projectsPath = get_projects_directory();
    QDir projectsDir(projectsPath);
    QStringList entries = projectsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QString projectPath = projectsDir.absoluteFilePath(entry);
        QFileInfo projectInfo(projectPath);
        QFile projectFile(projectPath + "/project.json");
        ProjectInfo info;
        info.name = entry;
        info.path = projectPath;
        info.lastModified = projectInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");
        info.engineVersion = "0.1.0"; // Default
        if (projectFile.exists() && projectFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(projectFile.readAll());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("name")) {
                    info.name = obj["name"].toString();
                }
                if (obj.contains("engineVersion")) {
                    info.engineVersion = obj["engineVersion"].toString();
                }
            }
            projectFile.close();
        }
        projects.append(info);
    }
    return projects;
}

void ProjectLauncherDialog::update_project_details(const ProjectInfo& project) {
    m_ProjectNameLabel->setText(project.name);
    m_ProjectPathLabel->setText(project.path);
    m_ProjectVersionLabel->setText(project.engineVersion);
    m_ProjectModifiedLabel->setText(project.lastModified);
}

void ProjectLauncherDialog::on_project_double_clicked(QListWidgetItem* item) {
    if (!item || item->flags() & Qt::ItemIsSelectable) {
        on_open_clicked();
    }
}

void ProjectLauncherDialog::on_project_selected(QListWidgetItem* current, QListWidgetItem* previous) {
    Q_UNUSED(previous);
    if (current && (current->flags() & Qt::ItemIsSelectable)) {
        m_OpenButton->setEnabled(true);
        QString projectPath = current->data(Qt::UserRole).toString();
        for (const auto& project : m_Projects) {
            if (project.path == projectPath) {
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
    QString projectName = QInputDialog::getText(
        this,
        "New Project",
        "Project name:",
        QLineEdit::Normal,
        "MyProject",
        &ok
    );
    if (ok && !projectName.isEmpty()) {
        if (projectName.contains(QRegularExpression("[^a-zA-Z0-9_-]"))) {
            QMessageBox::warning(this, "Invalid Name", "Project name can only contain letters, numbers, underscores, and hyphens.");
            return;
        }
        QString projectPath = get_projects_directory() + "/" + projectName;
        QDir projectDir(projectPath);
        if (projectDir.exists()) {
            QMessageBox::warning(this, "Project Exists", "A project with this name already exists.");
            return;
        }
        m_NewProjectName = projectName;
        m_SelectedProjectPath = projectPath;
        m_CreateNewProject = true;
        accept();
    }
}

void ProjectLauncherDialog::on_browse_clicked() {
    QString projectPath = QFileDialog::getExistingDirectory(
        this,
        "Select Project Directory",
        get_projects_directory()
    );
    if (!projectPath.isEmpty()) {
        m_SelectedProjectPath = projectPath;
        m_CreateNewProject = false;
        accept();
    }
}
