#include "splash_screen.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QPainter>
#include <QScreen>
#include <QVBoxLayout>

SplashScreen::SplashScreen(QWidget* parent) : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
    setup_ui();
}

void SplashScreen::setup_ui() {
    setFixedSize(800, 450);
    setAttribute(Qt::WA_TranslucentBackground);
    QScreen* screen = QApplication::primaryScreen();
    QRect screen_geometry = screen->geometry();
    int x = (screen_geometry.width() - width()) / 2;
    int y = (screen_geometry.height() - height()) / 2;
    move(x, y);
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    QWidget* container = new QWidget();
    container->setStyleSheet("QWidget { background-color: #000000; }");
    QVBoxLayout* container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    container_layout->addStretch(2);
    m_LogoLabel = new QLabel();
    m_LogoLabel->setAlignment(Qt::AlignCenter);
    QString logo_path = QApplication::applicationDirPath() + "/editor_assets/logo.png";
    if (QFile::exists(logo_path)) {
        m_LogoPixmap.load(logo_path);
        if (!m_LogoPixmap.isNull()) {
            QPixmap scaledLogo = m_LogoPixmap.scaled(400, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_LogoLabel->setPixmap(scaledLogo);
        } else {
            m_LogoLabel->setText("SAPFIRE");
            m_LogoLabel->setStyleSheet("color: #ffffff; font-size: 72px; font-weight: bold; letter-spacing: 4px;");
        }
    } else {
        m_LogoLabel->setText("SAPFIRE");
        m_LogoLabel->setStyleSheet("color: #ffffff; font-size: 72px; font-weight: bold; letter-spacing: 4px;");
    }
    container_layout->addWidget(m_LogoLabel);
    container_layout->addSpacing(15);
    m_VersionLabel = new QLabel("Version 0.1.0");
    m_VersionLabel->setAlignment(Qt::AlignCenter);
    m_VersionLabel->setStyleSheet("color: #606060; font-size: 11px;");
    container_layout->addWidget(m_VersionLabel);
    container_layout->addStretch(3);
    m_StatusLabel = new QLabel("Initializing...");
    m_StatusLabel->setAlignment(Qt::AlignCenter);
    m_StatusLabel->setStyleSheet("color: #a0a0a0; font-size: 13px; padding-bottom: 15px;");
    m_StatusLabel->setMinimumHeight(40);
    container_layout->addWidget(m_StatusLabel);
    m_ProgressBar = new QProgressBar();
    m_ProgressBar->setRange(0, 100);
    m_ProgressBar->setValue(0);
    m_ProgressBar->setTextVisible(false);
    m_ProgressBar->setFixedHeight(4);
    m_ProgressBar->setStyleSheet("QProgressBar {"
                                 "   border: none;"
                                 "   background-color: #1a1a1a;"
                                 "   border-radius: 0px;"
                                 "   margin: 0px;"
                                 "}"
                                 "QProgressBar::chunk {"
                                 "   background-color: #00aaff;"
                                 "   border-radius: 0px;"
                                 "}");
    container_layout->addWidget(m_ProgressBar);
    main_layout->addWidget(container);
}

void SplashScreen::set_status(const QString& message) {
    m_StatusLabel->setText(message);
    QApplication::processEvents();
}

void SplashScreen::set_progress(int value) {
    m_ProgressBar->setValue(value);
    QApplication::processEvents();
}

void SplashScreen::finish() { close(); }

void SplashScreen::paintEvent(QPaintEvent* event) { QWidget::paintEvent(event); }
