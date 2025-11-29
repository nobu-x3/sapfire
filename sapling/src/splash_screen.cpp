#include "splash_screen.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QFile>

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
    setup_ui();
}

void SplashScreen::setup_ui() {
    setFixedSize(800, 450);
    setAttribute(Qt::WA_TranslucentBackground);
    // Center on screen
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    // Main container with black background
    QWidget* container = new QWidget();
    container->setStyleSheet("QWidget { background-color: #000000; }");
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    // Top spacer to push logo toward center
    containerLayout->addStretch(2);
    // Logo
    m_LogoLabel = new QLabel();
    m_LogoLabel->setAlignment(Qt::AlignCenter);
    QString logoPath = QApplication::applicationDirPath() + "/editor_assets/logo.png";
    if (QFile::exists(logoPath)) {
        m_LogoPixmap.load(logoPath);
        if (!m_LogoPixmap.isNull()) {
            // Scale logo to be large and prominent (Unreal Engine style)
            QPixmap scaledLogo = m_LogoPixmap.scaled(400, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_LogoLabel->setPixmap(scaledLogo);
        } else {
            m_LogoLabel->setText("SAPFIRE");
            m_LogoLabel->setStyleSheet("color: #ffffff; font-size: 72px; font-weight: bold; letter-spacing: 4px;");
        }
    } else {
        // Fallback text logo - large and bold like Unreal Engine
        m_LogoLabel->setText("SAPFIRE");
        m_LogoLabel->setStyleSheet("color: #ffffff; font-size: 72px; font-weight: bold; letter-spacing: 4px;");
    }
    containerLayout->addWidget(m_LogoLabel);
    containerLayout->addSpacing(15);
    // Version label (small, subtle)
    m_VersionLabel = new QLabel("Version 0.1.0");
    m_VersionLabel->setAlignment(Qt::AlignCenter);
    m_VersionLabel->setStyleSheet("color: #606060; font-size: 11px;");
    containerLayout->addWidget(m_VersionLabel);
    // Spacer before status
    containerLayout->addStretch(3);
    // Status label at bottom
    m_StatusLabel = new QLabel("Initializing...");
    m_StatusLabel->setAlignment(Qt::AlignCenter);
    m_StatusLabel->setStyleSheet("color: #a0a0a0; font-size: 13px; padding-bottom: 15px;");
    m_StatusLabel->setMinimumHeight(40);
    containerLayout->addWidget(m_StatusLabel);
    // Progress bar at very bottom edge (Unreal Engine style - thin line at bottom)
    m_ProgressBar = new QProgressBar();
    m_ProgressBar->setRange(0, 100);
    m_ProgressBar->setValue(0);
    m_ProgressBar->setTextVisible(false);
    m_ProgressBar->setFixedHeight(4);
    m_ProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: none;"
        "   background-color: #1a1a1a;"
        "   border-radius: 0px;"
        "   margin: 0px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #00aaff;"
        "   border-radius: 0px;"
        "}"
    );
    containerLayout->addWidget(m_ProgressBar);
    mainLayout->addWidget(container);
}

void SplashScreen::set_status(const QString& message) {
    m_StatusLabel->setText(message);
    QApplication::processEvents();
}

void SplashScreen::set_progress(int value) {
    m_ProgressBar->setValue(value);
    QApplication::processEvents();
}

void SplashScreen::finish() {
    close();
}

void SplashScreen::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    // Custom painting handled by stylesheets
    QWidget::paintEvent(event);
}
