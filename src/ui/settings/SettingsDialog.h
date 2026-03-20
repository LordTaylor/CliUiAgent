#pragma once
#include <QDialog>

class QTabWidget;

namespace CodeHex {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private:
    void setupUi();
    QWidget* createGeneralTab();
    QWidget* createModelTab();
    QWidget* createUiTab();

    QTabWidget* m_tabs;
};

} // namespace CodeHex
