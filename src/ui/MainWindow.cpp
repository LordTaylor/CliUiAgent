#include "settings/ProviderSettingsDialog.h"
#include "MainWindow.h"
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QSlider>
#include <QPainter>
#include <QSvgRenderer>
#include <optional>
#include "../core/LlmDiscoveryService.h"
#include "../core/AppConfig.h"
#include "help/HelpDialog.h"
#include "../audio/AudioPlayer.h"
#include "../audio/AudioRecorder.h"
#include "../cli/CliRunner.h"
#include "../cli/OllamaProfile.h"
#include "../core/AppConfig.h"
#include "../core/ChatController.h"
#include "../core/SessionManager.h"
#include "../core/AgentEngine.h"
#include "../core/AgentRole.h" // Added
#include "../data/Message.h"
#include "chat/ChatView.h"
#include "chat/ChatControlBanner.h"
#include "chat/MessageModel.h"
// Removed legacy console include
#include "input/InputPanel.h"
#include "session/SessionPanel.h"
#include "workfolder/WorkFolderSelector.h"
#include "workfolder/WorkFolderPanel.h"
#include "settings/SettingsDialog.h"
#include "skills/SkillsDialog.h"
#include "plugins/PluginsDialog.h"

using namespace CodeHex;

MainWindow::MainWindow(AppConfig* config,
                       SessionManager* sessions,
                       ChatController* controller,
                       AudioRecorder* recorder,
                       AudioPlayer* player,
                       const ProfileList& extraProfiles,
                       QWidget* parent)
    : QMainWindow(parent),
      m_config(config),
      m_sessions(sessions),
      m_controller(controller),
      m_recorder(recorder),
      m_player(player),
      m_extraProfiles(extraProfiles) {
    setWindowTitle("CodeHex");
    setWindowIcon(QIcon(":/resources/icons/app.png"));
    setMinimumSize(900, 600);
    resize(1200, 800);

    setupUi();
    setupMenuBar();
    loadStyleSheet();
    updateProviderList();

    // Connect ChatController signals
    connect(m_controller, &ChatController::userMessageReady, this,
            [this](const Message& msg) { m_messageModel->appendMessage(msg); });
    connect(m_controller, &ChatController::tokenReceived, this, [this](const QString& chunk) {
        m_messageModel->appendToken(chunk);
        m_chatView->scrollToBottomSmooth();
    });
    connect(m_controller, &ChatController::tokenStatsUpdated, this, [this](int in, int out) {
        updateTokenLabel(in, out);
    });
    connect(m_controller, &ChatController::responseComplete,
            this, &MainWindow::onResponseComplete);
    connect(m_controller, &ChatController::cliOutputReceived,
            m_terminalPanel, &TerminalPanel::appendOutput);
    connect(m_controller, &ChatController::cliErrorReceived,
            m_terminalPanel, &TerminalPanel::appendError);
    connect(m_controller, &ChatController::generationStarted,
            this, &MainWindow::onGenerationStarted);
    connect(m_controller, &ChatController::generationStopped,
            this, &MainWindow::onGenerationStopped);
    connect(m_controller, &ChatController::generationStopped,
            this, [this]() { m_scrollToBottomBtn->setEnabled(true); });
    connect(m_controller, &ChatController::errorOccurred,
            m_terminalPanel, &TerminalPanel::appendError);
    connect(m_controller, &ChatController::statusChanged,
            this, [this](const QString& status) { 
                m_statusLabel->setText(status);
                m_chatBanner->setStatusText(status);
            });
    connect(m_controller, &ChatController::toolApprovalRequested,
            this, &MainWindow::onToolApprovalRequested);
    connect(m_controller, &ChatController::sessionRenamed,
            this, [this](const QString& /*id*/, const QString& title) {
                m_sessionPanel->refresh();
                setWindowTitle("CodeHex — " + title);
            });

    // Cursor blink timer (started/stopped around generation)

    // Cursor blink timer (started/stopped around generation)
    m_cursorTimer = new QTimer(this);
    m_cursorTimer->setInterval(500);
    connect(m_cursorTimer, &QTimer::timeout, this, &MainWindow::onCursorBlink);

    m_tokenTimer = new QTimer(this);
    m_tokenTimer->setInterval(50); // 50ms batch window
    m_tokenTimer->setSingleShot(true);
    connect(m_tokenTimer, &QTimer::timeout, this, &MainWindow::onTokenBufferTimeout);

    // Load sessions and open last
    m_sessions->loadAll();
    Session* last = m_sessions->allSessions().isEmpty()
        ? m_sessions->createSession(m_config->activeProviderId(), "default")
        : m_sessions->allSessions().first();
    m_sessions->setCurrentSession(last);
    switchSession(last);
    updateButtonIcons();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Main Horizontal Splitter ---
    m_splitter = new QSplitter(Qt::Horizontal, central);
    mainLayout->addWidget(m_splitter);

    // --- Left Sidebar: Vertical Splitter (Sessions | Files) ---
    m_sidebarSplitter = new QSplitter(Qt::Vertical, m_splitter);
    m_sidebarSplitter->setObjectName("sidebarSplitter");
    m_sidebarSplitter->setMinimumWidth(200);
    m_sidebarSplitter->setMaximumWidth(320);

    m_sessionPanel = new SessionPanel(m_sessions, m_sidebarSplitter);
    m_sessionPanel->setMinimumHeight(150);
    
    m_workFolderPanel = new WorkFolderPanel(m_sidebarSplitter);
    m_workFolderPanel->setFolder(m_config->workingFolder());
    connect(m_workFolderPanel, &WorkFolderPanel::folderChanged,
            m_config, &AppConfig::setWorkingFolder);
    
    m_sidebarSplitter->addWidget(m_sessionPanel);
    m_sidebarSplitter->addWidget(m_workFolderPanel);
    
    // Set 30/70 ratio
    m_sidebarSplitter->setStretchFactor(0, 3);
    m_sidebarSplitter->setStretchFactor(1, 7);
    
    m_splitter->addWidget(m_sidebarSplitter);

    // Right side container for Chat and Input
    auto* rightWidget = new QWidget(m_splitter);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Toolbar row
    auto* toolbar = new QWidget(rightWidget);
    toolbar->setObjectName("toolbar");
    rightLayout->addWidget(toolbar);

    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(12, 6, 12, 6);
    tbLayout->setSpacing(12);

    // Settings Icons Area
    auto* settingsBar = new QWidget(toolbar);
    auto* settingsLayout = new QHBoxLayout(settingsBar);
    settingsLayout->setContentsMargins(0, 0, 0, 0);
    settingsLayout->setSpacing(4);
    
    auto* genSettingsBtn = new QPushButton(QIcon(":/resources/icons/settings.svg"), "", settingsBar);
    auto* skillsBtn = new QPushButton(QIcon(":/resources/icons/skills.svg"), "", settingsBar);
    auto* pluginsBtn = new QPushButton(QIcon(":/resources/icons/plugins.svg"), "", settingsBar);
    
    for (auto* b : {genSettingsBtn, skillsBtn, pluginsBtn}) {
        b->setFixedSize(32, 32);
        b->setIconSize(QSize(20, 20));
        b->setCursor(Qt::PointingHandCursor);
        settingsLayout->addWidget(b);
    }
    tbLayout->addWidget(settingsBar);

    connect(genSettingsBtn, &QPushButton::clicked, this, [this]() {
        SettingsDialog dlg(this);
        dlg.exec();
    });
    connect(skillsBtn, &QPushButton::clicked, this, [this]() {
        SkillsDialog dlg(this);
        dlg.exec();
    });
    connect(pluginsBtn, &QPushButton::clicked, this, [this]() {
        PluginsDialog dlg(this);
        dlg.setScratchpadPath(m_config->workingFolder() + "/.agent/scratchpad");
        dlg.exec();
    });

    tbLayout->addStretch();

    // LLM Provider Selection
    auto* llmBar = new QWidget(toolbar);
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
    
    // Role Selection Group
    auto* roleGroup = new QWidget(toolbar);
    auto* roleLayout = new QHBoxLayout(roleGroup);
    roleLayout->setContentsMargins(0, 0, 0, 0);
    roleLayout->setSpacing(8);

    QLabel* roleLbl = new QLabel("Role:", roleGroup);
    m_roleCombo = new QComboBox(roleGroup);
    m_roleCombo->addItem("Base", (int)AgentRole::Base);
    m_roleCombo->addItem("Explorer", (int)AgentRole::Explorer);
    m_roleCombo->addItem("Executor", (int)AgentRole::Executor);
    m_roleCombo->addItem("Reviewer", (int)AgentRole::Reviewer);
    m_roleCombo->setMinimumWidth(100);
    
    roleLayout->addWidget(roleLbl);
    roleLayout->addWidget(m_roleCombo);
    tbLayout->addWidget(roleGroup);
    
    tbLayout->addSpacing(12);

    // Theme & Debug Buttons Group
    auto* actionGroup = new QWidget(toolbar);
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
    
    // Chat control banner (NEW)
    m_chatBanner = new ChatControlBanner(rightWidget);
    m_chatBanner->setAutoApprove(!m_config->manualApproval());
    rightLayout->addWidget(m_chatBanner);
    
    connect(m_chatBanner, &ChatControlBanner::autoApproveChanged, this, [this](bool checked) {
        m_config->setManualApproval(!checked);
        m_controller->setManualApproval(!checked);
    });
    
    connect(m_chatBanner, &ChatControlBanner::clearChatRequested,
            this, &MainWindow::onClearChatRequested);

    connect(m_themeBtn, &QPushButton::clicked,
            this, &MainWindow::onThemeToggleRequested);
    connect(m_debugBtn, &QPushButton::clicked, this, &MainWindow::onDebugLogRequested);

    // Input area
    // Chat view container for floating buttons
    auto* chatContainer = new QWidget(rightWidget);
    auto* chatGrid = new QGridLayout(chatContainer);
    chatGrid->setContentsMargins(0, 0, 0, 0);

    m_messageModel = new MessageModel(this);
    m_chatView = new ChatView(chatContainer);
    m_messageModel->setViewWidth(m_chatView->width());
    m_chatView->setMessageModel(m_messageModel);
    connect(m_chatView, &ChatView::loadMoreRequested,
            m_messageModel, &MessageModel::loadMoreMessages);

    // Update layout width when user resizes the chat area
    connect(m_splitter, &QSplitter::splitterMoved, this, [this]() {
        if (m_messageModel && m_chatView) {
            m_messageModel->setViewWidth(m_chatView->viewport()->width());
        }
    });

    chatGrid->addWidget(m_chatView, 0, 0, 3, 3);

    // Floating Buttons (Right-Bottom corner)
    auto* btnOverlay = new QWidget(chatContainer);
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

    // Status Label (Floating at bottom-left or center-bottom)
    m_statusLabel = new QLabel(chatContainer);
    m_statusLabel->setObjectName("agentStatusLabel");
    m_statusLabel->setVisible(true);
    m_statusLabel->setText("Ready");
    m_statusLabel->setStyleSheet(
        "background: rgba(31, 41, 55, 0.95); "
        "color: #9CA3AF; "
        "padding: 8px 20px; "
        "border-radius: 14px; "
        "border: 1px solid #4B5563; "
        "font-weight: bold; "
        "font-size: 13px;"
    );
    chatGrid->addWidget(m_statusLabel, 0, 0, 3, 3, Qt::AlignHCenter | Qt::AlignBottom);
    m_statusLabel->raise();

    rightLayout->addWidget(chatContainer, 1);

    // Wire navigation
    connect(m_scrollToBottomBtn, &QPushButton::clicked, m_chatView, &ChatView::scrollToBottom);
    connect(m_autoScrollBtn, &QPushButton::toggled, m_chatView, &ChatView::setAutoScrollEnabled);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopRequested);

    // Uncheck magnet if user scrolls up manually
    connect(m_chatView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        if (value < m_chatView->verticalScrollBar()->maximum() - 20) {
            if (m_autoScrollBtn->isChecked()) {
                m_autoScrollBtn->setChecked(false);
            }
        }
    });

    // Input panel
    m_inputPanel = new InputPanel(m_recorder, rightWidget);
    rightLayout->addWidget(m_inputPanel);
    
    // Connect input panel interactions to MainWindow slots
    connect(m_inputPanel, &InputPanel::sendRequested, this, &MainWindow::onSendRequested);
    connect(m_inputPanel, &InputPanel::commandRequested, this, &MainWindow::onCommandRequested);
    connect(m_inputPanel, &InputPanel::stopRequested, this, &MainWindow::onStopRequested);

    // Console
    m_terminalPanel = new TerminalPanel(rightWidget);
    rightLayout->addWidget(m_terminalPanel);

    m_splitter->addWidget(rightWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    // Wire toolbar actions
    connect(manageProvidersBtn, &QPushButton::clicked, this, [this]() {
        ProviderSettingsDialog dlg(m_config, this);
        if (dlg.exec() == QDialog::Accepted) {
            updateProviderList();
        }
    });

    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProviderChanged);

    connect(m_roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        AgentRole role = (AgentRole)m_roleCombo->itemData(index).toInt();
        m_controller->agent()->setRole(role);
    });

    connect(m_themeBtn, &QPushButton::clicked,
            this, &MainWindow::onThemeToggleRequested);
    connect(m_debugBtn, &QPushButton::clicked, this, &MainWindow::onDebugLogRequested);

    // Token stats label in status bar
    m_tokenLabel = new QLabel(this);
    m_tokenLabel->setObjectName("tokenLabel");
    statusBar()->addPermanentWidget(m_tokenLabel);
}

