/**
 * @file AgentEngine_Loop.cpp
 * @brief Agent main loop: process(), runLoop(), sendContinueRequest(), injectAutoContext()
 */
#include "AgentEngine.h"
#include "AgentGraph.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QDateTime>
#include <QtConcurrent>
#include "PromptManager.h"
#include "TokenCounter.h"
#include "ModelRouter.h"
#include "SessionManager.h"
#include "AppConfig.h"
#include "ToolExecutor.h"
#include "ContextManager.h"
#include "rag/CodebaseIndexer.h"
#include "../cli/CliRunner.h"
#include "../cli/ConfigurableProfile.h"
#include "../data/Session.h"

namespace CodeHex {

void AgentEngine::runLoop(const QString& prompt, const QStringList& imagePaths) {
    Q_UNUSED(imagePaths);
    m_isRunning = true;

    // Route profile based on current role
    QString profileId = m_router->getProfileIdForRole(m_currentRole);
    LlmProvider provider = m_config->activeProvider();
    for (const auto& p : m_config->providers()) {
        if (p.id == profileId) { provider = p; break; }
    }

    qInfo() << "AgentEngine: Using profile" << provider.name << "for role" << (int)m_currentRole;
    auto profile = ConfigurableProfile::fromProvider(provider);
    if (!m_selectedModel.isEmpty() && m_currentRole == AgentRole::Base) {
        profile->setModel(m_selectedModel);
    }
    m_runner->setProfile(std::move(profile));

    emit statusChanged(QString("Thinking (%1)...").arg(prompt.left(20)));
    auto session = m_sessions->currentSession();
    if (!session) { m_isRunning = false; return; }

    bool useJsonSchema = m_runner->profile() &&
                         m_runner->profile()->name().contains("claude", Qt::CaseInsensitive);

    QString ragContext;
    if (m_currentRole == AgentRole::RAG) {
        emit statusChanged("🔍 Searching Codebase...");
        auto results = m_indexer->search(prompt, 5);
        if (!results.isEmpty()) {
            ragContext = "\n### RELEVANT CODE CONTEXT (Local RAG):\n";
            for (const auto& chunk : results) {
                ragContext += QString("File: %1 (Lines %2-%3)\n```\n%4\n```\n\n")
                    .arg(chunk.filePath).arg(chunk.startLine)
                    .arg(chunk.endLine).arg(chunk.content);
            }
        }
    }

    QString systemPrompt = m_prompts->buildSystemPrompt(
        m_currentRole, ragContext, m_activeTechniques, m_blackboard);

    if (useJsonSchema) {
        emit statusChanged("🧠 Reasoning (JSON Schema)...");
        QJsonArray tools = m_toolExecutor->getToolDefinitionsJson();
        ContextManager::ContextStats stats;
        QJsonObject request = m_prompts->buildRequestJson(
            m_currentRole, prompt, session->messages, tools,
            m_activeTechniques, m_blackboard, 16000, true, ragContext, &stats);
        emit contextStatsUpdated(stats);
        m_lastRawRequest = QJsonDocument(request).toJson();
        m_lastRawResponse.clear();
        m_llmRequestTimer.start(); m_llmTimeoutTimer->start(adaptiveLlmTimeout());
        m_runner->sendJson(request, m_config->workingFolder());
    } else {
        emit statusChanged("🧠 Thinking...");
        QString implicitGoals = m_prompts->detectImplicitGoals(prompt);
        QString enrichedPrompt = implicitGoals + "\n\n### USER REQUEST:\n" + prompt;

        const int inputTokens = TokenCounter::estimate(enrichedPrompt);
        session->updateTokens(inputTokens, 0);

        // Emit context stats for non-JSON providers too
        {
            const auto provider = m_config->activeProvider();
            ContextManager::PruningOptions pruneOpts;
            pruneOpts.maxTokens = provider.contextWindow;
            if (pruneOpts.maxTokens <= 0) pruneOpts.maxTokens = 32768; // Fallback
            ContextManager::ContextStats stats;
            ContextManager::prune(session->messages, pruneOpts, &stats);
            emit contextStatsUpdated(stats);
        }

        qInfo() << "[AgentEngine] Sending request to LLM (Model:" << m_selectedModel << ")";
        qInfo() << "[AgentEngine] Prompt snippet:" << enrichedPrompt.left(300).replace("\n", " ") << "...";
        
        m_lastRawRequest = "--- System Prompt ---\n" + systemPrompt +
                           "\n\n--- Prompt ---\n" + enrichedPrompt;
        m_lastRawResponse.clear();
        m_llmRequestTimer.start(); m_llmTimeoutTimer->start(adaptiveLlmTimeout());
        m_runner->send(enrichedPrompt, m_config->workingFolder(), {}, session->messages, systemPrompt);
    }
}

void AgentEngine::process(const QString& userInput, const QList<Attachment>& attachments) {
    auto session = m_sessions->currentSession();
    if (!session) return;

    Message userMsg;
    userMsg.id = QUuid::createUuid();
    userMsg.role = Message::Role::User;
    userMsg.addText(userInput);
    userMsg.timestamp = QDateTime::currentDateTime();

    QStringList imagePaths;
    for (const auto& att : attachments) {
        userMsg.addAttachment(att);
        if (att.type == Attachment::Type::Image) imagePaths << att.filePath;
    }

    session->appendMessage(userMsg);
    session->save();

    if (m_isRunning) {
        qDebug() << "[AgentEngine] Already running. Enqueuing request.";
        m_requestQueue.enqueue({userInput, attachments});
        emit statusChanged(QString("Request queued (%1 in queue)").arg(m_requestQueue.size()));
        return;
    }

    // P-6: Rolling Summary Compression (replaces old LLM-based compaction)
    if (session->messages.size() > 20) {
        qDebug() << "[AgentEngine] Session too long. Applying rolling summary...";
        emit statusChanged("📦 Compressing conversation history...");
        session->messages = ContextManager::rollingSummarize(session->messages, 15);
        session->save();
        emit terminalOutput(QString("[%1] 📦 COMPRESS  %2 messages → rolling summary + 15 recent")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
            .arg(session->messages.size()));
    }

    m_requestQueue.clear();
    resetStreamState();
    m_isRunning = true;
    m_loopIterations = 0;
    // m_lastToolCallFingerprints.clear(); // P-13: Persist across turns to detect inter-turn loops

    // Role Auto-Detect (P-8): if user hasn't manually set a role, detect from prompt
    if (m_currentRole == AgentRole::Base) {
        AgentRole detected = m_router->detectRoleFromPrompt(userInput);
        if (detected != AgentRole::Base) {
            m_currentRole = detected;
            static const QMap<AgentRole, QString> roleNames = {
                {AgentRole::Explorer, "Explorer"}, {AgentRole::Executor, "Executor"},
                {AgentRole::Reviewer, "Reviewer"}, {AgentRole::Debugger, "Debugger"},
                {AgentRole::Refactor, "Refactor"}, {AgentRole::Architect, "Architect"},
                {AgentRole::SecurityAuditor, "Security"}, {AgentRole::RAG, "RAG"},
            };
            emit terminalOutput(QString("[%1] 🎭 Auto-detected role: %2")
                .arg(QDateTime::currentDateTime().toString("HH:mm:ss"),
                     roleNames.value(detected, "Base")));
        }
    }

    (void)QtConcurrent::run(&CodebaseIndexer::indexDirectory, m_indexer, m_config->workingFolder());

    // P-4: Use AgentGraph for complex tasks (heuristics)
    bool isComplex = userInput.length() > 50 || 
                     userInput.contains("implement", Qt::CaseInsensitive) ||
                     userInput.contains("create", Qt::CaseInsensitive) ||
                     userInput.contains("napisz", Qt::CaseInsensitive) ||
                     userInput.contains("dodaj", Qt::CaseInsensitive);

    if (isComplex && m_graph) {
        emit terminalOutput(QString("[%1] 🕸️ Transitioning to Graph Orchestration...")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
        m_graph->begin(userInput);
        m_graph->step();
    } else {
        runLoop(userInput, imagePaths);
    }
}

void AgentEngine::sendContinueRequest(const QString& nudge) {
    auto session = m_sessions->currentSession();
    if (!session) return;

    // Circuit Breaker (#1)
    ++m_loopIterations;
    qInfo() << "[AgentEngine] SendContinueRequest - Iteration:" << m_loopIterations << "/" << MAX_LOOP_ITERATIONS;
    if (m_loopIterations >= MAX_LOOP_ITERATIONS) {
        qWarning() << "[AgentEngine] Circuit breaker triggered after" << m_loopIterations << "iterations.";
        m_isRunning = false;
        emit statusChanged("⚠️ Circuit breaker: too many iterations. Task stopped.");
        emit errorOccurred(QString("Agent exceeded the maximum of %1 iterations. "
                                   "The task has been stopped to prevent an infinite loop. "
                                   "Please rephrase your request or try a simpler task.")
                           .arg(MAX_LOOP_ITERATIONS));
        return;
    }

    qInfo() << "[AgentEngine] Nudge content:" << nudge.left(200).replace("\n", " ") << "...";

    m_isRunning = true;
    resetStreamState();

    bool useJsonSchema = m_runner->profile() &&
                         m_runner->profile()->name().contains("claude", Qt::CaseInsensitive);

    if (useJsonSchema) {
        QJsonArray tools = m_toolExecutor->getToolDefinitionsJson();
        ContextManager::ContextStats stats;
        QJsonObject request = m_prompts->buildRequestJson(
            m_currentRole, nudge, session->messages, tools,
            m_activeTechniques, m_blackboard, 16000, true, QString(), &stats);
        emit contextStatsUpdated(stats);
        m_lastRawRequest = QJsonDocument(request).toJson();
        m_lastRawResponse.clear();
        m_llmRequestTimer.start(); m_llmTimeoutTimer->start(adaptiveLlmTimeout());
        m_runner->sendJson(request, m_config->workingFolder());
    } else {
        // Emit context stats for non-JSON providers too
        {
            const auto provider = m_config->activeProvider();
            ContextManager::PruningOptions pruneOpts;
            pruneOpts.maxTokens = provider.contextWindow;
            if (pruneOpts.maxTokens <= 0) pruneOpts.maxTokens = 32768; // Fallback
            ContextManager::ContextStats stats;
            ContextManager::prune(session->messages, pruneOpts, &stats);
            emit contextStatsUpdated(stats);
        }

        m_lastRawRequest = "--- System Prompt ---\n" + getSystemPrompt() +
                           "\n\n--- Nudge ---\n" + nudge;
        m_lastRawResponse.clear();
        m_llmRequestTimer.start(); m_llmTimeoutTimer->start(adaptiveLlmTimeout());
        m_runner->send(nudge, m_config->workingFolder(), {}, session->messages, getSystemPrompt());
    }
}

void AgentEngine::injectAutoContext(const QString& query) {
    m_autoContext.clear();
    auto chunks = m_indexer->search(query, 3);
    if (chunks.isEmpty()) return;

    m_autoContext = "\n\n### AUTOMATIC PROJECTS CONTEXT (Based on semantic search):\n";
    for (const auto& chunk : chunks) {
        m_autoContext += QString("--- File: %1 (Lines %2-%3) ---\n%4\n")
                            .arg(chunk.filePath).arg(chunk.startLine)
                            .arg(chunk.endLine).arg(chunk.content);
    }
}

} // namespace CodeHex
