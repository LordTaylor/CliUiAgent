#include "ChatController.h"
#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include "AppConfig.h"
#include "SessionManager.h"
#include "AgentEngine.h"
#include "ToolExecutor.h"
#include "../cli/CliRunner.h"

namespace CodeHex {

ChatController::ChatController(AppConfig* config,
                               SessionManager* sessions,
                               CliRunner* runner,
                               ScriptManager* scripts,
                               QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_sessions(sessions)
    , m_runner(runner)
    , m_scripts(scripts)
{
    qDebug() << "[ChatController] Constructor start";
    m_toolExecutor = new ToolExecutor(this);
    qDebug() << "[ChatController] ToolExecutor created";
    m_agent = new AgentEngine(config, sessions, runner, m_toolExecutor, this);
    qDebug() << "[ChatController] AgentEngine created";

    // Tunnel AgentEngine signals to UI (Item 31: Queued Connections)
    connect(m_agent, &AgentEngine::statusChanged,          this, &ChatController::statusChanged, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::tokenReceived,           this, &ChatController::tokenReceived, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::tokenStatsUpdated,    this, &ChatController::tokenStatsUpdated, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::consoleOutput,           this, &ChatController::consoleOutput, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::toolCallStarted,        this, &ChatController::toolCallStarted, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::contextStatsUpdated, this, &ChatController::contextStatsUpdated, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::toolApprovalRequested,   this, &ChatController::toolApprovalRequested, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::responseComplete,        this, [this](const Message& msg){
        emit responseComplete(msg);
        emit generationStopped();
    }, Qt::QueuedConnection);
    connect(m_agent, &AgentEngine::errorOccurred,           this, &ChatController::errorOccurred, Qt::QueuedConnection);
    
    // Sessions rename signaling (newly added to SessionManager)
    connect(m_sessions, &SessionManager::sessionRenamed, this, &ChatController::sessionRenamed, Qt::QueuedConnection);

    // Terminal connections
    connect(m_runner, &CliRunner::rawOutput, this, &ChatController::cliOutputReceived, Qt::QueuedConnection);
    connect(m_runner, &CliRunner::errorChunk, this, &ChatController::cliErrorReceived, Qt::QueuedConnection);
}

void ChatController::sendMessage(const QString& text, const QList<Attachment>& attachments) {
    qDebug() << "[ChatController] sendMessage called with text:" << text;
    if (text.isEmpty()) {
        qDebug() << "[ChatController] Text is empty, returning.";
        return;
    }

    auto session = m_sessions->currentSession();
    if (!session) {
        qCritical() << "[ChatController] ERROR: currentSession is NULL! Cannot send message.";
        return;
    }

    // Roadmap Item 15: Detect merge conflicts
    if (!text.contains("conflict", Qt::CaseInsensitive) && !text.contains("merge", Qt::CaseInsensitive)) {
        QProcess gitProc;
        gitProc.setWorkingDirectory(m_config->workingFolder());
        gitProc.start("git", {"ls-files", "--unmerged"});
        if (gitProc.waitForFinished(1000)) {
            QString unmerged = QString::fromLocal8Bit(gitProc.readAllStandardOutput()).trimmed();
            if (!unmerged.isEmpty()) {
                Message warnMsg;
                warnMsg.id = QUuid::createUuid();
                warnMsg.role = Message::Role::System;
                CodeBlock warnBlock;
                warnBlock.type = BlockType::Text;
                warnBlock.content = "⚠️ **Merge Conflicts Detected!**\n\nThere are unresolved Git merge conflicts in your repository. Please resolve them before asking me to modify code to prevent accidental corruption of the conflict markers.\n\n*(If you want my help to resolve them, just include the word 'conflict' in your message).*";
                warnMsg.contentBlocks << warnBlock;
                warnMsg.timestamp = QDateTime::currentDateTime();
                emit responseComplete(warnMsg); // show warning immediately
                return; // Stop processing
            }
        }
    }

    // Immediate UI feedback
    Message userMsg;
    userMsg.id = QUuid::createUuid();
    userMsg.role = Message::Role::User;
    
    CodeBlock block;
    block.type = BlockType::Text;
    block.content = text;
    userMsg.contentBlocks << block;
    
    userMsg.timestamp = QDateTime::currentDateTime();
    userMsg.attachments = attachments;
    
    emit userMessageReady(userMsg);

    emit generationStarted();
    m_agent->process(text, attachments);
}


void ChatController::stopGeneration() {
    m_agent->stop();
    emit generationStopped();
}

bool ChatController::isRunning() const {
    return m_agent->isRunning();
}

void ChatController::resetAgent() {
    m_agent->reset();
    emit statusChanged("Agent Reset");
    emit generationStopped();
}

void ChatController::setManualApproval(bool enabled) {
    m_agent->setManualApproval(enabled);
}

void ChatController::approveToolCall(const CodeHex::ToolCall& call) {
    m_agent->approveToolCall(call);
}

void ChatController::setSelectedModel(const QString& model) {
    if (m_agent) {
        m_agent->setSelectedModel(model);
    }
}

/**
 * @brief Triggered when the active LLM provider is changed in MainWindow.
 * Synchronizes internal state and notifies relevant components.
 */
void ChatController::onProviderChanged() {
    // Global runner update is handled by Application.
    emit statusChanged("Provider switch updated.");
}

void ChatController::onOutputChunk(const QString& chunk) {
    emit cliOutputReceived(chunk);
}

void ChatController::onRawOutput(const QString& raw) {
    emit consoleOutput(raw);
}

void ChatController::onErrorChunk(const QString& chunk) {
    emit cliErrorReceived(chunk);
}

void ChatController::onRunnerFinished(int exitCode) {
    Q_UNUSED(exitCode);
    emit generationStopped();
}

void ChatController::onToolCallReady(const CodeHex::ToolCall& call) {
    // Forward to UI
    emit toolCallStarted(call.name, call.input);
}

void ChatController::onSimpleCommandFinished(int exitCode, const QString& output, const QString& errorOutput) {
    Q_UNUSED(exitCode);
    Q_UNUSED(output);
    Q_UNUSED(errorOutput);
}

void ChatController::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result) {
    Q_UNUSED(toolName);
    Q_UNUSED(result);
}

}  // namespace CodeHex