void MainWindow::setupMenuBar() {
    // ── File ──────────────────────────────────────────────────────
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* newSession = fileMenu->addAction("&New Session");
    newSession->setShortcut(QKeySequence("Ctrl+N"));
    connect(newSession, &QAction::triggered, this, &MainWindow::onNewSessionRequested);

    QAction* clearChat = fileMenu->addAction("&Clear Chat");
    clearChat->setShortcut(QKeySequence("Ctrl+Shift+L"));
    connect(clearChat, &QAction::triggered, this, &MainWindow::onClearChatRequested);

    QAction* stopGeneration = fileMenu->addAction("Stop &Generation");
    stopGeneration->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(stopGeneration, &QAction::triggered, this, &MainWindow::onStopRequested);

    fileMenu->addSeparator();

    QAction* quit = fileMenu->addAction("&Quit");
    quit->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    // ── View ──────────────────────────────────────────────────────
    QMenu* viewMenu = menuBar()->addMenu("&View");

    QAction* toggleSidebar = viewMenu->addAction("Toggle &Sidebar");
    toggleSidebar->setShortcut(QKeySequence("Ctrl+B"));
    connect(toggleSidebar, &QAction::triggered, this, [this]() {
        m_sessionPanel->setVisible(!m_sessionPanel->isVisible());
    });

    QAction* toggleConsole = viewMenu->addAction("Toggle &Console");
    toggleConsole->setShortcut(QKeySequence("Ctrl+`"));
    connect(toggleConsole, &QAction::triggered, m_terminalPanel, &TerminalPanel::toggleExpanded);

    viewMenu->addSeparator();

    QAction* fullscreen = viewMenu->addAction("&Fullscreen");
    fullscreen->setShortcut(QKeySequence("Ctrl+Shift+F"));
    fullscreen->setCheckable(true);
    connect(fullscreen, &QAction::triggered, this, [this](bool checked) {
        checked ? showFullScreen() : showNormal();
    });

    // ── Help ──────────────────────────────────────────────────────
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    struct HelpEntry { QString label; QString page; QString shortcut; };
    const QList<HelpEntry> helpPages = {
        {"&Getting Started",       "getting-started",      "F1"},
        {"&Interface Guide",       "ui-guide",             ""},
        {"&Sessions",              "sessions",             ""},
        {"&CLI Profiles & Models", "cli-profiles",         ""},
        {"Sc&ripting (Lua/Python)","scripting",            ""},
        {"&Voice && Attachments",  "voice-and-attachments",""},
        {"&Keyboard Shortcuts",    "keyboard-shortcuts",   ""},
    };

    for (const auto& entry : helpPages) {
        QAction* act = helpMenu->addAction(entry.label);
        if (!entry.shortcut.isEmpty())
            act->setShortcut(QKeySequence(entry.shortcut));
        const QString page = entry.page;
        connect(act, &QAction::triggered, this, [this, page]() {
            onHelpRequested(page);
        });
    }

    helpMenu->addSeparator();

    QAction* aboutAct = helpMenu->addAction("&About CodeHex");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::onHelpRequested(const QString& page) {
    if (!m_helpDialog) {
        m_helpDialog = new HelpDialog(page, this);
    } else {
        m_helpDialog->openPage(page);
    }
    m_helpDialog->show();
    m_helpDialog->raise();
    m_helpDialog->activateWindow();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About CodeHex",
        "CodeHex v" + qApp->applicationVersion() + "\n\n"
        "Advanced Agentic Coding Environment\n"
        "Built with Qt 6.7 and LLMs.");
}

