#include "project_manager.h"
#include "splash_screen.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent) {
}

bool ProjectManager::create_project(const QString& projectPath, const QString& projectName, SplashScreen* splash) {
    if (splash) {
        splash->set_status("Creating project structure...");
        splash->set_progress(10);
    }
    if (!create_project_structure(projectPath, projectName)) {
        emit project_load_failed("Failed to create project structure");
        return false;
    }
    if (splash) {
        splash->set_status("Creating project configuration...");
        splash->set_progress(30);
    }
    if (!create_project_json(projectPath, projectName)) {
        emit project_load_failed("Failed to create project configuration");
        return false;
    }
    if (splash) {
        splash->set_status("Initializing project...");
        splash->set_progress(60);
    }
    m_CurrentProjectPath = projectPath;
    m_CurrentProjectName = projectName;
    if (splash) {
        splash->set_status("Project created successfully");
        splash->set_progress(100);
    }
    emit project_loaded();
    return true;
}

bool ProjectManager::load_project(const QString& projectPath, SplashScreen* splash) {
    if (splash) {
        splash->set_status("Validating project...");
        splash->set_progress(10);
    }
    if (!validate_project(projectPath)) {
        emit project_load_failed("Invalid project directory");
        return false;
    }
    if (splash) {
        splash->set_status("Loading project configuration...");
        splash->set_progress(20);
    }
    QFile projectFile(projectPath + "/project.json");
    if (!projectFile.open(QIODevice::ReadOnly)) {
        emit project_load_failed("Failed to open project.json");
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(projectFile.readAll());
    projectFile.close();
    if (!doc.isObject()) {
        emit project_load_failed("Invalid project.json format");
        return false;
    }
    QJsonObject obj = doc.object();
    QString projectName = obj["name"].toString();
    if (splash) {
        splash->set_status("Loading assets...");
        splash->set_progress(40);
    }
    // TODO: remove simulating asset loading
    QThread::msleep(2000);
    if (splash) {
        splash->set_status("Compiling shaders...");
        splash->set_progress(60);
    }
    // TODO: remove simulating shader compilation
    QThread::msleep(2000);
    if (splash) {
        splash->set_status("Initializing scene...");
        splash->set_progress(80);
    }
    // TODO: remove simulating scene initialization
    QThread::msleep(150);
    m_CurrentProjectPath = projectPath;
    m_CurrentProjectName = projectName;
    if (splash) {
        splash->set_status("Project loaded successfully");
        splash->set_progress(100);
    }
    emit project_loaded();
    return true;
}

bool ProjectManager::create_project_structure(const QString& projectPath, const QString& projectName) {
    QDir dir;
    if (!dir.mkpath(projectPath)) {
        return false;
    }
    QStringList subdirs = {
        "Assets",
        "Assets/Scenes",
        "Assets/Materials",
        "Assets/Textures",
        "Assets/Models",
        "Assets/Scripts",
        "Library",
        "Temp"
    };
    for (const QString& subdir : subdirs) {
        if (!dir.mkpath(projectPath + "/" + subdir)) {
            return false;
        }
    }
    QFile sceneFile(projectPath + "/Assets/Scenes/DefaultScene.scene");
    if (sceneFile.open(QIODevice::WriteOnly)) {
        sceneFile.write("{}"); // Empty scene JSON
        sceneFile.close();
    }
    return true;
}

bool ProjectManager::create_project_json(const QString& projectPath, const QString& projectName) {
    QJsonObject projectJson;
    projectJson["name"] = projectName;
    projectJson["engineVersion"] = "0.1.0";
    projectJson["defaultScene"] = "Assets/Scenes/DefaultScene.scene";
    QJsonDocument doc(projectJson);
    QFile file(projectPath + "/project.json");
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ProjectManager::validate_project(const QString& projectPath) {
    QDir dir(projectPath);
    if (!dir.exists()) {
        return false;
    }
    QFile projectFile(projectPath + "/project.json");
    if (!projectFile.exists()) {
        return false;
    }
    return true;
}
