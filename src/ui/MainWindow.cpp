/**
 * @file MainWindow.cpp
 * @brief Constructor, setupMenuBar, loadStyleSheet, and UI utility methods.
 *
 * Implementation split across:
 *   MainWindow_SetupUi.cpp    — widget tree construction (setupUi)
 *   MainWindow_Generation.cpp — streaming/generation UI handlers
 *   MainWindow_Slots.cpp      — session management, commands, drag-drop, dialogs
 */
#include "MainWindow.h"
#include <QApplication>
#include <QIcon>
#include <QKeySequence>
#include <QMenuBar>
#include <QPainter>
#include <QStatusBar>
#include <QSvgRenderer>
#include <QTimer>
#include <optional>
#include "../audio/AudioPlayer.h"
#include "../audio/AudioRecorder.h"
#include "../core/AppConfig.h"
#include "../core/ChatController.h"
#include "../core/LlmDiscoveryService.h"
#include "../core/SessionManager.h"
#include "UpdateChecker.h"
#include "chat/ChatControlBanner.h"
#include "chat/ChatView.h"
#include "chat/MessageModel.h"
#include "widgets/PixelCauldron.h"
#include "help/HelpDialog.h"
#include "input/InputPanel.h"
#include "session/SessionPanel.h"

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
      m_extraProfiles(extraProfiles)
{
    setWindowTitle("CodeHex");
    setWindowIcon(QIcon(":/resources/icons/app.png"));
    setMinimumSize(900, 600);
    resize(1200, 800);

    setupUi();
    setupMenuBar();
    loadStyleSheet();
    updateProviderList();

    m_updateChecker = new UpdateChecker("1.2.0", this);
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &MainWindow::onUpdateAvailable);
    QTimer::singleShot(2000, m_updateChecker, &UpdateChecker::checkForUpdates);

    setAcceptDrops(true);

    connect(m_controller, &ChatController::userMessageReady, this,
            [this](const Message& msg) { m_messageModel->appendMessage(msg); });
    connect(m_controller, &ChatController::tokenReceived, this, [this](const QString& chunk) {
        m_messageModel->appendToken(chunk);
        m_chatView->scrollToBottomSmooth();
    });

    m_contextBar = new QProgressBar(this);
    m_contextBar->setMaximumWidth(150);
    m_contextBar->setTextVisible(false);
    m_contextBar->setToolTip("Context Usage (Tokens)");
    m_contextBar->setStyleSheet(
        "QProgressBar { border: 1px solid #3F3F46; border-radius: 4px; background: #18181B; } "
        "QProgressBar::chunk { background: #3B82F6; border-radius: 3px; }");

    m_contextLabel = new QLabel("CTX: 0%", this);
    m_contextLabel->setStyleSheet("color: #A1A1AA; font-size: 11px; margin-left: 5px;");

    statusBar()->addPermanentWidget(m_contextBar);
    statusBar()->addPermanentWidget(m_contextLabel);

    connect(m_controller, &ChatController::contextStatsUpdated, this, &MainWindow::onContextStatsUpdated);
    connect(m_controller, &ChatController::tokenStatsUpdated,   this, &MainWindow::onTokenStatsUpdated);
    connect(m_controller, &ChatController::responseComplete,    this, &MainWindow::onResponseComplete);
    connect(m_controller, &ChatController::cliOutputReceived,   m_terminalPanel, &TerminalPanel::appendOutput);
    connect(m_controller, &ChatController::cliErrorReceived,    m_terminalPanel, &TerminalPanel::appendError);
    connect(m_controller, &ChatController::generationStarted,   this, &MainWindow::onGenerationStarted);
    connect(m_controller, &ChatController::generationStopped,   this, &MainWindow::onGenerationStopped);
    connect(m_controller, &ChatController::generationStopped,
            this, [this]() { m_scrollToBottomBtn->setEnabled(true); });
    connect(m_controller, &ChatController::errorOccurred,
            m_terminalPanel, &TerminalPanel::appendError);
    connect(m_controller, &ChatController::statusChanged,
            this, [this](const QString& status) {
                m_statusLabel->setText(status);
                m_chatBanner->setStatusText(status);
                if (m_cauldron) m_cauldron->setStatus(status);
            });
    connect(m_controller, &ChatController::toolApprovalRequested,
            this, &MainWindow::onToolApprovalRequested);
    connect(m_controller, &ChatController::potentialLoopDetected, this, [this](const QString& msg){
        if (m_loopWarningBanner) {
            m_loopWarningBanner->setVisible(true);
            m_loopWarningBanner->raise();
        }
    });
    connect(m_controller, &ChatController::sessionRenamed,
            this, [this](const QString& /*id*/, const QString& title) {
                m_sessionPanel->refresh();
                setWindowTitle("CodeHex — " + title);
            });

    m_cursorTimer = new QTimer(this);
    m_cursorTimer->setInterval(500);
    connect(m_cursorTimer, &QTimer::timeout, this, &MainWindow::onCursorBlink);

    m_tokenTimer = new QTimer(this);
    m_tokenTimer->setInterval(50);
    m_tokenTimer->setSingleShot(true);
    connect(m_tokenTimer, &QTimer::timeout, this, &MainWindow::onTokenBufferTimeout);

    m_sessions->loadAll();
    Session* last = m_sessions->allSessions().isEmpty()
        ? m_sessions->createSession(m_config->activeProviderId(), "default")
        : m_sessions->allSessions().first();
    m_sessions->setCurrentSession(last);
    switchSession(last);
    updateButtonIcons();

    connect(m_sessionPanel, &SessionPanel::newSessionRequested, this, &MainWindow::onNewSessionRequested);
    connect(m_sessionPanel, &SessionPanel::sessionSelected,     this, &MainWindow::onSessionSelected);
    connect(m_chatView,     &ChatView::rerunRequested,           this, [this](const QString& text){
        onSendRequested("Re-run tool command: " + text, {});
    });
}