void MainWindow::onDebugLogRequested() {
    QString logDir = QDir(m_config->workingFolder()).filePath("Debug_Logs");
    QDir dir(logDir);
    
    // 1. Clear existing files
    if (dir.exists()) {
        dir.removeRecursively();
    }
    dir.mkpath(".");

    // 2. Save logs via controller
    if (m_controller && m_controller->agent()) {
        m_controller->agent()->saveDebugLog(logDir);
        statusBar()->showMessage("Debug logs saved to Debug_Logs/", 3000);
    } else {
        statusBar()->showMessage("Error: AgentEngine not available", 3000);
    }
}

void MainWindow::loadStyleSheet() {
    // Initial Theme
    qApp->setStyleSheet(ThemeManager::instance().currentStyleSheet());
}

void MainWindow::switchSession(Session* session) {
    if (!session) return;   // guard against null (e.g. openSession failure)
    m_messageModel->setSession(session);
    m_chatView->scrollToBottom();
    setWindowTitle("CodeHex — " + session->title);
    updateTokenLabel();
}

void MainWindow::updateTokenLabel(int in, int out) {
    auto* s = m_sessions->currentSession();
    if (!s || !m_tokenLabel) return;

    int inputTokens = in >= 0 ? in : s->tokens.input;
    int outputTokens = out >= 0 ? out : s->tokens.output;

    double cost = (inputTokens / 1000000.0 * 5.0) + (outputTokens / 1000000.0 * 15.0);
    QString costStr = QString::number(cost, 'f', 4);

    if (in >= 0 && out >= 0) {
        m_tokenLabel->setText(QString("Tokens: %1 in / %2 out (~$%3) (streaming...)").arg(inputTokens).arg(outputTokens).arg(costStr));
    } else {
        m_tokenLabel->setText(QString("Tokens: %1 in / %2 out (~$%3)").arg(inputTokens).arg(outputTokens).arg(costStr));
    }
}

