#include "SettingsDialog.h"
#include "../../core/AppConfig.h"
#include "../../core/CrashHandler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QIcon>
#include <QListWidgetItem>
#include <QFrame>

namespace CodeHex {

SettingsDialog::SettingsDialog(AppConfig* config, QWidget* parent) 
    : QDialog(parent), m_config(config) {
    setWindowTitle("Application Settings");
    resize(540, 420);
    setupUi();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(0);
    mainLayout->addLayout(contentLayout);

    // Sidebar
    m_sidebar = new QListWidget(this);
    m_sidebar->setObjectName("settingsSidebar");
    m_sidebar->setFixedWidth(160);
    m_sidebar->setIconSize(QSize(20, 20));
    m_sidebar->setFrameShape(QFrame::NoFrame);
    
    m_sidebar->addItem(new QListWidgetItem(QIcon(":/resources/icons/settings.svg"), "General"));
    m_sidebar->addItem(new QListWidgetItem(QIcon(":/resources/icons/model.svg"), "Model"));
    m_sidebar->addItem(new QListWidgetItem(QIcon(":/resources/icons/appearance.svg"), "Appearance"));
    m_sidebar->addItem(new QListWidgetItem(QIcon(":/resources/icons/advanced.svg"), "Advanced"));
    
    contentLayout->addWidget(m_sidebar);

    // Separator line
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet("color: #374151;");
    contentLayout->addWidget(line);

    // Pages
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(createGeneralPage());
    m_pages->addWidget(createModelPage());
    m_pages->addWidget(createUiPage());
    m_pages->addWidget(createAdvancedPage());
    
    contentLayout->addWidget(m_pages);

    // Connections
    connect(m_sidebar, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    m_sidebar->setCurrentRow(0);

    // Bottom Buttons
    auto* btmBar = new QWidget(this);
    btmBar->setStyleSheet("background-color: #1F2937; border-top: 1px solid #374151;");
    auto* btnLayout = new QHBoxLayout(btmBar);
    btnLayout->setContentsMargins(12, 8, 12, 8);
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setFixedWidth(80);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addWidget(btmBar);
}

QWidget* SettingsDialog::createGeneralPage() {
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

QWidget* SettingsDialog::createModelPage() {
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

QWidget* SettingsDialog::createUiPage() {
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

QWidget* SettingsDialog::createAdvancedPage() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* systemPromptEdit = new QLineEdit(widget);
    systemPromptEdit->setText(m_config->systemPrompt());
    systemPromptEdit->setPlaceholderText("Enter custom system prompt...");
    systemPromptEdit->setToolTip("Override the default system prompt for all sessions.");
    layout->addRow("Global System Prompt:", systemPromptEdit);

    connect(systemPromptEdit, &QLineEdit::textEdited, m_config, &AppConfig::setSystemPrompt);
    connect(m_config, &AppConfig::systemPromptChanged, systemPromptEdit, [systemPromptEdit](const QString& p){
        if (systemPromptEdit->text() != p) systemPromptEdit->setText(p);
    });

    auto* rollbackBtn = new QPushButton("Rollback Prompt", widget);
    rollbackBtn->setToolTip("Restore the previous version of the system prompt (Item 45).");
    layout->addRow("Prompt Versioning:", rollbackBtn);

    connect(rollbackBtn, &QPushButton::clicked, m_config, &AppConfig::rollbackPrompt);

    auto* crashTestBtn = new QPushButton("Simulate Crash", widget);
    crashTestBtn->setToolTip("Triggers a crash to test the Crash Reporter (Item 51).");
    layout->addRow("Diagnostics:", crashTestBtn);

    return widget;
}

} // namespace CodeHex
