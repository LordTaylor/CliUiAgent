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
#include "help/HelpDialog.h"
#include "../audio/AudioPlayer.h"
#include "../audio/AudioRecorder.h"
#include "../cli/CliRunner.h"
#include "../cli/OllamaProfile.h"
#include "../core/AppConfig.h"
#include "../core/ChatController.h"
#include "../core/SessionManager.h"
#include "../data/Message.h"
#include "chat/ChatView.h"
#include "chat/MessageModel.h"
#include "console/ConsoleWidget.h"
#include "input/InputPanel.h"
#include "session/SessionPanel.h"
#include "workfolder/WorkFolderSelector.h"

namespace CodeHex {

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
    populateProfileCombo();

    // Connect ChatController signals
    connect(m_controller, &ChatController::userMessageReady, this,
            [this](const Message& msg) { m_messageModel->appendMessage(msg); });
    connect(m_controller, &ChatController::tokenReceived,
            this, &MainWindow::onTokenReceived);
    connect(m_controller, &ChatController::responseComplete,
            this, &MainWindow::onResponseComplete);
    connect(m_controller, &ChatController::consoleOutput,
            m_console, &ConsoleWidget::appendText);
    connect(m_controller, &ChatController::generationStarted,
            this, &MainWindow::onGenerationStarted);
    connect(m_controller, &ChatController::generationStopped,
            this, &MainWindow::onGenerationStopped);
    connect(m_controller, &ChatController::generationStopped,
            this, [this]() { m_scrollToBottomBtn->setEnabled(true); });
    connect(m_controller, &ChatController::errorOccurred,
            m_console, &ConsoleWidget::appendText);
    connect(m_controller, &ChatController::statusChanged,
            this, [this](const QString& status) { m_statusLabel->setText(status); });
    connect(m_controller, &ChatController::toolApprovalRequested,
            this, &MainWindow::onToolApprovalRequested);
    connect(m_controller, &ChatController::sessionRenamed,
            this, [this](const QString& /*id*/, const QString& title) {
                m_sessionPanel->refresh();
                setWindowTitle("CodeHex — " + title);
            });

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
        ? m_sessions->createSession(m_config->activeProfile(), "default")
        : m_sessions->allSessions().first();
    m_sessions->setCurrentSession(last);
    switchSession(last);
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Horizontal splitter: SessionPanel | Chat area ---
    m_splitter = new QSplitter(Qt::Horizontal, central);

    // Session panel (left sidebar)
    m_sessionPanel = new SessionPanel(m_sessions, m_splitter);
    m_sessionPanel->setMinimumWidth(180);
    m_sessionPanel->setMaximumWidth(280);
    m_splitter->addWidget(m_sessionPanel);

    // Right: chat + input + console
    auto* rightWidget = new QWidget(m_splitter);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Toolbar row: folder selector + profile combo
    auto* toolbar = new QWidget(rightWidget);
    toolbar->setObjectName("toolbar");
    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(8, 4, 8, 4);
    tbLayout->setSpacing(8);

    m_folderSelector = new WorkFolderSelector(toolbar);
    m_folderSelector->setFolder(m_config->workingFolder());
    connect(m_folderSelector, &WorkFolderSelector::folderChanged,
            m_config, &AppConfig::setWorkingFolder);
    tbLayout->addWidget(m_folderSelector, 1);

    QLabel* profileLbl = new QLabel("CLI:", toolbar);
    tbLayout->addWidget(profileLbl);
    m_profileCombo = new QComboBox(toolbar);
    m_profileCombo->setObjectName("profileCombo");
    m_profileCombo->setMinimumWidth(120);
    m_profileCombo->setMinimumWidth(120);
    tbLayout->addWidget(m_profileCombo);

    tbLayout->addSpacing(10);
    m_agentModeCheck = new QCheckBox("Agent Mode (Manual Approval)", toolbar);
    m_agentModeCheck->setChecked(m_config->manualApproval());
    m_controller->setManualApproval(m_agentModeCheck->isChecked());
    tbLayout->addWidget(m_agentModeCheck);

