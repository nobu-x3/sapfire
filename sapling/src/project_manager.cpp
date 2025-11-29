#include "project_manager.h"
#include "splash_screen.h"
#include "widgets/file_system_model.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

ProjectManager::ProjectManager(QObject* parent) : QObject(parent) {}

bool ProjectManager::create_project(const QString& project_path, const QString& project_name, SplashScreen* splash) {
    if (splash) {
        splash->set_status("Creating project structure...");
        splash->set_progress(10);
    }
    if (!create_project_structure(project_path, project_name)) {
        emit project_load_failed("Failed to create project structure");
        return false;
    }
    if (splash) {
        splash->set_status("Creating project configuration...");
        splash->set_progress(30);
    }
    if (!create_project_json(project_path, project_name)) {
        emit project_load_failed("Failed to create project configuration");
        return false;
    }
    if (splash) {
        splash->set_status("Initializing project...");
        splash->set_progress(60);
    }
    m_CurrentProjectPath = project_path;
    m_CurrentProjectName = project_name;
    if (splash) {
        splash->set_status("Project created successfully");
        splash->set_progress(100);
    }
    emit project_loaded();
    return true;
}

bool ProjectManager::load_project(const QString& project_path, SplashScreen* splash) {
    if (splash) {
        splash->set_status("Validating project...");
        splash->set_progress(10);
    }
    if (!validate_project(project_path)) {
        emit project_load_failed("Invalid project directory");
        return false;
    }
    if (splash) {
        splash->set_status("Loading project configuration...");
        splash->set_progress(20);
    }
    QFile project_file(project_path + "/project.json");
    if (!project_file.open(QIODevice::ReadOnly)) {
        emit project_load_failed("Failed to open project.json");
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(project_file.readAll());
    project_file.close();
    if (!doc.isObject()) {
        emit project_load_failed("Invalid project.json format");
        return false;
    }
    QJsonObject obj = doc.object();
    QString project_name = obj["name"].toString();
    if (splash) {
        splash->set_status("Loading assets...");
        splash->set_progress(40);
    }
    if (!project_path.isEmpty()) {
        SQFileSystemModel::instance().init_model(project_path);
    }
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
    m_CurrentProjectPath = project_path;
    m_CurrentProjectName = project_name;
    if (splash) {
        splash->set_status("Project loaded successfully");
        splash->set_progress(100);
    }
    emit project_loaded();
    return true;
}

bool ProjectManager::create_project_structure(const QString& project_path, const QString& project_name) {
    QDir dir;
    if (!dir.mkpath(project_path)) {
        return false;
    }
    QStringList subdirs = {"assets",        "assets/scenes",  "assets/materials", "assets/textures",
                           "assets/models", "assets/scripts", "library",          "temp"};
    for (const QString& subdir : subdirs) {
        if (!dir.mkpath(project_path + "/" + subdir)) {
            return false;
        }
    }
    QFile scene_file(project_path + "/assets/scenes/default_scene.scene");
    if (scene_file.open(QIODevice::WriteOnly)) {
        scene_file.write("{}"); // Empty scene JSON
        scene_file.close();
    }
    return true;
}

bool ProjectManager::create_project_json(const QString& project_path, const QString& project_name) {
    QJsonObject project_json;
    project_json["name"] = project_name;
    project_json["engine_version"] = "0.1.0";
    project_json["defaultScene"] = "Assets/Scenes/DefaultScene.scene";
    QJsonDocument doc(project_json);
    QFile file(project_path + "/project.json");
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ProjectManager::validate_project(const QString& project_path) {
    QDir dir(project_path);
    if (!dir.exists()) {
        return false;
    }
    QFile project_file(project_path + "/project.json");
    if (!project_file.exists()) {
        return false;
    }
    return true;
}
