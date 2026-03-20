#include "AgentEngine.h"
#include <QDebug>
#include <QJsonDocument>
#include <QTimer>
#include "ToolExecutor.h"
#include "SessionManager.h"
#include "AppConfig.h"
#include "TokenCounter.h"
#include "../cli/CliRunner.h"
#include "../data/Session.h"

namespace CodeHex {

AgentEngine::AgentEngine(AppConfig* config, 
                         SessionManager* sessions, 
                         CliRunner* runner, 
                         ToolExecutor* toolExecutor,
                         QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_sessions(sessions)
    , m_runner(runner)
    , m_toolExecutor(toolExecutor)
{
    connect(m_runner, &CliRunner::outputChunk,    this, &AgentEngine::onOutputChunk);
    connect(m_runner, &CliRunner::rawOutput,      this, &AgentEngine::onRawOutput);
    connect(m_runner, &CliRunner::errorChunk,     this, &AgentEngine::onErrorChunk);
    connect(m_runner, &CliRunner::toolCallReady,  this, &AgentEngine::onToolCallReady);
    connect(m_runner, &CliRunner::finished,       this, &AgentEngine::onRunnerFinished);
    
    connect(m_toolExecutor, &ToolExecutor::toolFinished, this, &AgentEngine::onToolResultReceived);
}

void AgentEngine::process(const QString& userInput, const QList<Attachment>& attachments) {
    auto* session = m_sessions->currentSession();
    if (!session) return;

    // Prepare message
    Message userMsg;
    userMsg.id = QUuid::createUuid();
    userMsg.role = Message::Role::User;
    
    CodeBlock block;
    block.type = BlockType::Text;
    block.content = userInput;
    userMsg.contentBlocks << block;
    
    userMsg.timestamp = QDateTime::currentDateTime();
    userMsg.attachments = attachments;

    session->appendMessage(userMsg);
    session->save();

    m_currentResponse.clear();
    emit statusChanged("Agent is thinking...");

    // Build enriched prompt
    QString enrichedPrompt = userInput; 
    
    // Update input token stats
    const int inputTokens = TokenCounter::estimate(enrichedPrompt);
    session->updateTokens(inputTokens, 0);

    QStringList imagePaths;
    for (const auto& att : attachments) {
        if (att.type == Attachment::Type::Image) {
            imagePaths << att.filePath;
        }
    }

    m_runner->send(enrichedPrompt, m_config->workingFolder(), imagePaths, session->messages);
}

void AgentEngine::stop() {
    m_runner->stop();
}

bool AgentEngine::isRunning() const {
    return m_runner->isProfileRunning();
}

void AgentEngine::onOutputChunk(const QString& chunk) {
    m_currentResponse += chunk;
    emit tokenReceived(chunk);
}

void AgentEngine::onRawOutput(const QString& raw) {
    emit consoleOutput(raw);
}

void AgentEngine::onErrorChunk(const QString& chunk) {
    emit consoleOutput("[stderr] " + chunk);
}

void AgentEngine::onToolCallReady(const CodeHex::ToolCall& call) {
    qDebug() << "[AgentEngine] Tool call ready:" << call.name;
    emit toolCallStarted(call.name, call.input);

    static const QStringList kDangerous = { "WriteFile", "Write", "RunCommand", "Bash", "Replace", "Sed" };

    if (m_manualApproval && kDangerous.contains(call.name)) {
        emit statusChanged("Waiting for approval...");
        emit toolApprovalRequested(call.name, call.input);
        return;
    }

    emit statusChanged(QString("Executing %1...").arg(call.name));
    m_toolExecutor->execute(call, m_config->workingFolder());
}

void AgentEngine::approveToolCall(const ToolCall& call) {
    emit statusChanged(QString("Executing %1...").arg(call.name));
    m_toolExecutor->execute(call, m_config->workingFolder());
}

void AgentEngine::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result) {
    Q_UNUSED(toolName);
    qDebug() << "[AgentEngine] Tool result received for:" << toolName;
    
    auto* session = m_sessions->currentSession();
    if (!session) return;

    // Convert ToolResult to a Message (normally this is handled as part of the tool turn)
    Message toolMsg;
    toolMsg.id = QUuid::createUuid();
    toolMsg.role = Message::Role::User; // In some APIs this is Role::Tool
    
    CodeBlock block;
    block.type = BlockType::Text; // Or a specific 'Result' type if available
    block.content = result.content;
    toolMsg.contentBlocks << block;
    toolMsg.timestamp = QDateTime::currentDateTime();

    session->appendMessage(toolMsg);
    session->save();

    if (!m_runner->isProfileRunning()) {
        emit statusChanged("Agent is processing tool result...");
        m_runner->send("", m_config->workingFolder(), {}, session->messages);
    }
}

void AgentEngine::onRunnerFinished(int exitCode) {
    Q_UNUSED(exitCode);
    if (m_currentResponse.trimmed().isEmpty()) {
        emit statusChanged("");
        return;
    }

    buildAssistantMessage(m_currentResponse);
}

void AgentEngine::buildAssistantMessage(const QString& plainText) {
    auto* session = m_sessions->currentSession();
    if (!session) return;

    Message msg;
    msg.id = QUuid::createUuid();
    msg.role = Message::Role::Assistant;
    msg.timestamp = QDateTime::currentDateTime();

    CodeBlock block;
    block.type = BlockType::Text;
    block.content = plainText;
    msg.contentBlocks << block;

    // Handle session auto-rename if first message
    if (session->messages.size() <= 2) { 
        QString title = plainText.section(QRegularExpression("[.!?]"), 0, 0).trimmed();
        if (title.length() > 40) title = title.left(37) + "...";
        if (!title.isEmpty()) {
            session->title = title;
        }
    }

    // Try to separate Thought from Result if tags are present
    QString content = plainText;
    if (content.contains("<thought>") && content.contains("</thought>")) {
        int start = content.indexOf("<thought>") + 9;
        int end = content.indexOf("</thought>");
        QString thought = content.mid(start, end - start).trimmed();
        QString remaining = content.mid(end + 10).trimmed();

        if (!thought.isEmpty()) {
            CodeBlock thoughtBlock;
            thoughtBlock.type = BlockType::Thinking;
            thoughtBlock.content = thought;
            msg.contentTypes << Message::ContentType::Thinking;
            msg.contentBlocks << thoughtBlock;
        }
        
        if (!remaining.isEmpty()) {
            CodeBlock textBlock;
            textBlock.type = BlockType::Text;
            textBlock.content = remaining;
            msg.contentTypes << Message::ContentType::Text;
            msg.contentBlocks << textBlock;
        }
    } else {
        CodeBlock block;
        block.type = BlockType::Text;
        block.content = plainText;
        msg.contentBlocks << block;
        msg.contentTypes << Message::ContentType::Text;
    }

    session->appendMessage(msg);
    session->save();
    
    emit responseComplete(msg);
    emit statusChanged("");
}

} // namespace CodeHex