    connect(m_agentModeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_config->setManualApproval(checked);
        m_controller->setManualApproval(checked);
    });

    rightLayout->addWidget(toolbar);

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

    // Console
    m_console = new ConsoleWidget(rightWidget);
    rightLayout->addWidget(m_console);

    m_splitter->addWidget(rightWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_splitter);

    // Wire input panel
    connect(m_inputPanel, &InputPanel::sendRequested,
            this, &MainWindow::onSendRequested);
    connect(m_inputPanel, &InputPanel::stopRequested,
            this, &MainWindow::onStopRequested);

    // Wire session panel
    connect(m_sessionPanel, &SessionPanel::sessionSelected,
            this, &MainWindow::onSessionSelected);
    connect(m_sessionPanel, &SessionPanel::newSessionRequested,
            this, &MainWindow::onNewSessionRequested);

    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProfileChanged);

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
    connect(toggleConsole, &QAction::triggered, m_console, &ConsoleWidget::toggleExpanded);

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
    QMessageBox about(this);
    about.setWindowTitle("About CodeHex");
    about.setIconPixmap(QPixmap(":/resources/icons/app.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    about.setText("<h2>CodeHex 0.1.0</h2>"
        "<p>A desktop coding chatbot for developers.</p>"
        "<p>Supports <b>LM Studio</b>, <b>Ollama</b>, and other local "
        "OpenAI-compatible LLMs with Lua/Python scripting hooks.</p>"
        "<p>Built with Qt6/C++ · <a href='https://github.com/LordTaylor/CliUiAgent'>"
        "GitHub</a></p>");
    about.exec();
}

void MainWindow::loadStyleSheet() {
    QFile f(":/stylesheets/dark.qss");
    if (f.open(QIODevice::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
    }
}

void MainWindow::populateProfileCombo() {
    m_profileCombo->blockSignals(true);
    m_profileCombo->clear();

    // ── Built-in profiles (local-only) ────────────────────────────
    m_profileCombo->addItem("Ollama",      "ollama");

    // ── Extra profiles from ~/.codehex/profiles/ ──────────────────
    if (!m_extraProfiles.isEmpty()) {
        m_profileCombo->insertSeparator(m_profileCombo->count());
        for (const auto& entry : m_extraProfiles)
            m_profileCombo->addItem(entry.displayName, entry.name);
    }

    // Restore selection
    const QString active = m_config->activeProfile();
    for (int i = 0; i < m_profileCombo->count(); ++i) {
        if (m_profileCombo->itemData(i).toString() == active) {
            m_profileCombo->setCurrentIndex(i);
            break;
        }
    }
    m_profileCombo->blockSignals(false);
}

void MainWindow::switchSession(Session* session) {
    if (!session) return;   // guard against null (e.g. openSession failure)
    m_messageModel->setSession(session);
    m_chatView->scrollToBottom();
    setWindowTitle("CodeHex — " + session->title);
    updateTokenLabel();
}

void MainWindow::updateTokenLabel() {
    auto* s = m_sessions->currentSession();
    if (!s || (s->tokens.input == 0 && s->tokens.output == 0)) {
        m_tokenLabel->clear();
        return;
    }
    m_tokenLabel->setText(
        QString("Tokens: %1 in / %2 out").arg(s->tokens.input).arg(s->tokens.output));
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
    Session* s = m_sessions->createSession(m_config->activeProfile(), "default");
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

    if (m_hasStreamingMsg && !msg.textFromContentBlocks().isEmpty())
        m_messageModel->updateLastMessage(msg.textFromContentBlocks());
    m_hasStreamingMsg = false;
    m_streamingText.clear();
    m_chatView->scrollToBottom();
    updateTokenLabel();
}

void MainWindow::onGenerationStarted() {
    m_isStreaming = true;
    m_cursorVisible = true;
    m_cursorTimer->start();
    m_inputPanel->setSendEnabled(false);
    m_inputPanel->setStopEnabled(true);
    m_stopBtn->setVisible(true);
    m_statusLabel->setVisible(true);
    m_statusLabel->setText("Agent is Thinking...");
    m_statusLabel->setStyleSheet(m_statusLabel->styleSheet().replace(QRegularExpression("color:[^;]+"), "color: #3B82F6").replace(QRegularExpression("border:[^;]+"), "border: 1px solid #3B82F6"));
    statusBar()->showMessage("Processing…");
}

void MainWindow::onGenerationStopped() {
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

void MainWindow::onProfileChanged(int index) {
    const QString name = m_profileCombo->itemData(index).toString();
    m_config->setActiveProfile(name);
    m_config->save();

    // Swap CliRunner profile
    // This requires access to CliRunner — accessed through controller
    // For now, emit a signal that Application can handle
}

}  // namespace CodeHex
