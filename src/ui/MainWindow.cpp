#include "MainWindow.h"
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include "help/HelpDialog.h"
#include "../audio/AudioPlayer.h"
#include "../audio/AudioRecorder.h"
#include "../cli/ClaudeProfile.h"
#include "../cli/CliRunner.h"
#include "../cli/GptProfile.h"
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
                       QWidget* parent)
    : QMainWindow(parent),
      m_config(config),
      m_sessions(sessions),
      m_controller(controller),
      m_recorder(recorder),
      m_player(player) {
    setWindowTitle("CodeHex");
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
    connect(m_controller, &ChatController::errorOccurred,
            m_console, &ConsoleWidget::appendText);

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
    tbLayout->addWidget(m_profileCombo);

    rightLayout->addWidget(toolbar);

    // Chat view
    m_messageModel = new MessageModel(this);
    m_chatView = new ChatView(rightWidget);
    m_chatView->setMessageModel(m_messageModel);
    connect(m_chatView, &ChatView::loadMoreRequested,
            m_messageModel, &MessageModel::loadMoreMessages);
    rightLayout->addWidget(m_chatView, 1);

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
        {"Claude Code &Wizard",    "wizard-claude-code",   ""},
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
    QMessageBox::about(this,
        "About CodeHex",
        "<h2>CodeHex 0.1.0</h2>"
        "<p>A desktop coding chatbot for developers.</p>"
        "<p>Supports <b>Claude CLI</b>, <b>Ollama</b>, and <b>OpenAI</b> "
        "backends with Lua/Python scripting hooks.</p>"
        "<p>Built with Qt6/C++ · <a href='https://github.com/LordTaylor/CliUiAgent'>"
        "GitHub</a></p>"
    );
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
    m_profileCombo->addItem("Claude CLI",  "claude");
    m_profileCombo->addItem("Ollama",      "ollama");
    m_profileCombo->addItem("OpenAI/sgpt", "gpt");

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
    m_messageModel->setSession(session);
    m_chatView->scrollToBottom();
    setWindowTitle("CodeHex — " + (session ? session->title : QString("No session")));
}

void MainWindow::onSendRequested(const QString& text, const QList<Attachment>& attachments) {
    m_controller->sendMessage(text, attachments);
}

void MainWindow::onStopRequested() {
    m_controller->stopGeneration();
}

void MainWindow::onSessionSelected(const QString& id) {
    Session* s = m_sessions->openSession(id);
    if (!s) return;
    m_sessions->setCurrentSession(s);
    switchSession(s);
}

void MainWindow::onNewSessionRequested() {
    Session* s = m_sessions->createSession(m_config->activeProfile(), "default");
    m_sessions->setCurrentSession(s);
    switchSession(s);
}

void MainWindow::onTokenReceived(const QString& token) {
    // Update or append the live streaming assistant message
    if (!m_hasStreamingMsg) {
        m_streamingText.clear();
        Message liveMsg;
        liveMsg.id = QUuid::createUuid();
        liveMsg.role = Message::Role::Assistant;
        liveMsg.contentType = Message::ContentType::Text;
        liveMsg.text = token;
        liveMsg.timestamp = QDateTime::currentDateTimeUtc();
        m_messageModel->appendMessage(liveMsg);
        m_hasStreamingMsg = true;
    } else {
        m_streamingText += token;
        // Update last row in-place
        const int lastRow = m_messageModel->rowCount() - 1;
        if (lastRow >= 0) {
            // We use a workaround: setData is not exposed, so we re-append
            // For a production version, expose a updateLastMessage() on the model
        }
    }
    m_chatView->scrollToBottom();
}

void MainWindow::onResponseComplete(const Message& msg) {
    Q_UNUSED(msg)
    m_hasStreamingMsg = false;
    m_streamingText.clear();
    m_chatView->scrollToBottom();
}

void MainWindow::onGenerationStarted() {
    m_inputPanel->setSendEnabled(false);
    m_inputPanel->setStopEnabled(true);
    statusBar()->showMessage("Generating…");
}

void MainWindow::onGenerationStopped() {
    m_inputPanel->setSendEnabled(true);
    m_inputPanel->setStopEnabled(false);
    statusBar()->clearMessage();
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
