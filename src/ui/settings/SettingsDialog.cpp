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

    auto* nameEdit = new QLineEdit("User", widget);
    nameEdit->setToolTip("The name the agent will refer to you as.");
    layout->addRow("User Name:", nameEdit);

    auto* rememberCheck = new QCheckBox("Remember last folder", widget);
    rememberCheck->setToolTip("Automatically open the last used working directory on startup.");
    layout->addRow("Work Folder Persistence:", rememberCheck);

    auto* autoSaveCheck = new QCheckBox("", widget);
    autoSaveCheck->setToolTip("Automatically save chat sessions to disk to prevent data loss.");
    layout->addRow("Auto-save Sessions:", autoSaveCheck);

    return widget;
}

QWidget* SettingsDialog::createModelTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* modelCombo = new QComboBox(widget);
    modelCombo->addItems({"Qwen 2.5 Coder 14B", "Llama 3.1 8B", "GPT-4o (API)"});
    modelCombo->setToolTip("Select the default language model to use for new sessions.");
    layout->addRow("Preferred Model:", modelCombo);

    auto* tempEdit = new QLineEdit("0.7", widget);
    tempEdit->setToolTip("Controls randomness: Lower values are more deterministic, higher are more creative (0.0 to 1.0).");
    layout->addRow("Temperature:", tempEdit);

    auto* maxTokensEdit = new QLineEdit("4096", widget);
    maxTokensEdit->setToolTip("The maximum number of tokens to generate in a single response.");
    layout->addRow("Max Tokens:", maxTokensEdit);

    return widget;
}

QWidget* SettingsDialog::createUiTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* themeCombo = new QComboBox(widget);
    themeCombo->addItems({"Dark (Glass)", "Deep Dark", "Light"});
    themeCombo->setToolTip("Choose the visual theme for CodeHex.");
    layout->addRow("Theme:", themeCombo);

    auto* fontEdit = new QLineEdit("13", widget);
    fontEdit->setToolTip("Base font size for the editor and chat interface.");
    layout->addRow("Font Size:", fontEdit);

    auto* minimapCheck = new QCheckBox("", widget);
    minimapCheck->setToolTip("Show a code mini-map in the file viewer for easier navigation.");
    layout->addRow("Show Mini-map:", minimapCheck);

    return widget;
}

} // namespace CodeHex