void MainWindow::onSendRequested(const QString& text, const QList<Attachment>& attachments) {
    m_controller->sendMessage(text, attachments);
}

void MainWindow::onSessionSelected(const QString& id) {
    Session* s = m_sessions->openSession(id);
    if (!s) return;
    m_sessions->setCurrentSession(s);
    switchSession(s);
}

void MainWindow::onNewSessionRequested() {
    Session* s = m_sessions->createSession(m_config->activeProviderId(), "default");
    if (!s) return;
    m_sessions->setCurrentSession(s);
    switchSession(s);
}

void MainWindow::onTokenReceived(const QString& token) {
    m_tokenBuffer += token;
    if (!m_tokenTimer->isActive()) {
        m_tokenTimer->start();
    }
}

void MainWindow::onTokenBufferTimeout() {
    if (m_tokenBuffer.isEmpty()) return;
    
    const QString tokens = m_tokenBuffer;
    m_tokenBuffer.clear();

    if (!m_hasStreamingMsg) {
        // First batch — create the assistant bubble
        m_streamingText = tokens;
        Message liveMsg;
        liveMsg.id = QUuid::createUuid();
        liveMsg.role = Message::Role::Assistant;
        liveMsg.contentBlocks.append(CodeBlock{tokens, BlockType::Text});
        liveMsg.contentTypes.append(Message::ContentType::Text);
        liveMsg.timestamp = QDateTime::currentDateTimeUtc();
        m_messageModel->appendMessage(liveMsg);
        m_hasStreamingMsg = true;
    } else {
        // Subsequent batches — accumulate and update in-place.
        m_streamingText += tokens;
        m_messageModel->updateLastMessage(m_streamingText);
    }
    if (m_chatView->autoScrollEnabled()) {
        m_chatView->scrollToBottom();
    }
}

