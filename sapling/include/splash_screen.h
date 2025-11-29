#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QString>
#include <QPixmap>

class SplashScreen : public QWidget {
    Q_OBJECT

public:
    explicit SplashScreen(QWidget* parent = nullptr);
    ~SplashScreen() override = default;

    void set_status(const QString& message);
    void set_progress(int value);
    void finish();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setup_ui();

private:
    QLabel* m_LogoLabel{nullptr};
    QLabel* m_VersionLabel{nullptr};
    QLabel* m_StatusLabel{nullptr};
    QProgressBar* m_ProgressBar{nullptr};
    QPixmap m_LogoPixmap;
};
