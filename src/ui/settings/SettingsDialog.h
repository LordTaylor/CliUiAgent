#pragma once
#include <QDialog>

class QListWidget;
class QStackedWidget;

namespace CodeHex {

class AppConfig;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(AppConfig* config, QWidget* parent = nullptr);

private:
    void setupUi();
    QWidget* createGeneralPage();
    QWidget* createModelPage();
    QWidget* createUiPage();
    QWidget* createAdvancedPage();

    AppConfig* m_config;
    QListWidget* m_sidebar;
    QStackedWidget* m_pages;
};

} // namespace CodeHex