void MainWindow::onCursorBlink() {
    if (!m_hasStreamingMsg) return;
    m_cursorVisible = !m_cursorVisible;
    m_messageModel->updateLastMessage(
        m_streamingText + (m_cursorVisible ? "▋" : ""));
}

void MainWindow::onResponseComplete(const Message& msg) {
    // Stop cursor, set final complete text (no cursor)
    m_cursorTimer->stop();
    m_tokenTimer->stop();
    m_isStreaming = false;
    m_cursorVisible = false;
    
    // Flush any remaining tokens in buffer
    if (!m_tokenBuffer.isEmpty()) {
        onTokenBufferTimeout();
    }

    if (m_hasStreamingMsg && !msg.textFromContentBlocks().isEmpty()) {
        m_messageModel->updateLastMessage(msg.textFromContentBlocks());
    } else if (!m_hasStreamingMsg) {
        // If it never streamed (e.g. short response, error, or JSON parsed at end)
        Message finalMsg = msg;
        if (msg.contentBlocks.isEmpty() && !msg.rawContent.isEmpty()) {
            CodeBlock textBlock;
            textBlock.type = BlockType::Text;
            textBlock.content = msg.rawContent;
            finalMsg.contentBlocks.append(textBlock);
        }
        if (!finalMsg.contentBlocks.isEmpty()) {
            m_messageModel->appendMessage(finalMsg);
        }
    }

    m_hasStreamingMsg = false;
    m_streamingText.clear();
    m_chatView->scrollToBottom();
    updateTokenLabel();
}