void MainWindow::setupMenuBar() {
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

    QMenu* viewMenu = menuBar()->addMenu("&View");

    QAction* toggleSidebar = viewMenu->addAction("Toggle &Sidebar");
    toggleSidebar->setShortcut(QKeySequence("Ctrl+B"));
    connect(toggleSidebar, &QAction::triggered, this, [this]() {
        m_sidebarToggleBtn->click();
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

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    struct HelpEntry { QString label, page, shortcut; };
    const QList<HelpEntry> helpPages = {
        {"&Getting Started",       "getting-started",       "F1"},
        {"&Interface Guide",       "ui-guide",              ""},
        {"&Sessions",              "sessions",              ""},
        {"&CLI Profiles & Models", "cli-profiles",          ""},
        {"Sc&ripting (Lua/Python)","scripting",             ""},
        {"&Voice && Attachments",  "voice-and-attachments", ""},
        {"&Keyboard Shortcuts",    "keyboard-shortcuts",    ""},
    };
    for (const auto& entry : helpPages) {
        QAction* act = helpMenu->addAction(entry.label);
        if (!entry.shortcut.isEmpty()) act->setShortcut(QKeySequence(entry.shortcut));
        const QString page = entry.page;
        connect(act, &QAction::triggered, this, [this, page]() { onHelpRequested(page); });
    }
    helpMenu->addSeparator();
    QAction* aboutAct = helpMenu->addAction("&About CodeHex");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::loadStyleSheet() {
    qApp->setStyleSheet(ThemeManager::instance().currentStyleSheet());
}

void MainWindow::switchSession(Session* session) {
    if (!session) return;
    m_messageModel->setSession(session);
    m_chatView->scrollToBottom();
    setWindowTitle("CodeHex — " + session->title);
    updateTokenLabel();
}

void MainWindow::updateTokenLabel(int in, int out) {
    auto* s = m_sessions->currentSession();
    if (!s || !m_tokenLabel) return;

    int inputTokens  = in  >= 0 ? in  : s->tokens.input;
    int outputTokens = out >= 0 ? out : s->tokens.output;
    double cost = (inputTokens / 1000000.0 * 5.0) + (outputTokens / 1000000.0 * 15.0);
    QString costStr = QString::number(cost, 'f', 4);

    m_tokenLabel->setText(
        in >= 0 && out >= 0
        ? QString("Tokens: %1 in / %2 out (~$%3) (streaming...)").arg(inputTokens).arg(outputTokens).arg(costStr)
        : QString("Tokens: %1 in / %2 out (~$%3)").arg(inputTokens).arg(outputTokens).arg(costStr));
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
        if (p.id == activeId) activeIndex = m_providerCombo->count() - 1;
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
    m_config->setActiveProviderId(m_providerCombo->itemData(index).toString());
    m_controller->onProviderChanged();
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

    auto* toolbar = findChild<QWidget*>("toolbar");
    if (!toolbar) return;
    for (auto* btn : toolbar->findChildren<QPushButton*>()) {
        QString iconPath;
        if      (btn->toolTip().contains("Settings", Qt::CaseInsensitive)) iconPath = ":/resources/icons/settings.svg";
        else if (btn->toolTip().contains("Skills",   Qt::CaseInsensitive)) iconPath = ":/resources/icons/skills.svg";
        else if (btn->toolTip().contains("Plugins",  Qt::CaseInsensitive)) iconPath = ":/resources/icons/plugins.svg";
        else if (btn->toolTip().contains("Manage LLM", Qt::CaseInsensitive)) iconPath = ":/resources/icons/power.svg";
        else if (btn == m_debugBtn) iconPath = ":/resources/icons/bug.svg";

        if (!iconPath.isEmpty()) {
            QColor currentIconColor = (btn == m_debugBtn) ? QColor("#FCA5A5") : iconColor;
            btn->setIcon(tint(iconPath, currentIconColor));
        }
    }
}
