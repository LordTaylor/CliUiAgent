/**
 * @file AgentEngine_Runner.cpp
 * @brief CliRunner callbacks: onOutputChunk, onRawOutput, onErrorChunk,
 *        onRunnerFinished, buildAssistantMessage.
 */
#include "AgentEngine.h"
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QRegularExpression>
#include "ResponseFilter.h"
#include "ResponseParser.h"
#include "SessionManager.h"
#include "../data/Session.h"

namespace CodeHex {

void AgentEngine::onOutputChunk(const QString& chunk) {
    // Reset LLM timeout on every token — runner is still alive (#2)
    if (m_llmTimeoutTimer->isActive()) {
        m_llmTimeoutTimer->start(adaptiveLlmTimeout());
    }
    m_lastRawResponse += chunk;
    QString filtered = m_filter->processChunk(chunk);
    if (!filtered.isEmpty()) emit tokenReceived(filtered);
}

void AgentEngine::onRawOutput(const QString& raw) {
    emit consoleOutput(raw);
}

void AgentEngine::onErrorChunk(const QString& chunk) {
    emit consoleOutput("[stderr] " + chunk);
}

void AgentEngine::onRunnerFinished(int exitCode) {
    Q_UNUSED(exitCode);
    m_llmTimeoutTimer->stop();
    m_isRunning = false;

    // Record response time for adaptive timeout (P-10)
    if (m_llmRequestTimer.isValid()) {
        int elapsed = static_cast<int>(m_llmRequestTimer.elapsed());
        m_llmResponseTimesMs.append(elapsed);
        if (m_llmResponseTimesMs.size() > MAX_RESPONSE_HISTORY)
            m_llmResponseTimesMs.removeFirst();
        m_llmRequestTimer.invalidate();
    }

    // Exact-output loop detection
    auto* session = m_sessions->currentSession();
    QString lastAssistantContent;
    if (session && !session->messages.isEmpty()) {
        for (int i = session->messages.size() - 1; i >= 0; --i) {
            if (session->messages[i].role == Message::Role::Assistant) {
                lastAssistantContent = session->messages[i].textFromContentBlocks().trimmed();
                break;
            }
        }
    }

    QString currentResp = m_filter->currentResponse();
    if (!lastAssistantContent.isEmpty() && currentResp.trimmed() == lastAssistantContent) {
        qWarning() << "[AgentEngine] LOOP DETECTED. Assistant repeated itself exactly.";
        emit statusChanged("Loop detected. Nudging agent...");
        sendContinueRequest(
            "WARNING: You just sent the EXACT SAME response. DO NOT repeat your previous thought "
            "or tool call. If you are stuck because the tool output is not what you expected, try "
            "a DIFFERENT approach or a DIFFERENT tool. If the task is finished, simply state 'TASK COMPLETE'.");
        return;
    }

    // Chain-of-Verification (CoVe) state machine
    if (m_coveState == CoVeState::None && !currentResp.contains("<tool_call>")) {
        qDebug() << "[AgentEngine] CoVe: Entering Drafting -> VerifyingQuestions";
        emit statusChanged("🧐 Verifying facts (CoVe)...");
        m_coveState = CoVeState::VerifyingQuestions;
        sendContinueRequest(
            "### CHAIN-OF-VERIFICATION (CoVe) - STEP 2: GENERATE QUESTIONS ###\n"
            "Based on your previous response, generate a list of verification questions to cross-check "
            "any facts or claims you made. Focus on numbers, dates, technical details, and logical "
            "assumptions.\n\n### YOUR RESPONSE:\n" + currentResp);
        return;
    } else if (m_coveState == CoVeState::VerifyingQuestions) {
        qDebug() << "[AgentEngine] CoVe: VerifyingQuestions -> Answering";
        emit statusChanged("🔍 Answering verification questions...");
        m_coveState = CoVeState::Answering;
        sendContinueRequest(
            "### CHAIN-OF-VERIFICATION (CoVe) - STEP 3: ANSWER QUESTIONS ###\n"
            "Answer the verification questions you just generated. Be objective and factual.\n");
        return;
    } else if (m_coveState == CoVeState::Answering) {
        qDebug() << "[AgentEngine] CoVe: Answering -> Finalizing";
        emit statusChanged("✨ Finalizing response...");
        m_coveState = CoVeState::Finalizing;
        sendContinueRequest(
            "### CHAIN-OF-VERIFICATION (CoVe) - STEP 4: FINAL RESPONSE ###\n"
            "Incorporate the findings from your verification steps. If you found errors, correct them. "
            "Provide the final, verified response to the user. DO NOT include the internal CoVe steps "
            "in the final output.");
        return;
    }

    m_coveState = CoVeState::None;
    m_isRunning = false;

    ResponseParser::ParseResult parseResult = ResponseParser::parse(currentResp);

    // Confidence Anchor
    if (parseResult.confidenceScore < 5 && parseResult.toolCalls.isEmpty()) {
        qDebug() << "[AgentEngine] Low confidence detected:" << parseResult.confidenceScore;
        emit statusChanged("⚠️ Low confidence. Nudging for research...");
        sendContinueRequest(QString("Your confidence score is low (%1/10). "
                                    "Please perform additional research using Grep, ReadFile, or "
                                    "SearchRepo before making changes.")
                            .arg(parseResult.confidenceScore));
        return;
    }

    m_lastRawResponse = currentResp;
    buildAssistantMessage(parseResult, currentResp);

    qDebug() << "[AgentEngine] onRunnerFinished: parsedCalls.size()=" << parseResult.toolCalls.size();
    if (parseResult.toolCalls.size() > 1) {
        qDebug() << "[AgentEngine] Dispatching tool batch:" << parseResult.toolCalls.size() << "calls";
        dispatchToolBatch(parseResult.toolCalls);
    } else if (parseResult.toolCalls.size() == 1) {
        qDebug() << "[AgentEngine] Dispatching tool:" << parseResult.toolCalls.first().name;
        onToolCallReady(parseResult.toolCalls.first());
    } else {
        qDebug() << "[AgentEngine] No tool calls found in response";
        emit statusChanged("");
        if (m_requestQueue.isEmpty()) cleanupScratchpad();
        processNextQueueItem();
    }
}

void AgentEngine::buildAssistantMessage(const ResponseParser::ParseResult& result, const QString& rawText) {
    auto* session = m_sessions->currentSession();
    if (!session) return;

    Message msg;
    msg.id = QUuid::createUuid();
    msg.role = Message::Role::Assistant;
    msg.timestamp = QDateTime::currentDateTime();
    msg.rawContent = rawText;
    msg.confidenceScore = result.confidenceScore; // Surface to UI (#14)

    for (const auto& thought : result.thoughts) {
        CodeBlock b;
        b.type = BlockType::Thinking;
        b.content = thought.content;
        b.isCollapsed = thought.isCollapsed;
        msg.contentBlocks << b;
        msg.contentTypes << Message::ContentType::Thinking;
    }

    if (!result.cleanText.isEmpty()) msg.addText(result.cleanText);

    // Auto-rename session on first message
    if (session->messages.size() <= 2) {
        QString firstText = msg.textFromContentBlocks();
        QString title = firstText.section(QRegularExpression("[.!?]"), 0, 0).trimmed();
        title.remove(QRegularExpression("[^\\w\\s-]"));
        if (title.length() > 40) title = title.left(37) + "...";
        if (title.trimmed().isEmpty()) title = "New Task";
        session->title = title.trimmed();
    }

    session->appendMessage(msg);
    emit responseComplete(msg);
    emit statusChanged("");
}

} // namespace CodeHex