void MainWindow::onGenerationStarted() {
    m_isStreaming = true;
    m_cursorVisible = true;
    m_cursorTimer->start();
    m_chatBanner->setThinking(true);
    m_stopBtn->setEnabled(true);
    m_scrollToBottomBtn->setEnabled(false);
    m_inputPanel->setSendEnabled(false);
    m_inputPanel->setStopEnabled(true);
    m_stopBtn->setVisible(true);
    m_statusLabel->setVisible(true);
    m_statusLabel->setText("Agent is Thinking...");
    m_statusLabel->setStyleSheet(m_statusLabel->styleSheet().replace(QRegularExpression("color:[^;]+"), "color: #3B82F6").replace(QRegularExpression("border:[^;]+"), "border: 1px solid #3B82F6"));
    statusBar()->showMessage("Processing…");
}

void MainWindow::onGenerationStopped() {
    m_chatBanner->setThinking(false);
    m_cursorTimer->stop();
    m_isStreaming = false;
    m_cursorVisible = false;
    m_inputPanel->setSendEnabled(true);
    m_inputPanel->setStopEnabled(false);
    m_stopBtn->setVisible(false);
    m_statusLabel->setText("Ready");
    m_statusLabel->setStyleSheet(m_statusLabel->styleSheet().replace(QRegularExpression("color:[^;]+"), "color: #9CA3AF").replace(QRegularExpression("border:[^;]+"), "border: 1px solid #4B5563"));
    statusBar()->clearMessage();
    updateTokenLabel();
}

void MainWindow::onStopRequested() {
    m_controller->stopGeneration();
    m_stopBtn->setVisible(false);
}

void MainWindow::onToolApprovalRequested(const QString& toolName, const QJsonObject& input) {
    m_statusLabel->setText("Waiting for Tool Approval: " + toolName);
    m_statusLabel->setStyleSheet(m_statusLabel->styleSheet().replace(QRegularExpression("color:[^;]+"), "color: #F59E0B").replace(QRegularExpression("border:[^;]+"), "border: 1px solid #F59E0B"));
    
    ToolCall call;
    call.name = toolName;
    call.input = input;
    ToolApprovalDialog dlg(call, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_statusLabel->setText("Executing Tool: " + toolName);
        m_statusLabel->setStyleSheet(m_statusLabel->styleSheet().replace(QRegularExpression("color:[^;]+"), "color: #10B981").replace(QRegularExpression("border:[^;]+"), "border: 1px solid #10B981"));
        m_controller->approveToolCall(call);
    } else {
        onGenerationStopped();
    }
}

void MainWindow::updateProviderList() {
    m_providerCombo->blockSignals(true);
    m_providerCombo->clear();
    
    LlmProviderList providers = m_config->providers();
    QString activeId = m_config->activeProviderId();
    
    int activeIndex = -1;
    for (int i = 0; i < providers.size(); ++i) {
        const auto& p = providers[i];
        if (!p.active) continue;
        
        m_providerCombo->addItem(p.name, p.id);
        if (p.id == activeId) {
            activeIndex = m_providerCombo->count() - 1;
        }
    }
    
    if (activeIndex >= 0) {
        m_providerCombo->setCurrentIndex(activeIndex);
    } else if (m_providerCombo->count() > 0) {
        m_providerCombo->setCurrentIndex(0);
        m_config->setActiveProviderId(m_providerCombo->currentData().toString());
    }
    
    m_providerCombo->blockSignals(false);
}

