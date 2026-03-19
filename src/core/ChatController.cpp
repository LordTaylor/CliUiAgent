#include "ChatController.h"
#include <QDateTime>
#include <QUuid>
#include "../cli/CliRunner.h"
#include "../scripting/ScriptManager.h"
#include "AppConfig.h"
#include "SessionManager.h"
#include "TokenCounter.h"

namespace CodeHex {

ChatController::ChatController(AppConfig* config,
                               SessionManager* sessions,
                               CliRunner* runner,
                               ScriptManager* scripts,
                               QObject* parent)
    : QObject(parent),
      m_config(config),
      m_sessions(sessions),
      m_runner(runner),
      m_scripts(scripts) {
    connect(m_runner, &CliRunner::outputChunk, this, &ChatController::onOutputChunk);
    connect(m_runner, &CliRunner::errorChunk,  this, &ChatController::onErrorChunk);
    connect(m_runner, &CliRunner::finished,    this, &ChatController::onRunnerFinished);
}

void ChatController::sendMessage(const QString& text, const QList<Attachment>& attachments) {
    if (text.trimmed().isEmpty() && attachments.isEmpty()) return;

    auto* session = m_sessions->currentSession();
    if (!session) {
        session = m_sessions->createSession(m_config->activeProfile(), "default");
        m_sessions->setCurrentSession(session);
    }

    // Build user message
    Message userMsg;
    userMsg.id = QUuid::createUuid();
    userMsg.role = Message::Role::User;
    userMsg.contentType = Message::ContentType::Text;
    userMsg.text = text;
    userMsg.attachments = attachments;
    userMsg.timestamp = QDateTime::currentDateTimeUtc();
    userMsg.tokenCount = TokenCounter::estimate(text);

    // Pre-send scripting hooks
    if (m_scripts) m_scripts->runHook("pre_send", {{"text", text}});

    session->appendMessage(userMsg);
    session->save();
    emit userMessageReady(userMsg);

    // Start streaming response
    m_currentResponse.clear();
    emit generationStarted();
    m_runner->send(text, m_config->workingFolder());
}

void ChatController::stopGeneration() {
    m_runner->stop();
}

void ChatController::onOutputChunk(const QString& chunk) {
    m_currentResponse += chunk;
    emit tokenReceived(chunk);
    emit consoleOutput(chunk);
}

void ChatController::onErrorChunk(const QString& chunk) {
    emit consoleOutput("[stderr] " + chunk);
}

void ChatController::onRunnerFinished(int exitCode) {
    Q_UNUSED(exitCode)
    buildAssistantMessage();
    emit generationStopped();
}

void ChatController::buildAssistantMessage() {
    if (m_currentResponse.trimmed().isEmpty()) return;

    auto* session = m_sessions->currentSession();
    if (!session) return;

    Message assistantMsg;
    assistantMsg.id = QUuid::createUuid();
    assistantMsg.role = Message::Role::Assistant;
    assistantMsg.contentType = Message::ContentType::Text;
    assistantMsg.text = m_currentResponse;
    assistantMsg.timestamp = QDateTime::currentDateTimeUtc();
    assistantMsg.tokenCount = TokenCounter::estimate(m_currentResponse);

    // Post-receive scripting hooks
    if (m_scripts) m_scripts->runHook("post_receive", {{"text", m_currentResponse}});

    session->appendMessage(assistantMsg);
    session->updateTokens(0, assistantMsg.tokenCount);
    session->save();

    emit responseComplete(assistantMsg);
    m_currentResponse.clear();
}

}  // namespace CodeHex
