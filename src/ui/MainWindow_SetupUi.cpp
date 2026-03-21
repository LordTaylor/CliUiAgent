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
    m_sidebarSplitter->setMinimumWidth(220);
    m_sidebarSplitter->setMaximumWidth(320);

    auto* sidebarWidget = new QWidget(m_sidebarSplitter);
    sidebarWidget->setObjectName("sidebarWidget");
    auto* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    // Header: single "New Chat" button
    auto* sideHeader = new QWidget(sidebarWidget);
    auto* sideHeaderLayout = new QHBoxLayout(sideHeader);
    sideHeaderLayout->setContentsMargins(8, 8, 8, 6);
    sideHeaderLayout->setSpacing(6);

    auto* newChatBtn = new QPushButton("+ New Chat", sideHeader);
    newChatBtn->setObjectName("newSessionBtn");
    newChatBtn->setCursor(Qt::PointingHandCursor);
    sideHeaderLayout->addWidget(newChatBtn, 1);
    connect(newChatBtn, &QPushButton::clicked, this, &MainWindow::onNewSessionRequested);

    sidebarLayout->addWidget(sideHeader);

    // Sessions
    m_sessionPanel = new SessionPanel(m_sessions, sidebarWidget);
    m_sessionPanel->setMinimumHeight(120);
    sidebarLayout->addWidget(m_sessionPanel, 1);

    // Work folder
    m_workFolderPanel = new WorkFolderPanel(sidebarWidget);
    m_workFolderPanel->setFolder(m_config->workingFolder());
    connect(m_workFolderPanel, &WorkFolderPanel::folderChanged,
            m_config, &AppConfig::setWorkingFolder);
    connect(m_workFolderPanel, &WorkFolderPanel::contextFilesChanged,
            m_controller->agent(), &AgentEngine::setForcedContextFiles);
    sidebarLayout->addWidget(m_workFolderPanel, 2);

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
    tbLayout->setContentsMargins(8, 4, 8, 4);
    tbLayout->setSpacing(6);

    // ── Sidebar toggle (leftmost) ────────────────────────────────
    m_sidebarToggleBtn = new QPushButton("◀", toolbar);
    m_sidebarToggleBtn->setFixedSize(28, 28);
    m_sidebarToggleBtn->setCursor(Qt::PointingHandCursor);
    m_sidebarToggleBtn->setToolTip("Toggle Sidebar (Ctrl+B)");
    m_sidebarToggleBtn->setObjectName("sidebarToggleBtn");
    tbLayout->addWidget(m_sidebarToggleBtn);

    auto* sep0 = new QFrame(toolbar);
    sep0->setFrameShape(QFrame::VLine);
    sep0->setFixedWidth(1);
    sep0->setStyleSheet("background: #2D241C; margin: 6px 2px;");
    tbLayout->addWidget(sep0);

    // ── Left group: action icons ─────────────────────────────────
    auto* genSettingsBtn = new QPushButton(QIcon(":/resources/icons/settings.svg"), "", toolbar);
    genSettingsBtn->setToolTip("Settings");
    auto* skillsBtn      = new QPushButton(QIcon(":/resources/icons/skills.svg"),   "", toolbar);
    skillsBtn->setToolTip("Skills");
    auto* pluginsBtn     = new QPushButton(QIcon(":/resources/icons/plugins.svg"),  "", toolbar);
    pluginsBtn->setToolTip("Plugins");
    for (auto* b : {genSettingsBtn, skillsBtn, pluginsBtn}) {
        b->setFixedSize(30, 30);
        b->setIconSize(QSize(18, 18));
        b->setCursor(Qt::PointingHandCursor);
        tbLayout->addWidget(b);
    }

    // Thin separator
    auto* sep1 = new QFrame(toolbar);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFixedWidth(1);
    sep1->setStyleSheet("background: #2D241C; margin: 6px 4px;");
    tbLayout->addWidget(sep1);

    // Search box
    auto* searchEdit = new QLineEdit(toolbar);
    searchEdit->setPlaceholderText("Search in chat...");
    searchEdit->setFixedWidth(180);
    searchEdit->setClearButtonEnabled(true);
    tbLayout->addWidget(searchEdit);

    tbLayout->addStretch();  // single central stretch

    // ── Right group: provider ────────────────────────────────────
    auto* manageProvidersBtn = new QPushButton(QIcon(":/resources/icons/power.svg"), "", toolbar);
    manageProvidersBtn->setFixedSize(30, 30);
    manageProvidersBtn->setIconSize(QSize(18, 18));
    manageProvidersBtn->setToolTip("Manage LLM Providers");
    manageProvidersBtn->setCursor(Qt::PointingHandCursor);
    tbLayout->addWidget(manageProvidersBtn);

    m_providerCombo = new QComboBox(toolbar);
    m_providerCombo->setObjectName("providerCombo");
    m_providerCombo->setMinimumWidth(140);
    tbLayout->addWidget(m_providerCombo);

    // Thin separator
    auto* sep2 = new QFrame(toolbar);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFixedWidth(1);
    sep2->setStyleSheet("background: #2D241C; margin: 6px 4px;");
    tbLayout->addWidget(sep2);

    // ── Role combo ───────────────────────────────────────────────
    QLabel* roleLbl = new QLabel("Role:", toolbar);
    roleLbl->setStyleSheet("color: #6B7280; font-size: 12px;");
    m_roleCombo = new QComboBox(toolbar);
    m_roleCombo->addItem("Base",                  (int)AgentRole::Base);
    m_roleCombo->addItem("Explorer",              (int)AgentRole::Explorer);
    m_roleCombo->addItem("Executor",              (int)AgentRole::Executor);
    m_roleCombo->addItem("Reviewer",              (int)AgentRole::Reviewer);
    m_roleCombo->addItem("Local RAG",             (int)AgentRole::RAG);
    m_roleCombo->addItem("Refactoring Assistant", (int)AgentRole::Refactor);
    m_roleCombo->setMinimumWidth(90);
    tbLayout->addWidget(roleLbl);
    tbLayout->addWidget(m_roleCombo);

    // Thin separator
    auto* sep3 = new QFrame(toolbar);
    sep3->setFrameShape(QFrame::VLine);
    sep3->setFixedWidth(1);
    sep3->setStyleSheet("background: #2D241C; margin: 6px 4px;");
    tbLayout->addWidget(sep3);

    // ── Theme + Debug buttons ────────────────────────────────────
    m_themeBtn = new QPushButton("🌓", toolbar);
    m_themeBtn->setFixedSize(30, 30);
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setToolTip("Toggle Dark/Light Theme");
    m_debugBtn = new QPushButton(toolbar);
    m_debugBtn->setIcon(QIcon(":/resources/icons/bug.svg"));
    m_debugBtn->setFixedSize(30, 30);
    m_debugBtn->setCursor(Qt::PointingHandCursor);
    m_debugBtn->setToolTip("Save Debug Logs (Raw LLM I/O)");
    m_debugBtn->setObjectName("debugBtn");
    tbLayout->addWidget(m_themeBtn);
    tbLayout->addWidget(m_debugBtn);

    connect(genSettingsBtn, &QPushButton::clicked, this, [this]() { SettingsDialog dlg(m_config, this); dlg.exec(); });
    connect(skillsBtn,      &QPushButton::clicked, this, [this]() { SkillsDialog dlg(this); dlg.exec(); });
    connect(pluginsBtn,     &QPushButton::clicked, this, [this]() {
        PluginsDialog dlg(this);
        dlg.setScratchpadPath(m_config->workingFolder() + "/.agent/scratchpad");
        dlg.exec();
    });

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
    connect(searchEdit, &QLineEdit::textChanged, m_chatView, &ChatView::setSearchTerm);
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
 
    // Loop Warning Banner (Phase 40)
    m_loopWarningBanner = new QWidget(chatContainer);
    m_loopWarningBanner->setObjectName("loopWarningBanner");
    m_loopWarningBanner->setVisible(false);
    auto* loopLayout = new QHBoxLayout(m_loopWarningBanner);
    loopLayout->setContentsMargins(15, 8, 15, 8);
    
    QLabel* loopText = new QLabel("⚠️ Potential logic loop detected. The agent is repeating actions.", m_loopWarningBanner);
    loopText->setStyleSheet("color: #FCA5A5; font-weight: bold;");
    
    QPushButton* stopLoopBtn = new QPushButton("Stop Agent", m_loopWarningBanner);
    stopLoopBtn->setFixedSize(100, 24);
    stopLoopBtn->setStyleSheet("background: #991B1B; color: white; border-radius: 4px; font-size: 11px;");
    connect(stopLoopBtn, &QPushButton::clicked, this, &MainWindow::onStopRequested);

    loopLayout->addWidget(loopText, 1);
    loopLayout->addWidget(stopLoopBtn);
    
    m_loopWarningBanner->setStyleSheet("background: rgba(153, 27, 27, 0.9); border: 1px solid #EF4444; border-radius: 8px;");
    chatGrid->addWidget(m_loopWarningBanner, 0, 0, 3, 3, Qt::AlignTop | Qt::AlignHCenter);
    m_loopWarningBanner->raise();

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

    // Sidebar collapse/expand
    connect(m_sidebarToggleBtn, &QPushButton::clicked, this, [this]() {
        if (m_sidebarSplitter->isVisible()) {
            m_sidebarSizes = m_splitter->sizes();
            m_sidebarSplitter->setVisible(false);
            m_sidebarToggleBtn->setText("▶");
            m_sidebarToggleBtn->setToolTip("Show Sidebar (Ctrl+B)");
        } else {
            m_sidebarSplitter->setVisible(true);
            if (!m_sidebarSizes.isEmpty()) m_splitter->setSizes(m_sidebarSizes);
            m_sidebarToggleBtn->setText("◀");
            m_sidebarToggleBtn->setToolTip("Hide Sidebar (Ctrl+B)");
        }
    });
}
