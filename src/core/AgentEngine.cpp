/**
 * @file AgentEngine.cpp
 * @brief Constructor, lifecycle (stop/reset), permission management, and utility methods.
 *
 * Heavy implementation is split across:
 *   AgentEngine_Loop.cpp   — process(), runLoop(), sendContinueRequest(), injectAutoContext()
 *   AgentEngine_Runner.cpp — onOutputChunk/Raw/Error, onRunnerFinished, buildAssistantMessage
 *   AgentEngine_Tools.cpp  — onToolCallReady, approveToolCall, onToolResultReceived
 *   AgentEngine_Collab.cpp — consultCollaborator (Multi-Agent, roadmap #5)
 */
#include "AgentEngine.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QtConcurrent>
#include <memory>
#include "AppConfig.h"
#include "EnsembleManager.h"
#include "ModelRouter.h"
#include "ProjectAuditor.h"
#include "PromptManager.h"
#include "ResponseFilter.h"
#include "SessionManager.h"
#include "ThinkingCache.h"
#include "ToolExecutor.h"
#include "AgentPipeline.h"
#include "AgentGraph.h"
#include "WalkthroughGenerator.h"
#include "rag/CodebaseIndexer.h"
#include "rag/EmbeddingManager.h"
#include "tools/ActivateTechniqueTool.h"
#include "tools/AnalyzePerformanceTool.h"
#include "tools/AnalyzeVisionTool.h"
#include "tools/AskAgentTool.h"
#include "tools/BuildTool.h"
#include "tools/CompleteTaskTool.h"
#include "tools/CreateWorkflowTool.h"
#include "tools/ExportKnowledgeGraphTool.h"
#include "tools/MathLogicTool.h"
#include "tools/ModifyUiTool.h"
#include "tools/PostNoteTool.h"
#include "tools/ReadStacktraceTool.h"
#include "tools/SearchReplaceTool.h"
#include "tools/SearchRepoTool.h"
#include "tools/VisualizeCodebaseTool.h"
#include "tools/WebSearchTool.h"
#include "../cli/CliRunner.h"
#include "../data/Session.h"

