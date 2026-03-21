#pragma once
#include <QDialog>

class QListWidget;
class QStackedWidget;

namespace CodeHex {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private:
    void setupUi();
    QWidget* createGeneralPage();
    QWidget* createModelPage();
    QWidget* createUiPage();
    QWidget* createAdvancedPage();

    QListWidget* m_sidebar;
    QStackedWidget* m_pages;
};

} // namespace CodeHex
