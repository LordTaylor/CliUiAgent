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
    m_toolExecutor = new ToolExecutor(this);
    m_agent = new AgentEngine(config, sessions, runner, m_toolExecutor, this);

    // Tunnel AgentEngine signals to UI
    connect(m_agent, &AgentEngine::statusChanged,          this, &ChatController::statusChanged);
    connect(m_agent, &AgentEngine::tokenReceived,           this, &ChatController::tokenReceived);
    connect(m_agent, &AgentEngine::consoleOutput,           this, &ChatController::consoleOutput);
    connect(m_agent, &AgentEngine::toolCallStarted,        this, &ChatController::toolCallStarted);
    connect(m_agent, &AgentEngine::toolApprovalRequested,   this, &ChatController::toolApprovalRequested);
    connect(m_agent, &AgentEngine::responseComplete,        this, [this](const Message& msg){
        emit responseComplete(msg);
        emit generationStopped();
    });
    connect(m_agent, &AgentEngine::errorOccurred,           this, &ChatController::errorOccurred);
    
    // Sessions rename signaling (newly added to SessionManager)
    connect(m_sessions, &SessionManager::sessionRenamed, this, &ChatController::sessionRenamed);
}

void ChatController::sendMessage(const QString& text, const QList<Attachment>& attachments) {
    if (text.isEmpty()) return;

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

void ChatController::setManualApproval(bool enabled) {
    m_agent->setManualApproval(enabled);
}

void ChatController::approveToolCall(const CodeHex::ToolCall& call) {
    m_agent->approveToolCall(call);
}

}  // namespace CodeHex