void MainWindow::onProviderChanged(int index) {
    if (index < 0) return;
    const QString id = m_providerCombo->itemData(index).toString();
    m_config->setActiveProviderId(id);
    
    // Notify controller/agent to update runner profile
    m_controller->onProviderChanged();
}

void MainWindow::onClearChatRequested() {
    Session* s = m_sessions->currentSession();
    if (s) {
        s->clear();
        s->save();
        m_messageModel->clear();
    }
}

void MainWindow::onThemeToggleRequested() {
    auto& tm = ThemeManager::instance();
    bool nextDark = !tm.isDark();
    tm.setTheme(nextDark);
    m_themeBtn->setText(nextDark ? "🌙" : "☀️");
    updateButtonIcons();
}

void MainWindow::onCommandRequested(const QString& cmd, const QStringList& args) {
    QString lowerCmd = cmd.toLower();
    if (lowerCmd == "/clear") {
        onClearChatRequested();
    } else if (lowerCmd == "/reset") {
        onClearChatRequested();
        m_controller->resetAgent();
    } else if (lowerCmd == "/help") {
        Message helpMsg;
        helpMsg.role = Message::Role::Assistant;
        helpMsg.timestamp = QDateTime::currentDateTime();
        helpMsg.addText("🌟 **CodeHex Commands:**\n\n"
                        "- `/clear` : Clear the message window\n"
                        "- `/reset` : Reset the conversation & agent state\n"
                        "- `/help`  : Show this help message\n\n"
                        "Use **Up/Down arrows** to navigate your message history!");
        m_messageModel->appendMessage(helpMsg);
    } else {
        Message errMsg;
        errMsg.role = Message::Role::Assistant;
        errMsg.addText("❌ Unknown command: " + cmd);
        m_messageModel->appendMessage(errMsg);
    }
}

void MainWindow::updateButtonIcons() {
    bool isDark = ThemeManager::instance().isDark();
    QColor iconColor = isDark ? QColor("#E5E7EB") : QColor("#374151");
    
    auto tint = [&](const QString& path, std::optional<QColor> color = std::nullopt) -> QIcon {
        QColor targetColor = color.value_or(iconColor);
        QPixmap pixmap(64, 64);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        QSvgRenderer renderer(path);
        renderer.render(&painter);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), targetColor);
        painter.end();
        return QIcon(pixmap);
    };

    // Find children buttons with icons (or just update the ones we know)
    // We can use the member variables
    // Note: Some buttons might be created in setupUi and not have members if they were local,
    // but the key ones were requested. I'll update the ones I have members for.
    
    // We need to find the settings buttons again if they aren't members.
    // They were local in setupUi. Let's make them members if needed, 
    // or just find them by tooltips/object names.
    
    // Let's find all QPushButtons in the toolbar
    auto toolbar = findChild<QWidget*>("toolbar");
    if (toolbar) {
        for (auto* btn : toolbar->findChildren<QPushButton*>()) {
            QString iconPath;
            if (btn->toolTip().contains("Settings", Qt::CaseInsensitive)) iconPath = ":/resources/icons/settings.svg";
            else if (btn->toolTip().contains("Skills", Qt::CaseInsensitive)) iconPath = ":/resources/icons/skills.svg";
            else if (btn->toolTip().contains("Plugins", Qt::CaseInsensitive)) iconPath = ":/resources/icons/plugins.svg";
            else if (btn->toolTip().contains("Plugins", Qt::CaseInsensitive)) iconPath = ":/resources/icons/plugins.svg";
            else if (btn->toolTip().contains("Manage LLM", Qt::CaseInsensitive)) iconPath = ":/resources/icons/power.svg";
            else if (btn == m_debugBtn) iconPath = ":/resources/icons/bug.svg";
            
            if (!iconPath.isEmpty()) {
                // Use a lighter tint for the debug button icon if it's on a dark red background
                QColor currentIconColor = (btn == m_debugBtn) ? QColor("#FCA5A5") : iconColor;
                btn->setIcon(tint(iconPath, currentIconColor));
            }
        }
    }
}

// End of MainWindow.cpp
