/**
 * @file MainWindow_SetupUi.cpp
 * @brief Widget tree construction — sidebar, toolbar, chat container, input panel.
 *        Called once from the constructor via setupUi().
 */
#include "MainWindow.h"
#include "settings/ProviderSettingsDialog.h"
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include "../core/AgentEngine.h"
#include "../core/AgentRole.h"
#include "../core/AppConfig.h"
#include "../core/ChatController.h"
#include "../core/SessionManager.h"
#include "chat/ChatControlBanner.h"
#include "chat/ChatView.h"
#include "chat/MessageModel.h"
#include "input/InputPanel.h"
#include "plugins/PluginsDialog.h"
#include "session/SessionPanel.h"
#include "settings/SettingsDialog.h"
#include "skills/SkillsDialog.h"
#include "workfolder/WorkFolderPanel.h"

using namespace CodeHex;

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal, central);
    mainLayout->addWidget(m_splitter);

    // ── Left sidebar ─────────────────────────────────────────────
    m_sidebarSplitter = new QSplitter(Qt::Vertical, m_splitter);
    m_sidebarSplitter->setObjectName("sidebarSplitter");
    m_sidebarSplitter->setMinimumWidth(240);
    m_sidebarSplitter->setMaximumWidth(320);

    auto* sidebarWidget = new QWidget(m_sidebarSplitter);
    sidebarWidget->setObjectName("sidebarWidget");
    auto* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto* sideHeader = new QWidget(sidebarWidget);
    auto* sideHeaderLayout = new QVBoxLayout(sideHeader);
    sideHeaderLayout->setContentsMargins(12, 12, 12, 8);
    sideHeaderLayout->setSpacing(4);

    auto* newTaskBtn  = new QPushButton("New Task", sideHeader);
    newTaskBtn->setObjectName("newTaskBtn");
    auto* newChatBtn1 = new QPushButton("New Chat", sideHeader);
    newChatBtn1->setObjectName("newSessionBtn");
    auto* newChatBtn2 = new QPushButton("New Chat", sideHeader);
    newChatBtn2->setObjectName("newSessionBtn");
    sideHeaderLayout->addWidget(newTaskBtn);
    sideHeaderLayout->addWidget(newChatBtn1);
    sideHeaderLayout->addWidget(newChatBtn2);

    auto* dirLabel = new QLabel("<I current directory contents to unde", sideHeader);
    dirLabel->setStyleSheet("color: #6B7280; font-size: 11px; margin-top: 8px;");
    sideHeaderLayout->addWidget(dirLabel);

    auto* thoughtLabel = new QLabel("<thought>", sideHeader);
    thoughtLabel->setStyleSheet("color: #6B7280; font-size: 11px; font-style: italic;");
    sideHeaderLayout->addWidget(thoughtLabel);

    sidebarLayout->addWidget(sideHeader);

    m_sessionPanel = new SessionPanel(m_sessions, sidebarWidget);
    m_sessionPanel->setMinimumHeight(150);
    sidebarLayout->addWidget(m_sessionPanel);

    m_workFolderPanel = new WorkFolderPanel(sidebarWidget);
    m_workFolderPanel->setFolder(m_config->workingFolder());
    connect(m_workFolderPanel, &WorkFolderPanel::folderChanged,
            m_config, &AppConfig::setWorkingFolder);
    connect(m_workFolderPanel, &WorkFolderPanel::contextFilesChanged,
            m_controller->agent(), &AgentEngine::setForcedContextFiles);
    sidebarLayout->addWidget(m_workFolderPanel);
    sidebarLayout->addStretch();

    m_sidebarSplitter->addWidget(sidebarWidget);
    m_splitter->addWidget(m_sidebarSplitter);

    // ── Right panel ──────────────────────────────────────────────
    auto* rightWidget = new QWidget(m_splitter);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Toolbar
    auto* toolbar = new QWidget(rightWidget);
    toolbar->setObjectName("toolbar");
    rightLayout->addWidget(toolbar);

    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(12, 6, 12, 6);
    tbLayout->setSpacing(12);

    auto* settingsBar    = new QWidget(toolbar);
    auto* settingsLayout = new QHBoxLayout(settingsBar);
    settingsLayout->setContentsMargins(0, 0, 0, 0);
    settingsLayout->setSpacing(4);

    auto* genSettingsBtn = new QPushButton(QIcon(":/resources/icons/settings.svg"), "", settingsBar);
    auto* skillsBtn      = new QPushButton(QIcon(":/resources/icons/skills.svg"),   "", settingsBar);
    auto* pluginsBtn     = new QPushButton(QIcon(":/resources/icons/plugins.svg"),  "", settingsBar);
    for (auto* b : {genSettingsBtn, skillsBtn, pluginsBtn}) {
        b->setFixedSize(32, 32);
        b->setIconSize(QSize(20, 20));
        b->setCursor(Qt::PointingHandCursor);
        settingsLayout->addWidget(b);
    }
    tbLayout->addWidget(settingsBar);

    auto* searchEdit = new QLineEdit(toolbar);
    searchEdit->setPlaceholderText("Search in chat...");
    searchEdit->setFixedWidth(200);
    searchEdit->setClearButtonEnabled(true);
    tbLayout->addStretch();
    tbLayout->addWidget(searchEdit);
    connect(searchEdit, &QLineEdit::textChanged, m_chatView, &ChatView::setSearchTerm);

    connect(genSettingsBtn, &QPushButton::clicked, this, [this]() { SettingsDialog dlg(m_config, this); dlg.exec(); });
    connect(skillsBtn,      &QPushButton::clicked, this, [this]() { SkillsDialog dlg(this); dlg.exec(); });
    connect(pluginsBtn,     &QPushButton::clicked, this, [this]() {
        PluginsDialog dlg(this);
        dlg.setScratchpadPath(m_config->workingFolder() + "/.agent/scratchpad");
        dlg.exec();
    });
    tbLayout->addStretch();

    auto* llmBar    = new QWidget(toolbar);
    auto* llmLayout = new QHBoxLayout(llmBar);
    llmLayout->setContentsMargins(0, 0, 0, 0);
    llmLayout->setSpacing(4);

    auto* manageProvidersBtn = new QPushButton(QIcon(":/resources/icons/power.svg"), "", llmBar);
    manageProvidersBtn->setFixedSize(32, 32);
    manageProvidersBtn->setIconSize(QSize(20, 20));
    manageProvidersBtn->setToolTip("Manage LLM Providers");
    manageProvidersBtn->setCursor(Qt::PointingHandCursor);

    m_providerCombo = new QComboBox(llmBar);
    m_providerCombo->setObjectName("providerCombo");
    m_providerCombo->setMinimumWidth(150);
    llmLayout->addWidget(manageProvidersBtn);
    llmLayout->addWidget(m_providerCombo);
    tbLayout->addWidget(llmBar);
    tbLayout->addStretch();

    auto* roleGroup  = new QWidget(toolbar);
    auto* roleLayout = new QHBoxLayout(roleGroup);
    roleLayout->setContentsMargins(0, 0, 0, 0);
    roleLayout->setSpacing(8);
    QLabel* roleLbl = new QLabel("Role:", roleGroup);
    m_roleCombo = new QComboBox(roleGroup);
    m_roleCombo->addItem("Base",                  (int)AgentRole::Base);
    m_roleCombo->addItem("Explorer",              (int)AgentRole::Explorer);
    m_roleCombo->addItem("Executor",              (int)AgentRole::Executor);
    m_roleCombo->addItem("Reviewer",              (int)AgentRole::Reviewer);
    m_roleCombo->addItem("Local RAG",             (int)AgentRole::RAG);
    m_roleCombo->addItem("Refactoring Assistant", (int)AgentRole::Refactor);
    m_roleCombo->setMinimumWidth(100);
    roleLayout->addWidget(roleLbl);
    roleLayout->addWidget(m_roleCombo);
    tbLayout->addWidget(roleGroup);
    tbLayout->addSpacing(12);

    auto* actionGroup  = new QWidget(toolbar);
    auto* actionLayout = new QHBoxLayout(actionGroup);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    m_themeBtn = new QPushButton("🌓", actionGroup);
    m_themeBtn->setFixedSize(32, 32);
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setToolTip("Toggle Dark/Light Theme");
    m_debugBtn = new QPushButton(actionGroup);
    m_debugBtn->setIcon(QIcon(":/resources/icons/bug.svg"));
    m_debugBtn->setFixedSize(32, 32);
    m_debugBtn->setCursor(Qt::PointingHandCursor);
    m_debugBtn->setToolTip("Save Debug Logs (Raw LLM I/O)");
    m_debugBtn->setObjectName("debugBtn");
    actionLayout->addWidget(m_themeBtn);
    actionLayout->addWidget(m_debugBtn);
    tbLayout->addWidget(actionGroup);

    // Chat control banner
    m_chatBanner = new ChatControlBanner(rightWidget);
    m_chatBanner->setAutoApprove(!m_config->manualApproval());
    rightLayout->addWidget(m_chatBanner);
    connect(m_chatBanner, &ChatControlBanner::autoApproveChanged, this, [this](bool checked) {
        m_config->setManualApproval(!checked);
        m_controller->setManualApproval(!checked);
    });
    connect(m_chatBanner, &ChatControlBanner::clearChatRequested, this, &MainWindow::onClearChatRequested);
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggleRequested);
    connect(m_debugBtn, &QPushButton::clicked, this, &MainWindow::onDebugLogRequested);

    // Chat view + floating overlays
    auto* chatContainer = new QWidget(rightWidget);
    auto* chatGrid      = new QGridLayout(chatContainer);
    chatGrid->setContentsMargins(0, 0, 0, 0);

    m_messageModel = new MessageModel(this);
    m_chatView     = new ChatView(chatContainer);
    m_messageModel->setViewWidth(m_chatView->width());
    m_chatView->setMessageModel(m_messageModel);
    connect(m_chatView, &ChatView::loadMoreRequested, m_messageModel, &MessageModel::loadMoreMessages);
    connect(m_splitter, &QSplitter::splitterMoved, this, [this]() {
        if (m_messageModel && m_chatView)
            m_messageModel->setViewWidth(m_chatView->viewport()->width());
    });
    chatGrid->addWidget(m_chatView, 0, 0, 3, 3);

    auto* btnOverlay    = new QWidget(chatContainer);
    auto* overlayLayout = new QVBoxLayout(btnOverlay);
    overlayLayout->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    overlayLayout->setSpacing(10);
    overlayLayout->setContentsMargins(0, 0, 20, 20);

    m_autoScrollBtn = new QPushButton("🧲", btnOverlay);
    m_autoScrollBtn->setCheckable(true);
    m_autoScrollBtn->setChecked(true);
    m_autoScrollBtn->setFixedSize(40, 40);
    m_autoScrollBtn->setToolTip("Keep auto-scroll (Magnet)");
    m_autoScrollBtn->setObjectName("magnetBtn");
    overlayLayout->addWidget(m_autoScrollBtn);

    m_scrollToBottomBtn = new QPushButton("↓", btnOverlay);
    m_scrollToBottomBtn->setFixedSize(40, 40);
    m_scrollToBottomBtn->setToolTip("Scroll to bottom");
    m_scrollToBottomBtn->setObjectName("scrollDownBtn");
    overlayLayout->addWidget(m_scrollToBottomBtn);

    m_stopBtn = new QPushButton("⏹", btnOverlay);
    m_stopBtn->setFixedSize(40, 40);
    m_stopBtn->setToolTip("Stop Agent");
    m_stopBtn->setObjectName("stopFloatingBtn");
    m_stopBtn->setVisible(false);
    overlayLayout->addWidget(m_stopBtn);

    btnOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    chatGrid->addWidget(btnOverlay, 0, 0, 3, 3, Qt::AlignBottom | Qt::AlignRight);
    btnOverlay->raise();

    m_statusLabel = new QLabel(chatContainer);
    m_statusLabel->setObjectName("agentStatusLabel");
    m_statusLabel->setVisible(true);
    m_statusLabel->setText("Ready");
    m_statusLabel->setStyleSheet(
        "background: rgba(31, 41, 55, 0.95); color: #9CA3AF; padding: 8px 20px; "
        "border-radius: 14px; border: 1px solid #4B5563; font-weight: bold; font-size: 13px;");
    chatGrid->addWidget(m_statusLabel, 0, 0, 3, 3, Qt::AlignHCenter | Qt::AlignBottom);
    m_statusLabel->raise();

    rightLayout->addWidget(chatContainer, 1);

    connect(m_scrollToBottomBtn, &QPushButton::clicked, m_chatView, &ChatView::scrollToBottom);
    connect(m_autoScrollBtn,     &QPushButton::toggled, m_chatView, &ChatView::setAutoScrollEnabled);
    connect(m_stopBtn,           &QPushButton::clicked, this,       &MainWindow::onStopRequested);
    connect(m_chatView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        if (value < m_chatView->verticalScrollBar()->maximum() - 20)
            if (m_autoScrollBtn->isChecked()) m_autoScrollBtn->setChecked(false);
    });

    m_inputPanel = new InputPanel(m_recorder, rightWidget);
    rightLayout->addWidget(m_inputPanel);
    connect(m_inputPanel, &InputPanel::sendRequested,    this, &MainWindow::onSendRequested);
    connect(m_inputPanel, &InputPanel::commandRequested, this, &MainWindow::onCommandRequested);
    connect(m_inputPanel, &InputPanel::stopRequested,    this, &MainWindow::onStopRequested);

    m_terminalPanel = new TerminalPanel(rightWidget);
    rightLayout->addWidget(m_terminalPanel);

    m_splitter->addWidget(rightWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    connect(manageProvidersBtn, &QPushButton::clicked, this, [this]() {
        ProviderSettingsDialog dlg(m_config, this);
        if (dlg.exec() == QDialog::Accepted) updateProviderList();
    });
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProviderChanged);
    connect(m_roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_controller->agent()->setRole((AgentRole)m_roleCombo->itemData(index).toInt());
    });

    m_tokenLabel = new QLabel(this);
    m_tokenLabel->setObjectName("tokenLabel");
    statusBar()->addPermanentWidget(m_tokenLabel);
}
