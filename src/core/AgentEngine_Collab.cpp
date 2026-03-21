/**
 * @file AgentEngine_Collab.cpp
 * @brief Multi-Agent Collaborative Mode (Roadmap Item #5).
 *
 * Contains both the legacy synchronous consultCollaborator() and
 * the new async consultCollaboratorAsync() (P-3).
 */
#include "AgentEngine.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include "ToolExecutor.h"
#include "../cli/CliRunner.h"
#include "../cli/ConfigurableProfile.h"
#include "AppConfig.h"
#include "PromptManager.h"

namespace CodeHex {

// ---------------------------------------------------------------------------
// Helper: resolve role string → AgentRole + build system prompt
// ---------------------------------------------------------------------------
static AgentRole resolveCollabRole(const QString& role) {
    if (role.contains("Architect",  Qt::CaseInsensitive)) return AgentRole::Architect;
    if (role.contains("Debugger",   Qt::CaseInsensitive)) return AgentRole::Debugger;
    if (role.contains("Security",   Qt::CaseInsensitive) ||
        role.contains("Auditor",    Qt::CaseInsensitive)) return AgentRole::SecurityAuditor;
    if (role.contains("Reviewer",   Qt::CaseInsensitive)) return AgentRole::Reviewer;
    if (role.contains("Refactor",   Qt::CaseInsensitive)) return AgentRole::Refactor;
    return AgentRole::Base;
}

static QString buildCollabSystemPrompt(PromptManager* prompts, const QString& role) {
    AgentRole targetRole = resolveCollabRole(role);
    if (targetRole != AgentRole::Base) {
        return prompts->loadRolePrompt(targetRole);
    }
    return QString(
        "You are a specialized AI %1. Your task is to provide a concise, expert-level "
        "analysis of the problem presented to you. Be direct, critical, and constructive. "
        "Do NOT use tool calls. Respond in plain text with your analysis."
    ).arg(role);
}

// ---------------------------------------------------------------------------
// Legacy synchronous version (kept for backward compatibility)
// ---------------------------------------------------------------------------
QString AgentEngine::consultCollaborator(const QString& prompt, const QString& role)
{
    QString collaboratorSystemPrompt = buildCollabSystemPrompt(m_prompts, role);

    LlmProvider provider = m_config->activeProvider();
    auto profile = ConfigurableProfile::fromProvider(provider);
    if (!m_selectedModel.isEmpty()) {
        profile->setModel(m_selectedModel);
    }
    m_collaboratorRunner->setProfile(std::move(profile));

    m_collaboratorResponse.clear();
    bool finished = false;
    QString errorMsg;

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(60000);

    auto chunkConn = connect(m_collaboratorRunner, &CliRunner::outputChunk,
        [this](const QString& chunk) { m_collaboratorResponse += chunk; });
    auto finishedConn = connect(m_collaboratorRunner, &CliRunner::finished,
        [&loop, &finished](int) { finished = true; loop.quit(); });
    auto errorConn = connect(m_collaboratorRunner, &CliRunner::errorChunk,
        [&errorMsg](const QString& err) { errorMsg += err; });
    auto timeoutConn = connect(&timeoutTimer, &QTimer::timeout,
        [&loop, &finished]() { finished = true; loop.quit(); });

    m_collaboratorRunner->send(prompt, m_config->workingFolder(), {}, {}, collaboratorSystemPrompt);
    timeoutTimer.start();
    loop.exec();

    disconnect(chunkConn);
    disconnect(finishedConn);
    disconnect(errorConn);
    disconnect(timeoutConn);
    timeoutTimer.stop();

    if (!timeoutTimer.isActive() && m_collaboratorResponse.isEmpty()) {
        qWarning() << "[AgentEngine] consultCollaborator: timed out.";
        return "[Collaborator timed out after 60 seconds. No response received.]";
    }
    if (m_collaboratorResponse.isEmpty() && !errorMsg.isEmpty()) {
        qWarning() << "[AgentEngine] consultCollaborator error:" << errorMsg;
        return QString("[Collaborator error: %1]").arg(errorMsg);
    }

    qInfo() << "[AgentEngine] consultCollaborator: got" << m_collaboratorResponse.size() << "chars.";
    return m_collaboratorResponse.trimmed();
}

// ---------------------------------------------------------------------------
// P-3: Async (non-blocking) version
// ---------------------------------------------------------------------------
void AgentEngine::consultCollaboratorAsync(const ToolCall& call,
                                           const QString& prompt,
                                           const QString& role)
{
    QString collaboratorSystemPrompt = buildCollabSystemPrompt(m_prompts, role);

    LlmProvider provider = m_config->activeProvider();
    auto profile = ConfigurableProfile::fromProvider(provider);
    if (!m_selectedModel.isEmpty()) profile->setModel(m_selectedModel);
    m_collaboratorRunner->setProfile(std::move(profile));

    m_collaboratorResponse.clear();
    m_pendingCollabCall = call;

    // Disconnect any previous connections from the collaborator runner
    disconnect(m_collaboratorRunner, &CliRunner::outputChunk, this, nullptr);
    disconnect(m_collaboratorRunner, &CliRunner::finished, this, nullptr);
    disconnect(m_collaboratorRunner, &CliRunner::errorChunk, this, nullptr);

    // Collect chunks
    connect(m_collaboratorRunner, &CliRunner::outputChunk, this,
        [this](const QString& chunk) { m_collaboratorResponse += chunk; });

    // On finish: build result and route it through the normal tool pipeline
    connect(m_collaboratorRunner, &CliRunner::finished, this,
        [this, role](int /*exitCode*/) {
            if (m_collabTimeoutTimer) m_collabTimeoutTimer->stop();

            ToolResult result;
            result.toolUseId = m_pendingCollabCall.id;
            result.subAgentRole = role;

            if (m_collaboratorResponse.trimmed().isEmpty()) {
                result.isError = true;
                result.content = QString("[%1: No response received]").arg(role);
            } else {
                result.isError = false;
                result.content = QString("[%1 Response]\n%2").arg(role, m_collaboratorResponse.trimmed());
            }

            qInfo() << "[AgentEngine] Async collaborator finished:" << m_collaboratorResponse.size() << "chars";
            emit terminalOutput(QString("[%1] ← DONE  %2  (%3 chars)")
                .arg(QDateTime::currentDateTime().toString("HH:mm:ss"),
                     role, QString::number(m_collaboratorResponse.size())));

            // Route through normal tool result pipeline
            emit m_toolExecutor->toolFinished(m_pendingCollabCall.name, result);

            // Cleanup connections
            disconnect(m_collaboratorRunner, &CliRunner::outputChunk, this, nullptr);
            disconnect(m_collaboratorRunner, &CliRunner::finished, this, nullptr);
            disconnect(m_collaboratorRunner, &CliRunner::errorChunk, this, nullptr);
        });

    // Error collection
    connect(m_collaboratorRunner, &CliRunner::errorChunk, this,
        [this](const QString& err) { m_collaboratorResponse += "\n[stderr] " + err; });

    // Timeout (60s)
    if (!m_collabTimeoutTimer) {
        m_collabTimeoutTimer = new QTimer(this);
        m_collabTimeoutTimer->setSingleShot(true);
    }
    disconnect(m_collabTimeoutTimer, &QTimer::timeout, nullptr, nullptr);
    connect(m_collabTimeoutTimer, &QTimer::timeout, this, [this, role]() {
        qWarning() << "[AgentEngine] Async collaborator timed out.";
        m_collaboratorRunner->stop();

        ToolResult result;
        result.toolUseId = m_pendingCollabCall.id;
        result.isError = true;
        result.subAgentRole = role;
        result.content = QString("[%1 timed out after 60 seconds]").arg(role);

        emit m_toolExecutor->toolFinished(m_pendingCollabCall.name, result);
    });

    emit terminalOutput(QString("[%1] → ASK   %2  (async)")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), role));

    m_collaboratorRunner->send(prompt, m_config->workingFolder(), {}, {}, collaboratorSystemPrompt);
    m_collabTimeoutTimer->start(60000);
}

} // namespace CodeHex
