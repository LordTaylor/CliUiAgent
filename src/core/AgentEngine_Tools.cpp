/**
 * @file AgentEngine_Tools.cpp
 * @brief Tool execution pipeline: onToolCallReady, approveToolCall, onToolResultReceived.
 */
#include "AgentEngine.h"
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QTimer>
#include <QRegularExpression>
#include <algorithm>

// Helper: formatted terminal log line  [HH:MM:SS] → TOOL      detail
static QString termLine(const QString& arrow, const QString& tool, const QString& detail) {
    const QString ts  = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString pad = tool.leftJustified(12);
    return QString("[%1] %2 %3  %4").arg(ts, arrow, pad, detail);
}
#include "ToolExecutor.h"
#include "SessionManager.h"
#include "AppConfig.h"
#include "../cli/CliRunner.h"
#include "../data/Session.h"

namespace CodeHex {

void AgentEngine::onToolCallReady(const CodeHex::ToolCall& call) {
    qDebug() << "[AgentEngine] onToolCallReady: tool=" << call.name;

    // JSON Validation (#18): skip execution for calls with malformed input
    if (!call.valid) {
        qWarning() << "[AgentEngine] Skipping tool" << call.name << "— JSON input could not be parsed.";
        ToolResult errResult;
        errResult.toolUseId = call.id;
        errResult.isError   = true;
        errResult.content   = QString("Error: The parameters for tool '%1' could not be parsed "
                                      "(malformed JSON). Please rewrite the tool call with valid JSON input.")
                              .arg(call.name);
        onToolResultReceived(call.name, errResult);
        return;
    }

    auto* session = m_sessions->currentSession();
    if (!session) return;

    // Sandbox check
    if (call.input.contains("path")) {
        QString path = call.input.value("path").toString();
        if (!isPathAllowed(path)) {
            qWarning() << "[AgentEngine] Sandbox violation for path:" << path;
            emit errorOccurred("Sandbox violation: Path is outside of working directory: " + path);
            return;
        }
    }

    Permission p = toolPermission(call.name);
    qDebug() << "[AgentEngine] Permission for" << call.name << "=" << (int)p;
    if (p == Permission::Deny) {
        emit errorOccurred("Tool execution denied: " + call.name);
        return;
    }
    if (p == Permission::Ask) {
        emit toolApprovalRequested(call);
        m_pendingCalls.append(call);
        m_isRunning = false;
        return;
    }

    // Save call for retry on transient failure (#7)
    m_lastExecutedCall = call;

    // Log CALL step to session
    Message callMsg;
    callMsg.id = QUuid::createUuid();
    callMsg.role = Message::Role::Assistant;
    CodeBlock callBlock;
    callBlock.type = BlockType::LogStep;
    callBlock.content = "⚙ CALL: " + call.name;
    callMsg.contentBlocks << callBlock;
    callMsg.isInternal = true;
    callMsg.timestamp = QDateTime::currentDateTime();
    session->appendMessage(callMsg);
    session->save();

    // Terminal log: show what tool is being called and key params
    {
        QString detail;
        if (call.input.contains("path"))    detail = call.input.value("path").toString();
        else if (call.input.contains("command")) detail = call.input.value("command").toString().left(60);
        else if (call.input.contains("query"))   detail = call.input.value("query").toString().left(60);
        else if (call.input.contains("content")) detail = QString("(%1 chars)").arg(call.input.value("content").toString().size());
        emit terminalOutput(termLine("→ CALL", call.name, detail));
    }

    emit toolCallStarted(call.name, call.input);
    m_isRunning = true;
    if (m_syncTools) {
        m_toolExecutor->executeSync(call, m_config->workingFolder());
    } else {
        m_toolExecutor->execute(call, m_config->workingFolder());
    }
}

void AgentEngine::approveToolCall(const ToolCall& call) {
    auto* session = m_sessions->currentSession();
    if (session) {
        Message callMsg;
        callMsg.id = QUuid::createUuid();
        callMsg.role = Message::Role::Assistant;
        CodeBlock callBlock;
        callBlock.type = BlockType::LogStep;
        callBlock.content = "⚙ CALL: " + call.name;
        callMsg.contentBlocks << callBlock;
        callMsg.isInternal = true;
        callMsg.timestamp = QDateTime::currentDateTime();
        session->appendMessage(callMsg);
        session->save();
    }
    emit statusChanged(QString("Approved: Executing %1...").arg(call.name));
    emit toolCallStarted(call.name, call.input);
    m_toolExecutor->execute(call, m_config->workingFolder());
}

void AgentEngine::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result) {
    Q_UNUSED(toolName);

    // P-3: Skip pending results — the real result will arrive later via async callback
    if (result.isPending) {
        emit terminalOutput(QString("[%1] ⏳ PEND  %2  (awaiting async response)")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), toolName));
        return;
    }

    auto* session = m_sessions->currentSession();
    if (!session) return;

    // Append tool result message
    Message toolMsg;
    toolMsg.id = QUuid::createUuid();
    toolMsg.role = Message::Role::User;
    toolMsg.timestamp = QDateTime::currentDateTime();
    toolMsg.toolResults << result;
    toolMsg.isInternal = true;
    if (!result.subAgentRole.isEmpty()) {
        toolMsg.subAgentRole = result.subAgentRole;
    }
    CodeBlock block;
    block.type = BlockType::LogStep;
    block.content = (result.isError ? "❌ FAILED: " : "✅ DONE: ") + toolName;
    toolMsg.contentBlocks << block;
    toolMsg.addText(result.content);
    session->appendMessage(toolMsg);
    session->save();

    // Terminal log: show tool result
    {
        QString preview = result.content.trimmed().left(120).replace('\n', ' ');
        if (result.content.trimmed().size() > 120) preview += "…";
        if (result.isError) {
            emit terminalError(termLine("✗ FAIL", toolName, preview));
        } else {
            const int lines = result.content.count('\n') + 1;
            QString detail  = lines > 1
                ? QString("(%1 lines)  %2").arg(lines).arg(preview)
                : preview;
            emit terminalOutput(termLine("← DONE", toolName, detail));
        }
    }

    // --- Loop Detection (#3 semantic + output) ---
    m_lastToolResults.append(result.content.trimmed());
    if (m_lastToolResults.size() > MAX_LOOP_RESULTS) m_lastToolResults.removeFirst();

    {
        QString keyParam = m_lastExecutedCall.input.value("path").toString();
        if (keyParam.isEmpty()) keyParam = m_lastExecutedCall.input.value("command").toString().left(80);
        if (keyParam.isEmpty()) keyParam = m_lastExecutedCall.input.value("query").toString().left(80);
        QString semanticFp = toolName + ":" + keyParam.trimmed().toLower();
        m_lastToolCallFingerprints.append(semanticFp);
        if (m_lastToolCallFingerprints.size() > MAX_LOOP_RESULTS)
            m_lastToolCallFingerprints.removeFirst();
    }

    bool potentialLoop = false;
    if (m_lastToolResults.size() == MAX_LOOP_RESULTS) {
        auto getFingerprint = [](QString s) {
            s = s.trimmed().toLower();
            s.remove(QRegularExpression("\\s+"));
            if (s.length() > 500) s = s.left(500);
            return s;
        };
        QString firstFp = getFingerprint(m_lastToolResults[0]);
        potentialLoop = std::all_of(m_lastToolResults.begin() + 1, m_lastToolResults.end(),
                                    [&](const QString& s){ return getFingerprint(s) == firstFp; });

        if (!potentialLoop && m_lastToolCallFingerprints.size() == MAX_LOOP_RESULTS) {
            const QString& firstCallFp = m_lastToolCallFingerprints.first();
            potentialLoop = std::all_of(m_lastToolCallFingerprints.begin() + 1,
                                        m_lastToolCallFingerprints.end(),
                                        [&](const QString& s){ return s == firstCallFp; });
        }
    }

    if (!m_runner->isProfileRunning()) {
        // ISV: auto-validate file edits
        bool isEditTool = (toolName == "WriteFile" || toolName == "Replace" || toolName == "SearchReplace");
        if (isEditTool && !result.isError) {
            emit statusChanged("🔍 ISV: Automatic Validation...");
            sendContinueRequest(QString(
                "Tool %1 executed successfully. "
                "As part of Integrated Synthesis & Validation (ISV), you MUST now verify this change. "
                "Use Build, SyntaxCheck, or ReadFile to confirm the file is correct and consistent.")
                .arg(toolName));
            return;
        }

        if (!result.isError) {
            QString nudge = QString("Tool Executed: %1\nOutput: %2\n\n"
                                    "Please ANALYZE this output and decide on the NEXT STEP "
                                    "or FINALIZE the task if no more actions are needed.")
                            .arg(toolName, result.content);
            if (potentialLoop) {
                nudge += "\n⚠️ WARNING: I detected that you are repeating the same output multiple times. "
                         "Please check if you are stuck in a logic loop and try a DIFFERENT approach.";
                emit potentialLoopDetected("Agent seems to be repeating actions.");
            }
            sendContinueRequest(nudge);
        } else {
            // Retry with backoff (#7)
            static const int MAX_RETRIES = 2;
            if (m_lastExecutedCall.name == toolName && m_lastExecutedCall.retryCount < MAX_RETRIES) {
                m_lastExecutedCall.retryCount++;
                int delayMs = m_lastExecutedCall.retryCount * 500;
                qWarning() << "[AgentEngine] Tool" << toolName << "failed. Retry"
                           << m_lastExecutedCall.retryCount << "/" << MAX_RETRIES
                           << "in" << delayMs << "ms";
                emit statusChanged(QString("⏳ Retry %1/%2: %3...")
                                   .arg(m_lastExecutedCall.retryCount).arg(MAX_RETRIES).arg(toolName));
                ToolCall retryCall = m_lastExecutedCall;
                QTimer::singleShot(delayMs, this, [this, retryCall]() {
                    if (m_syncTools) m_toolExecutor->executeSync(retryCall, m_config->workingFolder());
                    else             m_toolExecutor->execute(retryCall, m_config->workingFolder());
                });
            } else {
                sendContinueRequest(QString("Tool '%1' failed after %2 attempt(s): %3\n"
                                            "Analyze the error and try a different approach.")
                                    .arg(toolName).arg(m_lastExecutedCall.retryCount + 1)
                                    .arg(result.content));
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Multi-Tool Batch Execution (P-1)
// ──────────────────────────────────────────────────────────────────────

void AgentEngine::dispatchToolBatch(const QList<ToolCall>& calls) {
    auto* session = m_sessions->currentSession();
    if (!session) return;

    // Pre-validate ALL calls: sandbox + permissions
    QList<ToolCall> allowed;
    for (const auto& call : calls) {
        if (!call.valid) {
            emit terminalError(termLine("✗ SKIP", call.name, "malformed JSON"));
            continue;
        }
        if (call.input.contains("path") && !isPathAllowed(call.input.value("path").toString())) {
            emit terminalError(termLine("✗ SKIP", call.name, "sandbox violation"));
            continue;
        }
        Permission p = toolPermission(call.name);
        if (p == Permission::Deny) continue;
        if (p == Permission::Ask) {
            // Fallback to sequential: can't batch if any call needs approval
            qDebug() << "[AgentEngine] Batch fallback: tool" << call.name << "needs approval";
            onToolCallReady(calls.first());
            return;
        }
        allowed.append(call);
    }

    if (allowed.isEmpty()) {
        sendContinueRequest("All tool calls were rejected (invalid JSON, sandbox, or denied). Try a different approach.");
        return;
    }

    // Log batch start
    emit terminalOutput(QString("[%1] ⚡ BATCH  %2 tools in parallel")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(allowed.size()));

    // Session log
    Message batchMsg;
    batchMsg.id = QUuid::createUuid();
    batchMsg.role = Message::Role::Assistant;
    CodeBlock batchBlock;
    batchBlock.type = BlockType::LogStep;
    batchBlock.content = QString("⚡ BATCH: %1 tools").arg(allowed.size());
    batchMsg.contentBlocks << batchBlock;
    batchMsg.isInternal = true;
    batchMsg.timestamp = QDateTime::currentDateTime();
    session->appendMessage(batchMsg);

    // Setup batch state
    {
        QMutexLocker lock(&m_batchMutex);
        m_batchCalls = allowed;
        m_batchResults.clear();
    }
    m_batchPending.store(allowed.size());
    m_isRunning = true;

    // Dispatch all tools in parallel
    for (const auto& call : allowed) {
        // Terminal log per call
        QString detail;
        if (call.input.contains("path"))    detail = call.input.value("path").toString();
        else if (call.input.contains("command")) detail = call.input.value("command").toString().left(60);
        else if (call.input.contains("query"))   detail = call.input.value("query").toString().left(60);
        emit terminalOutput(termLine("→ CALL", call.name, detail));

        emit toolCallStarted(call.name, call.input);
        m_toolExecutor->execute(call, m_config->workingFolder());
    }
}

void AgentEngine::onBatchToolFinished(const QString& toolName, const ToolResult& result) {
    auto* session = m_sessions->currentSession();

    // Terminal log
    {
        QString preview = result.content.trimmed().left(80).replace('\n', ' ');
        if (result.isError)
            emit terminalError(termLine("✗ FAIL", toolName, preview));
        else
            emit terminalOutput(termLine("← DONE", toolName, preview));
    }

    // Session log
    if (session) {
        Message toolMsg;
        toolMsg.id = QUuid::createUuid();
        toolMsg.role = Message::Role::User;
        toolMsg.timestamp = QDateTime::currentDateTime();
        toolMsg.toolResults << result;
        toolMsg.isInternal = true;
        if (!result.subAgentRole.isEmpty())
            toolMsg.subAgentRole = result.subAgentRole;
        CodeBlock block;
        block.type = BlockType::LogStep;
        block.content = (result.isError ? "❌ FAILED: " : "✅ DONE: ") + toolName;
        toolMsg.contentBlocks << block;
        toolMsg.addText(result.content);
        session->appendMessage(toolMsg);
    }

    // Collect result
    {
        QMutexLocker lock(&m_batchMutex);
        m_batchResults.append(result);
    }

    int remaining = m_batchPending.fetch_sub(1) - 1;
    qDebug() << "[AgentEngine] Batch tool finished:" << toolName << "remaining:" << remaining;

    if (remaining <= 0) {
        // All batch tools done — send combined nudge to LLM
        if (session) session->save();

        QString combined;
        QMutexLocker lock(&m_batchMutex);
        for (int i = 0; i < m_batchCalls.size() && i < m_batchResults.size(); ++i) {
            const auto& call = m_batchCalls[i];
            const auto& res  = m_batchResults[i];
            combined += QString("### Tool: %1\n%2: %3\n\n")
                .arg(call.name,
                     res.isError ? "ERROR" : "Output",
                     res.content.left(2000));
        }
        lock.unlock();

        emit terminalOutput(QString("[%1] ⚡ BATCH  complete — %2 results")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
            .arg(m_batchResults.size()));

        sendContinueRequest(
            QString("All %1 tools executed in parallel. Results:\n\n%2\n"
                    "Analyze the outputs and decide on the NEXT STEP or FINALIZE.")
            .arg(m_batchCalls.size()).arg(combined));
    }
}

} // namespace CodeHex
