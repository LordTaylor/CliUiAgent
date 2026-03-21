/**
 * @file MainWindow_Slots.cpp
 * @brief Session management, commands, drag-drop, and dialog slots:
 *        onSendRequested, onSessionSelected, onNewSessionRequested,
 *        onClearChatRequested, onThemeToggleRequested, onCommandRequested,
 *        onHelpRequested, onAbout, onUpdateAvailable,
 *        onDebugLogRequested, dragEnterEvent, dropEvent.
 */
#include "MainWindow.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>
#include <QUuid>
#include "../core/AppConfig.h"
#include "../core/AgentEngine.h"
#include "../core/ChatController.h"
#include "../core/SessionManager.h"
#include "chat/MessageModel.h"
#include "input/InputPanel.h"
#include "session/SessionPanel.h"
#include "help/HelpDialog.h"

using namespace CodeHex;

void MainWindow::onSendRequested(const QString& text, const QList<Attachment>& attachments) {
    if (m_loopWarningBanner) m_loopWarningBanner->setVisible(false);
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
    m_sessionPanel->selectSession(s->id.toString(QUuid::WithoutBraces));
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
    Q_UNUSED(args);
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

void MainWindow::onUpdateAvailable(const QString& version, const QString& url) {
    QMessageBox::StandardButton res = QMessageBox::information(this, "Update Available",
        QString("A new version (%1) of CodeHex is available!\n"
                "Would you like to view the release page?").arg(version),
        QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::onDebugLogRequested() {
    QString logDir = QDir(m_config->workingFolder()).filePath("Debug_Logs");
    QDir dir(logDir);
    if (dir.exists()) dir.removeRecursively();
    dir.mkpath(".");

    if (m_controller && m_controller->agent()) {
        m_controller->agent()->saveDebugLog(logDir);
        statusBar()->showMessage("Debug logs saved to Debug_Logs/", 3000);
    } else {
        statusBar()->showMessage("Error: AgentEngine not available", 3000);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    QList<Attachment> newAttachments;
    for (const QUrl& url : urls) {
        QString localPath = url.toLocalFile();
        if (localPath.isEmpty()) continue;
        Attachment att;
        att.filePath = localPath;
        QString ext = QFileInfo(localPath).suffix().toLower();
        if (ext == "png" || ext == "jpg" || ext == "jpeg") att.type = Attachment::Type::Image;
        else if (ext == "mp3" || ext == "wav" || ext == "m4a") att.type = Attachment::Type::Audio;
        else att.type = Attachment::Type::File;
        newAttachments.append(att);
    }
    if (!newAttachments.isEmpty()) m_inputPanel->addAttachments(newAttachments);
}