namespace CodeHex {

AgentEngine::AgentEngine(AppConfig* config,
                         SessionManager* sessions,
                         CliRunner* runner,
                         ToolExecutor* toolExecutor,
                         QObject* parent)
    : QObject(parent), m_sessions(sessions), m_runner(runner),
      m_toolExecutor(toolExecutor), m_currentRole(AgentRole::Base), m_config(config)
{
    m_pipeline = new AgentPipeline(this, this);
    m_graph    = new AgentGraph(this);

    // LangGraph-style orchestration
    connect(m_graph, &AgentGraph::nodeStarted, this, [this](const QString& node, AgentRole role, const QString& prompt) {
        emit terminalOutput(QString("[%1] 🕸️ Graph Node: %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), node));
        setRole(role);
        runLoop(prompt, {});
    });

    m_router = new ModelRouter(config, this);
    m_manualApproval = false;
    m_activeTechniques.clear();
    m_blackboard.clear();

    // Default permissions
    m_toolPermissions["ReadFile"]      = Permission::Allow;
    m_toolPermissions["WriteFile"]     = Permission::Allow;
    m_toolPermissions["ListDirectory"] = Permission::Allow;
    m_toolPermissions["Bash"]          = Permission::Ask;
    m_toolPermissions["SearchFiles"]   = Permission::Allow;
    m_toolPermissions["Grep"]          = Permission::Allow;
    m_toolPermissions["Replace"]       = Permission::Allow;

    m_embeddings = new EmbeddingManager(this);
    m_indexer    = new CodebaseIndexer(m_embeddings, this);
    m_filter     = new ResponseFilter(this);
    m_prompts    = new PromptManager(m_config, this);
    m_ensemble   = new EnsembleManager(m_config, this);

    connect(m_ensemble, &EnsembleManager::responseReady, this, [this](const QString& response) {
        onOutputChunk(response);
        onRunnerFinished(0);
    });

    // Register tools
    m_toolExecutor->registerTool(std::make_shared<SearchReplaceTool>());
    m_toolExecutor->registerTool(std::make_shared<SearchRepoTool>(m_indexer));
    m_toolExecutor->registerTool(std::make_shared<MathLogicTool>());
    m_toolExecutor->registerTool(std::make_shared<VisualizeCodebaseTool>());
    m_toolExecutor->registerTool(std::make_shared<CompleteTaskTool>(m_sessions, m_config));
    m_toolExecutor->registerTool(std::make_shared<WebSearchTool>(m_config));
    m_toolExecutor->registerTool(std::make_shared<AskAgentTool>(this));
    m_toolExecutor->registerAlias("Consult", "AskAgent");
    m_toolExecutor->registerAlias("ConsultCollaborator", "AskAgent");
    m_toolExecutor->registerTool(std::make_shared<ActivateTechniqueTool>(this));
    m_toolExecutor->registerTool(std::make_shared<PostNoteTool>(this));
    m_toolExecutor->registerTool(std::make_shared<AnalyzeVisionTool>(this));
    m_toolExecutor->registerTool(std::make_shared<ReadStacktraceTool>());
    m_toolExecutor->registerTool(std::make_shared<BuildTool>(m_config));
    m_toolExecutor->registerTool(std::make_shared<ExportKnowledgeGraphTool>(m_config));
    m_toolExecutor->registerAlias("Vision", "AnalyzeVision");
    m_toolExecutor->registerAlias("BuildPrj", "Build");
    m_toolExecutor->registerTool(std::make_shared<AnalyzePerformanceTool>());
    m_toolExecutor->registerTool(std::make_shared<CreateWorkflowTool>());
    m_toolExecutor->registerTool(std::make_shared<ModifyUiTool>());
    m_toolExecutor->registerAlias("Profile", "AnalyzePerformance");
    m_toolExecutor->registerAlias("Skill", "CreateWorkflow");
    m_toolExecutor->registerAlias("Design", "ModifyUi");

    // Dedicated secondary runner for collaborator queries (roadmap #5)
    m_collaboratorRunner = new CliRunner(this);

    // Role Pipeline (P-4)
    m_pipeline = new AgentPipeline(this, this);
    connect(m_pipeline, &AgentPipeline::stageStarted, this,
        [this](int index, AgentRole role, const QString&) {
            static const QMap<AgentRole, QString> roleNames = {
                {AgentRole::Base, "Base"}, {AgentRole::Explorer, "Explorer"},
                {AgentRole::Executor, "Executor"}, {AgentRole::Reviewer, "Reviewer"},
                {AgentRole::Debugger, "Debugger"}, {AgentRole::Refactor, "Refactor"},
                {AgentRole::Architect, "Architect"}, {AgentRole::SecurityAuditor, "Security"},
                {AgentRole::RAG, "RAG"},
            };
            emit statusChanged(QString("🔄 Pipeline [%1/%2]: %3")
                .arg(index + 1).arg(m_pipeline->totalStages())
                .arg(roleNames.value(role, "Base")));
        });
    connect(m_pipeline, &AgentPipeline::pipelineComplete, this,
        [this](const QString&) {
            emit statusChanged("✅ Pipeline complete");
            m_currentRole = AgentRole::Base; // reset role after pipeline
        });

    // LLM Timeout (#2): abort runner if no token arrives within LLM_TIMEOUT_DEFAULT_MS
    m_llmTimeoutTimer = new QTimer(this);
    m_llmTimeoutTimer->setSingleShot(true);
    connect(m_llmTimeoutTimer, &QTimer::timeout, this, [this]() {
        qWarning() << "[AgentEngine] LLM timeout — no response after" << LLM_TIMEOUT_DEFAULT_MS / 1000 << "s. Aborting.";
        m_runner->stop();
        m_isRunning = false;
        emit errorOccurred(QString("LLM did not respond within %1 seconds. "
                                   "Please check your provider or try again.")
                           .arg(LLM_TIMEOUT_DEFAULT_MS / 1000));
    });

    // Connect runner signals
    connect(m_runner, &CliRunner::outputChunk,   this, &AgentEngine::onOutputChunk);
    connect(m_runner, &CliRunner::rawOutput,     this, &AgentEngine::onRawOutput);
    connect(m_runner, &CliRunner::errorChunk,    this, &AgentEngine::onErrorChunk);
    connect(m_runner, &CliRunner::toolCallReady, this, &AgentEngine::onToolCallReady);
    connect(m_runner, &CliRunner::finished,      this, &AgentEngine::onRunnerFinished);
    connect(m_runner, &CliRunner::tokenStats, this, [this](int in, int out) {
        auto* session = m_sessions->currentSession();
        if (session) {
            session->updateTokens(in, out);
            emit tokenStatsUpdated(in, out);
        }
    });

    m_auditor = new ProjectAuditor(this);
    m_auditor->setWorkingDirectory(m_config->workingFolder());
    connect(m_auditor, &ProjectAuditor::auditSuggestion, this, &AgentEngine::onAuditSuggestion);
    m_auditor->start(300000);

    // Route tool results: batch mode → onBatchToolFinished, single → onToolResultReceived
    connect(m_toolExecutor, &ToolExecutor::toolFinished, this,
        [this](const QString& toolName, const CodeHex::ToolResult& result) {
            if (m_batchPending.load() > 0)
                onBatchToolFinished(toolName, result);
            else
                onToolResultReceived(toolName, result);
        });
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void AgentEngine::loadPersistence() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(configDir).mkpath(".");
    ThinkingCache::instance().load(configDir + "/thinking_cache.json");
    if (m_indexer) m_indexer->load(configDir + "/codebase_index.bin");
}

void AgentEngine::savePersistence() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    ThinkingCache::instance().save();
    if (m_indexer) m_indexer->save(configDir + "/codebase_index.bin");
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AgentEngine::stop() {
    m_isRunning = false;
    m_runner->stop();
    m_toolExecutor->stop();
    m_requestQueue.clear();
}

void AgentEngine::reset() {
    stop();
    m_pendingCalls.clear();
    resetStreamState();
    cleanupScratchpad();
}

bool AgentEngine::isRunning() const {
    return m_isRunning || m_runner->isProfileRunning();
}

// ---------------------------------------------------------------------------
// Permissions
// ---------------------------------------------------------------------------

void AgentEngine::setManualApproval(bool enabled) {
    m_manualApproval = enabled;
}

void AgentEngine::setToolPermission(const QString& toolName, Permission p) {
    m_toolPermissions[toolName] = p;
}

AgentEngine::Permission AgentEngine::toolPermission(const QString& toolName) const {
    if (!m_manualApproval) return Permission::Allow;
    return m_toolPermissions.value(toolName, Permission::Ask);
}

bool AgentEngine::isPathAllowed(const QString& path) const {
    if (path.isEmpty()) return true;
    QString workDir = QDir(m_config->workingFolder()).absolutePath();
    QFileInfo info(QDir(workDir), path);
    QString absPath = info.absoluteFilePath();
    qDebug() << "[AgentEngine] isPathAllowed:" << path << "->" << absPath;
    bool allowed = absPath.startsWith(workDir);
    if (!allowed)
        qWarning() << "[AgentEngine] SANDBOX BLOCKED:" << absPath << "is outside" << workDir;
    return allowed;
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

void AgentEngine::onAuditSuggestion(const QString& suggestion) {
    qDebug() << "[AgentEngine] Project Audit Suggestion:" << suggestion;
    emit statusChanged("📋 Auditor: " + suggestion);
}

void AgentEngine::processNextQueueItem() {
    if (m_requestQueue.isEmpty()) return;
    PendingRequest next = m_requestQueue.dequeue();
    qDebug() << "[AgentEngine] Processing next queued request.";
    process(next.input, next.attachments);
}

void AgentEngine::cleanupScratchpad() {
    QString path = m_config->workingFolder() + "/.agent/scratchpad";
    QDir dir(path);
    if (dir.exists()) dir.removeRecursively();
    dir.mkpath(".");
    qDebug() << "[AgentEngine] Scratchpad cleaned up.";
}

void AgentEngine::resetStreamState() {
    m_filter->reset();
    m_pendingCalls.clear();
}

QString AgentEngine::getSystemPrompt() const {
    QString sp = m_prompts->buildSystemPrompt(
        m_currentRole, m_autoContext, m_activeTechniques, m_blackboard);

    if (!m_forcedContextFiles.isEmpty()) {
        sp += "\n\n### FORCED CONTEXT (ATTACHED FILES):\n";
        for (const QString& path : m_forcedContextFiles) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                sp += QString("\n-- File: %1 --\n").arg(
                    QDir(m_config->workingFolder()).relativeFilePath(path));
                sp += f.readAll();
                sp += "\n---------------------\n";
            }
        }
    }

    if (m_toolExecutor) sp += "\n\n" + m_toolExecutor->getToolDefinitions();
    return sp;
}

void AgentEngine::saveDebugLog(const QString& targetDir) {
    QDir dir(targetDir);
    if (!dir.exists()) dir.mkpath(".");

    auto saveFile = [&](const QString& name, const QString& content) {
        QFile file(dir.filePath(name));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            file.write(content.toUtf8());
    };

    saveFile("request_raw.txt", m_lastRawRequest);
    saveFile("response_raw.txt", m_lastRawResponse);

    QString logPath = QDir::homePath() + "/.codehex/application.log";
    if (QFile::exists(logPath)) QFile::copy(logPath, dir.filePath("application.log"));

    if (auto* session = m_sessions->currentSession()) {
        QJsonArray messages;
        for (const auto& msg : session->messages) messages.append(msg.toJson());
        QJsonObject sessionObj;
        sessionObj["id"]       = session->id.toString();
        sessionObj["title"]    = session->title;
        sessionObj["messages"] = messages;
        saveFile("session_state.json", QJsonDocument(sessionObj).toJson());
    }

    saveFile("system_info.txt",
             QString("OS: %1\nArch: %2\nProvider: %3\nModel: %4\nRole: %5\nDir: %6")
             .arg(QSysInfo::prettyProductName())
             .arg(QSysInfo::currentCpuArchitecture())
             .arg(m_config->activeProvider().name)
             .arg(m_config->activeProvider().selectedModel)
             .arg(static_cast<int>(m_currentRole))
             .arg(m_config->workingFolder()));

    qInfo() << "Debug logs saved to" << targetDir;
}

// ---------------------------------------------------------------------------
// Adaptive LLM Timeout (P-10)
// ---------------------------------------------------------------------------

int AgentEngine::adaptiveLlmTimeout() const {
    if (m_llmResponseTimesMs.size() < 3) return LLM_TIMEOUT_DEFAULT_MS;

    // Calculate average of recent response times
    qint64 sum = 0;
    for (int t : m_llmResponseTimesMs) sum += t;
    int avg = static_cast<int>(sum / m_llmResponseTimesMs.size());

    // Timeout = 3× average, clamped between 30s and 300s
    int timeout = qBound(30'000, avg * 3, 300'000);
    return timeout;
}

} // namespace CodeHex
