#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

namespace CodeHex {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Application Settings");
    resize(480, 400);
    setupUi();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createGeneralTab(), "General");
    m_tabs->addTab(createModelTab(), "Model");
    m_tabs->addTab(createUiTab(), "Appearance");
    mainLayout->addWidget(m_tabs);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
}

QWidget* SettingsDialog::createGeneralTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    layout->addRow("User Name:", new QLineEdit("User", widget));
    layout->addRow("Work Folder Persistence:", new QCheckBox("Remember last folder", widget));
    layout->addRow("Auto-save Sessions:", new QCheckBox("", widget));

    return widget;
}

QWidget* SettingsDialog::createModelTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* modelCombo = new QComboBox(widget);
    modelCombo->addItems({"Qwen 2.5 Coder 14B", "Llama 3.1 8B", "GPT-4o (API)"});
    layout->addRow("Preferred Model:", modelCombo);

    layout->addRow("Temperature:", new QLineEdit("0.7", widget));
    layout->addRow("Max Tokens:", new QLineEdit("4096", widget));

    return widget;
}

QWidget* SettingsDialog::createUiTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* themeCombo = new QComboBox(widget);
    themeCombo->addItems({"Dark (Glass)", "Deep Dark", "Light"});
    layout->addRow("Theme:", themeCombo);

    layout->addRow("Font Size:", new QLineEdit("13", widget));
    layout->addRow("Show Mini-map:", new QCheckBox("", widget));

    return widget;
}

} // namespace CodeHex
