#include <QObject>
#include <algorithm>
#include "AgentEngine.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <memory>
#include <QSysInfo>
#include <QOperatingSystemVersion>
#include "ResponseParser.h"
#include <QtConcurrent>
#include <QUuid>
#include <QDateTime>
#include "ToolExecutor.h"
#include "ResponseFilter.h"
#include "PromptManager.h"
#include "EnsembleManager.h"
#include "WalkthroughGenerator.h"
#include "ThinkingCache.h"
#include "rag/CodebaseIndexer.h"
#include <QStandardPaths>
#include <QDir>
#include "tools/ToolUtils.h"
#include "tools/SearchRepoTool.h"
#include "tools/SearchReplaceTool.h"
#include "tools/MathLogicTool.h"
#include "tools/VisualizeCodebaseTool.h"
#include "tools/CompleteTaskTool.h"
#include "tools/WebSearchTool.h"
#include "tools/AskAgentTool.h"
#include "tools/ActivateTechniqueTool.h"
#include "tools/PostNoteTool.h"
#include "tools/AnalyzeVisionTool.h"
#include "tools/ReadStacktraceTool.h"
#include "tools/BuildTool.h"
#include "tools/ExportKnowledgeGraphTool.h"
#include "tools/AnalyzePerformanceTool.h"
#include "tools/CreateWorkflowTool.h"
#include "tools/ModifyUiTool.h"
#include "rag/EmbeddingManager.h"
#include "rag/CodebaseIndexer.h"
#include "SessionManager.h"
#include "AppConfig.h"
#include "TokenCounter.h"
#include "ModelRouter.h" // Added
#include "ProjectAuditor.h"
#include "../cli/CliRunner.h"
#include "../cli/ConfigurableProfile.h"
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
    m_router = new ModelRouter(config, this); // Initialize Router
    m_manualApproval = false;
    m_activeTechniques.clear();
    m_blackboard.clear();
    // Default Permissions - use tool.name() case as standard
    m_toolPermissions["ReadFile"]      = Permission::Allow;
    m_toolPermissions["WriteFile"]     = Permission::Allow;
    m_toolPermissions["ListDirectory"] = Permission::Allow;
    m_toolPermissions["Bash"]          = Permission::Ask;
    m_toolPermissions["SearchFiles"]   = Permission::Allow;
    m_toolPermissions["Grep"]          = Permission::Allow;
    m_toolPermissions["Replace"]       = Permission::Allow;

    m_embeddings = new EmbeddingManager(this);
    m_indexer = new CodebaseIndexer(m_embeddings, this);
    m_filter = new ResponseFilter(this);
    m_prompts = new PromptManager(m_config, this);
    m_ensemble = new EnsembleManager(m_config, this);

    connect(m_ensemble, &CodeHex::EnsembleManager::responseReady, this, [this](const QString& response) {
        onOutputChunk(response);
        onRunnerFinished(0);
    });

    // Register tools
    m_toolExecutor->registerTool(std::static_pointer_cast<CodeHex::Tool>(std::make_shared<CodeHex::SearchReplaceTool>()));
    m_toolExecutor->registerTool(std::static_pointer_cast<CodeHex::Tool>(std::make_shared<CodeHex::SearchRepoTool>(m_indexer)));
    m_toolExecutor->registerTool(std::static_pointer_cast<CodeHex::Tool>(std::make_shared<CodeHex::MathLogicTool>()));
    m_toolExecutor->registerTool(std::static_pointer_cast<CodeHex::Tool>(std::make_shared<CodeHex::VisualizeCodebaseTool>()));
    m_toolExecutor->registerTool(std::static_pointer_cast<CodeHex::Tool>(std::make_shared<CodeHex::CompleteTaskTool>(m_sessions, m_config)));
    m_toolExecutor->registerTool(std::static_pointer_cast<CodeHex::Tool>(std::make_shared<CodeHex::WebSearchTool>(m_config)));

    // Item #2 & #5: Multi-Agent Consult
    m_toolExecutor->registerTool(std::make_shared<AskAgentTool>(this));
    m_toolExecutor->registerAlias("Consult", "AskAgent");
    m_toolExecutor->registerAlias("ConsultCollaborator", "AskAgent");

    // Phase 2: Advanced Strategies
    m_toolExecutor->registerTool(std::make_shared<ActivateTechniqueTool>(this));
    m_toolExecutor->registerTool(std::make_shared<PostNoteTool>(this));

    // Batch Roadmap: New Tools
    m_toolExecutor->registerTool(std::make_shared<CodeHex::AnalyzeVisionTool>(this));
    m_toolExecutor->registerTool(std::make_shared<CodeHex::ReadStacktraceTool>());
    m_toolExecutor->registerTool(std::make_shared<CodeHex::BuildTool>(m_config));
    m_toolExecutor->registerTool(std::make_shared<CodeHex::ExportKnowledgeGraphTool>(m_config));
    
    m_toolExecutor->registerAlias("Vision", "AnalyzeVision");
    m_toolExecutor->registerAlias("BuildPrj", "Build");

    // Final Roadmap Batch
    m_toolExecutor->registerTool(std::make_shared<CodeHex::AnalyzePerformanceTool>());
    m_toolExecutor->registerTool(std::make_shared<CodeHex::CreateWorkflowTool>());
    m_toolExecutor->registerTool(std::make_shared<CodeHex::ModifyUiTool>());
    
    m_toolExecutor->registerAlias("Profile", "AnalyzePerformance");
    m_toolExecutor->registerAlias("Skill", "CreateWorkflow");
    m_toolExecutor->registerAlias("Design", "ModifyUi");

    // Create the dedicated secondary runner for collaborator queries
    m_collaboratorRunner = new CliRunner(this);

    // --- LLM Timeout (#2): abort runner if no token arrives within LLM_TIMEOUT_MS ---
    m_llmTimeoutTimer = new QTimer(this);
    m_llmTimeoutTimer->setSingleShot(true);
    connect(m_llmTimeoutTimer, &QTimer::timeout, this, [this]() {
        qWarning() << "[AgentEngine] LLM timeout — no response after" << LLM_TIMEOUT_MS / 1000 << "s. Aborting.";
        m_runner->stop();
        m_isRunning = false;
        emit errorOccurred(QString("LLM did not respond within %1 seconds. Please check your provider or try again.")
                           .arg(LLM_TIMEOUT_MS / 1000));
    });

    // Connect Runner
    connect(m_runner, &CliRunner::outputChunk,    this, &AgentEngine::onOutputChunk);
    connect(m_runner, &CliRunner::rawOutput,      this, &AgentEngine::onRawOutput);
    connect(m_runner, &CliRunner::errorChunk,     this, &AgentEngine::onErrorChunk);
    connect(m_runner, &CliRunner::toolCallReady,  this, &AgentEngine::onToolCallReady);
    connect(m_runner, &CliRunner::finished,       this, &AgentEngine::onRunnerFinished);
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
    m_auditor->start(300000); // Audit every 5 minutes

    connect(m_toolExecutor, &ToolExecutor::toolFinished, this, &AgentEngine::onToolResultReceived);
}

void AgentEngine::loadPersistence() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(configDir).mkpath(".");
    
    ThinkingCache::instance().load(configDir + "/thinking_cache.json");
    if (m_indexer) {
        m_indexer->load(configDir + "/codebase_index.bin");
    }
}

void AgentEngine::savePersistence() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    
    ThinkingCache::instance().save();
    if (m_indexer) {
        m_indexer->save(configDir + "/codebase_index.bin");
    }
}

void AgentEngine::runLoop(const QString& prompt, const QStringList& imagePaths) {
    m_isRunning = true; // Ensure we are in running state

    // Switch model/profile based on current role BEFORE sending
    QString profileId = m_router->getProfileIdForRole(m_currentRole);
    LlmProvider provider = m_config->activeProvider(); // Default
    
    // Find provider by profileId
    bool found = false;
    for (const auto& p : m_config->providers()) {
        if (p.id == profileId) {
            provider = p;
            found = true;
            break;
        }
    }

    qInfo() << "AgentEngine: Using profile" << provider.name << "for role" << (int)m_currentRole;
    auto profile = ConfigurableProfile::fromProvider(provider);
    if (!m_selectedModel.isEmpty() && m_currentRole == AgentRole::Base) {
        profile->setModel(m_selectedModel);
    }
    m_runner->setProfile(std::move(profile));

    emit statusChanged(QString("Thinking (%1)...").arg(prompt.left(20)));
    auto session = m_sessions->currentSession();
    if (!session) {
        m_isRunning = false;
        return;
    }

    // Determine if we use the new Structured JSON Schema
    bool useJsonSchema = m_runner->profile() && m_runner->profile()->name().contains("claude", Qt::CaseInsensitive);

    QString ragContext;
    if (m_currentRole == AgentRole::RAG) {
        emit statusChanged("🔍 Searching Codebase...");
        auto results = m_indexer->search(prompt, 5);
        if (!results.isEmpty()) {
            ragContext = "\n### RELEVANT CODE CONTEXT (Local RAG):\n";
            for (const auto& chunk : results) {
                ragContext += QString("File: %1 (Lines %2-%3)\n```\n%4\n```\n\n")
                    .arg(chunk.filePath)
                    .arg(chunk.startLine)
                    .arg(chunk.endLine)
                    .arg(chunk.content);
            }
        }
    }

    QString systemPrompt = m_prompts->buildSystemPrompt(m_currentRole, ragContext, m_activeTechniques, m_blackboard);

    if (useJsonSchema) {
        emit statusChanged("🧠 Reasoning (JSON Schema)...");
        QJsonArray tools = m_toolExecutor->getToolDefinitionsJson();
        ContextManager::ContextStats stats;
        QJsonObject request = m_prompts->buildRequestJson(m_currentRole, prompt, session->messages, tools, 
                                                         m_activeTechniques, m_blackboard,
                                                         16000, true, ragContext, &stats);
        emit contextStatsUpdated(stats);
        m_lastRawRequest = QJsonDocument(request).toJson();
        m_lastRawResponse.clear();
        m_llmTimeoutTimer->start(LLM_TIMEOUT_MS);
        m_runner->sendJson(request, m_config->workingFolder());
    } else {
        emit statusChanged("🧠 Thinking...");
        QString implicitGoals = m_prompts->detectImplicitGoals(prompt);
        QString enrichedPrompt = implicitGoals + "\n\n### USER REQUEST:\n" + prompt;

        // Estimate tokens for non-JSON path
        const int inputTokens = TokenCounter::estimate(enrichedPrompt);
        session->updateTokens(inputTokens, 0);

        m_lastRawRequest = "--- System Prompt ---\n" + systemPrompt + "\n\n--- Prompt ---\n" + enrichedPrompt;
        m_lastRawResponse.clear();
        m_llmTimeoutTimer->start(LLM_TIMEOUT_MS);
        m_runner->send(enrichedPrompt, m_config->workingFolder(), {}, session->messages, systemPrompt);
    }
}

void AgentEngine::process(const QString& userInput, const QList<Attachment>& attachments) {
    auto session = m_sessions->currentSession();
    if (!session) return;

    // Prepare message
    Message userMsg;
    userMsg.id = QUuid::createUuid();
    userMsg.role = Message::Role::User;
    
    userMsg.addText(userInput);
    
    userMsg.timestamp = QDateTime::currentDateTime();
    
    QStringList imagePaths;
    for (const auto& att : attachments) {
        userMsg.addAttachment(att);
        if (att.type == Attachment::Type::Image) {
            imagePaths << att.filePath;
        }
    }

    // Always append user message to session for persistence and UI visibility
    // Safety guard: Don't append if ID exists (prevents UI sync duplicates)
    session->appendMessage(userMsg);
    session->save();

    if (m_isRunning) {
        qDebug() << "[AgentEngine] Already running. Enqueuing request.";
        m_requestQueue.enqueue({userInput, attachments});
        emit statusChanged(QString("Request queued (%1 in queue)").arg(m_requestQueue.size()));
        return;
    }

    // --- Phase 2: Context Compression Check ---
    if (session->messages.size() > 15) {
        qDebug() << "[AgentEngine] Session too long. Triggering compression...";
        emit statusChanged("📦 Compressing conversation history...");
        QString compactionPrompt = "### INTERNAL COMPACTION REQUEST ###\n"
                                   "The conversation is too long. Please provide a CONCISE SUMMARY...";
        m_isRunning = true;
        m_runner->send(compactionPrompt, m_config->workingFolder(), {}, session->messages, getSystemPrompt());
        return;
    }

    m_requestQueue.clear();
    resetStreamState();
    m_isRunning = true;
    m_loopIterations = 0; // Reset circuit breaker counter for each new user request
    m_lastToolCallFingerprints.clear(); // Reset semantic loop tracker

    // Background indexing
    (void)QtConcurrent::run(&CodebaseIndexer::indexDirectory, m_indexer, m_config->workingFolder());

    runLoop(userInput, imagePaths);
}

void AgentEngine::sendContinueRequest(const QString& nudge) {
    auto session = m_sessions->currentSession();
    if (!session) return;

    // --- Circuit Breaker (#1): guard against infinite loops ---
    ++m_loopIterations;
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

    m_isRunning = true;
    resetStreamState();

    bool useJsonSchema = m_runner->profile() && m_runner->profile()->name().contains("claude", Qt::CaseInsensitive);

    if (useJsonSchema) {
        QJsonArray tools = m_toolExecutor->getToolDefinitionsJson();
        ContextManager::ContextStats stats;
        QJsonObject request = m_prompts->buildRequestJson(m_currentRole, nudge, session->messages, tools, 
                                                         m_activeTechniques, m_blackboard,
                                                         16000, true, QString(), &stats);
        emit contextStatsUpdated(stats);
        m_lastRawRequest = QJsonDocument(request).toJson();
        m_lastRawResponse.clear();
        m_llmTimeoutTimer->start(LLM_TIMEOUT_MS);
        m_runner->sendJson(request, m_config->workingFolder());
    } else {
        m_lastRawRequest = "--- System Prompt ---\n" + getSystemPrompt() + "\n\n--- Nudge ---\n" + nudge;
        m_lastRawResponse.clear();
        m_llmTimeoutTimer->start(LLM_TIMEOUT_MS);
        m_runner->send(nudge, m_config->workingFolder(), {}, session->messages, getSystemPrompt());
    }
}

void AgentEngine::injectAutoContext(const QString& query) {
    m_autoContext.clear();
    auto chunks = m_indexer->search(query, 3); // Top 3 snippets
    if (chunks.isEmpty()) return;

    m_autoContext = "\n\n### AUTOMATIC PROJECTS CONTEXT (Based on semantic search):\n";
    for (const auto& chunk : chunks) {
        m_autoContext += QString("--- File: %1 (Lines %2-%3) ---\n%4\n")
                            .arg(chunk.filePath)
                            .arg(chunk.startLine)
                            .arg(chunk.endLine)
                            .arg(chunk.content);
    }
}

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

void AgentEngine::onOutputChunk(const QString& chunk) {
    // Reset LLM timeout on every token — runner is still alive
    if (m_llmTimeoutTimer->isActive()) {
        m_llmTimeoutTimer->start(LLM_TIMEOUT_MS);
    }
    m_lastRawResponse += chunk;
    QString filtered = m_filter->processChunk(chunk);
    if (!filtered.isEmpty()) {
        emit tokenReceived(filtered);
    }
}

void AgentEngine::onRawOutput(const QString& raw) {
    emit consoleOutput(raw);
}

void AgentEngine::onErrorChunk(const QString& chunk) {
    emit consoleOutput("[stderr] " + chunk);
}


void AgentEngine::setManualApproval(bool enabled) {
    m_manualApproval = enabled;
}

void AgentEngine::setToolPermission(const QString& toolName, Permission p) {
    m_toolPermissions[toolName] = p;
}

bool AgentEngine::isPathAllowed(const QString& path) const {
    if (path.isEmpty()) return true;
    
    QString workDir = QDir(m_config->workingFolder()).absolutePath();
    
    // Resolve relative paths against working directory, NOT process CWD
    QFileInfo info(QDir(workDir), path);
    QString absPath = info.absoluteFilePath();
    
    qDebug() << "[AgentEngine] isPathAllowed: path=" << path << " resolved=" << absPath << " workDir=" << workDir;
    
    // Sandbox: Allow only within working directory or its subdirectories
    bool allowed = absPath.startsWith(workDir);
    if (!allowed) {
        qWarning() << "[AgentEngine] SANDBOX BLOCKED: " << absPath << " is outside " << workDir;
    }
    return allowed;
}

AgentEngine::Permission AgentEngine::toolPermission(const QString& toolName) const {
    if (!m_manualApproval) {
        return Permission::Allow;
    }
    
    if (m_toolPermissions.contains(toolName)) {
        return m_toolPermissions.value(toolName);
    }
    
    return Permission::Ask;
}

void AgentEngine::onToolCallReady(const CodeHex::ToolCall& call) {
    qDebug() << "[AgentEngine] onToolCallReady: tool=" << call.name << " m_isRunning=" << m_isRunning;
    // NOTE: Do NOT check m_isRunning here — onRunnerFinished sets it to false
    // before calling this method. The guard was silently blocking ALL tool executions.

    // --- JSON Validation (#18): skip execution for calls with malformed input ---
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

    // --- Sandbox Check ---
    auto* session = m_sessions->currentSession();
    if (!session) return;

    if (call.input.contains("path")) {
        QString path = call.input.value("path").toString();
        if (!isPathAllowed(path)) {
            qWarning() << "[AgentEngine] Sandbox violation for path:" << path;
            emit errorOccurred("Sandbox violation: Path is outside of working directory: " + path);
            return;
        }
    }

    Permission p = toolPermission(call.name);
    qDebug() << "[AgentEngine] Permission for" << call.name << "=" << (int)p << " (0=Allow,1=Ask,2=Deny)";
    if (p == Permission::Deny) {
        emit errorOccurred("Tool execution denied: " + call.name);
        return;
    }
    
    if (p == Permission::Ask) {
        emit toolApprovalRequested(call);
        m_pendingCalls.append(call);
        m_isRunning = false; // Stop running while waiting for approval
        return;
    }

    // Save call for potential retry on transient failure (#7)
    m_lastExecutedCall = call;

    // Execute Tool
    qDebug() << "[AgentEngine] EXECUTING tool:" << call.name;
    
    // Add compact CALL log to chat
    Message callMsg;
    callMsg.id = QUuid::createUuid();
    callMsg.role = Message::Role::Assistant;
    CodeBlock callBlock;
    callBlock.type = BlockType::LogStep;
    callBlock.content = "⚙ CALL: " + call.name;
    callMsg.contentBlocks << callBlock;
    callMsg.timestamp = QDateTime::currentDateTime();
    session->appendMessage(callMsg);
    session->save();

    emit toolCallStarted(call.name, call.input);
    m_isRunning = true; // Stay in running state during tool execution
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
        callMsg.timestamp = QDateTime::currentDateTime();
        session->appendMessage(callMsg);
        session->save();
    }

    emit statusChanged(QString("Approved: Executing %1...").arg(call.name));
    emit toolCallStarted(call.name, call.input); // Emit started if it was waiting
    m_toolExecutor->execute(call, m_config->workingFolder());
}

void AgentEngine::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result) {
    Q_UNUSED(toolName);
    
    auto* session = m_sessions->currentSession();
    if (!session) return;

    // Create a NEW message for the Tool Result
    Message toolMsg;
    toolMsg.id = QUuid::createUuid();
    toolMsg.role = Message::Role::User; // Fallback for local models
    toolMsg.timestamp = QDateTime::currentDateTime();
    
    // Add the tool result data
    toolMsg.toolResults << result;
    
    // Add text block for display (Compact LogStep)
    CodeBlock block;
    block.type = BlockType::LogStep;
    block.content = (result.isError ? "❌ FAILED: " : "✅ DONE: ") + toolName;
    toolMsg.contentBlocks << block;
    toolMsg.addText(result.content); // Full result is still kept in the hidden text for model context/fallback

    session->appendMessage(toolMsg);
    session->save();

    // --- Loop Detection (#3 Semantic + #13 Output) ---
    // Track output content for exact-output loop detection
    m_lastToolResults.append(result.content.trimmed());
    if (m_lastToolResults.size() > MAX_LOOP_RESULTS) {
        m_lastToolResults.removeFirst();
    }

    // Build a semantic fingerprint from tool name + key parameters.
    // This detects loops where the agent calls the same tool with the same inputs
    // even if the outputs vary slightly (e.g., slightly different error messages).
    {
        // Use m_lastExecutedCall which was saved in onToolCallReady
        QString keyParam = m_lastExecutedCall.input.value("path").toString();
        if (keyParam.isEmpty()) keyParam = m_lastExecutedCall.input.value("command").toString().left(80);
        if (keyParam.isEmpty()) keyParam = m_lastExecutedCall.input.value("query").toString().left(80);
        QString semanticFp = toolName + ":" + keyParam.trimmed().toLower();
        m_lastToolCallFingerprints.append(semanticFp);
        if (m_lastToolCallFingerprints.size() > MAX_LOOP_RESULTS) {
            m_lastToolCallFingerprints.removeFirst();
        }
    }

    bool potentialLoop = false;
    if (m_lastToolResults.size() == MAX_LOOP_RESULTS) {
        // Check 1: identical outputs
        auto getFingerprint = [](QString s) {
            s = s.trimmed().toLower();
            s.remove(QRegularExpression("\\s+"));
            if (s.length() > 500) s = s.left(500);
            return s;
        };
        QString firstFp = getFingerprint(m_lastToolResults[0]);
        potentialLoop = std::all_of(m_lastToolResults.begin() + 1, m_lastToolResults.end(),
                                    [&](const QString& s){ return getFingerprint(s) == firstFp; });

        // Check 2: identical semantic call fingerprints (same tool + same key param)
        if (!potentialLoop && m_lastToolCallFingerprints.size() == MAX_LOOP_RESULTS) {
            const QString& firstCallFp = m_lastToolCallFingerprints.first();
            potentialLoop = std::all_of(m_lastToolCallFingerprints.begin() + 1,
                                        m_lastToolCallFingerprints.end(),
                                        [&](const QString& s){ return s == firstCallFp; });
        }
    }

    // Automatically trigger the next agent turn if the profile isn't already doing something
    if (!m_runner->isProfileRunning()) {
        // --- Phase 2: ISV (Integrated Synthesis & Validation) ---
        bool isEditTool = (toolName == "WriteFile" || toolName == "Replace" || toolName == "SearchReplace");
        if (isEditTool && !result.isError) {
            emit statusChanged("🔍 ISV: Automatic Validation...");
            QString isvNudge = QString("Tool %1 executed successfully. "
                                       "As part of Integrated Synthesis & Validation (ISV), you MUST now verify this change. "
                                       "Use Build, SyntaxCheck, or ReadFile to confirm the file is correct and consistent.")
                               .arg(toolName);
            sendContinueRequest(isvNudge);
            return;
        }

        QString sp = getSystemPrompt();
        if (!result.isError) {
            if (m_runner) {
                // For local models, a explicit nudge helps prevent hallucinations
                QString nudge = QString("Tool Executed: %1\nOutput: %2\n\nPlease ANALYZE this output and decide on the NEXT STEP or FINALIZE the task if no more actions are needed.")
                                .arg(toolName, result.content);
                
                if (potentialLoop) {
                    nudge += "\n⚠️ WARNING: I detected that you are repeating the same output multiple times. Please check if you are stuck in a logic loop and try a DIFFERENT approach.";
                }
                sendContinueRequest(nudge);
            }
        } else {
            // --- Retry with backoff (#7): re-execute transient failures before giving up ---
            static const int MAX_RETRIES = 2;
            if (m_lastExecutedCall.name == toolName && m_lastExecutedCall.retryCount < MAX_RETRIES) {
                m_lastExecutedCall.retryCount++;
                int delayMs = m_lastExecutedCall.retryCount * 500; // 500ms, 1000ms
                qWarning() << "[AgentEngine] Tool" << toolName << "failed. Retry"
                           << m_lastExecutedCall.retryCount << "/" << MAX_RETRIES
                           << "in" << delayMs << "ms";
                emit statusChanged(QString("⏳ Retry %1/%2: %3...")
                                   .arg(m_lastExecutedCall.retryCount).arg(MAX_RETRIES).arg(toolName));
                ToolCall retryCall = m_lastExecutedCall;
                QTimer::singleShot(delayMs, this, [this, retryCall]() {
                    if (m_syncTools) {
                        m_toolExecutor->executeSync(retryCall, m_config->workingFolder());
                    } else {
                        m_toolExecutor->execute(retryCall, m_config->workingFolder());
                    }
                });
            } else {
                // Retries exhausted — send error to LLM to try a different approach
                sendContinueRequest(QString("Tool '%1' failed after %2 attempt(s): %3\n"
                                            "Analyze the error and try a different approach.")
                                    .arg(toolName).arg(m_lastExecutedCall.retryCount + 1)
                                    .arg(result.content));
            }
        }
    } else {
        // If we are NOT running the nudge, then we might be finished
        // But usually onToolResultReceived is followed by a nudge or a thought.
    }
}

void AgentEngine::onRunnerFinished(int exitCode) {
    Q_UNUSED(exitCode);
    m_llmTimeoutTimer->stop(); // Runner finished — cancel the timeout
    m_isRunning = false;
    QString lastAssistantContent;
    auto session = m_sessions->currentSession();
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
        
        sendContinueRequest("WARNING: You just sent the EXACT SAME response. DO NOT repeat your previous thought or tool call. If you are stuck because the tool output is not what you expected, try a DIFFERENT approach or a DIFFERENT tool. If the task is finished, simply state 'TASK COMPLETE'.");
        return;
    }

    // --- Phase 1.5: Chain-of-Verification (CoVe) ---
    if (m_coveState == CoVeState::None && !currentResp.contains("<tool_call>")) {
        qDebug() << "[AgentEngine] CoVe: Entering Drafting -> VerifyingQuestions";
        emit statusChanged("🧐 Verifying facts (CoVe)...");
        m_coveState = CoVeState::VerifyingQuestions;
        
        QString prompt = "### CHAIN-OF-VERIFICATION (CoVe) - STEP 2: GENERATE QUESTIONS ###\n"
                         "Based on your previous response, generate a list of verification questions to cross-check any facts or claims you made. "
                         "Focus on numbers, dates, technical details, and logical assumptions.\n\n"
                         "### YOUR RESPONSE:\n" + currentResp;
        
        sendContinueRequest(prompt);
        return;
    } else if (m_coveState == CoVeState::VerifyingQuestions) {
        qDebug() << "[AgentEngine] CoVe: VerifyingQuestions -> Answering";
        emit statusChanged("🔍 Answering verification questions...");
        m_coveState = CoVeState::Answering;
        
        QString prompt = "### CHAIN-OF-VERIFICATION (CoVe) - STEP 3: ANSWER QUESTIONS ###\n"
                         "Answer the verification questions you just generated. Be objective and factual.\n";
        
        sendContinueRequest(prompt);
        return;
    } else if (m_coveState == CoVeState::Answering) {
        qDebug() << "[AgentEngine] CoVe: Answering -> Finalizing";
        emit statusChanged("✨ Finalizing response...");
        m_coveState = CoVeState::Finalizing;
        
        QString prompt = "### CHAIN-OF-VERIFICATION (CoVe) - STEP 4: FINAL RESPONSE ###\n"
                         "Incorporate the findings from your verification steps. If you found errors, correct them. "
                         "Provide the final, verified response to the user. DO NOT include the internal CoVe steps in the final output.";
        
        sendContinueRequest(prompt);
        return;
    }
    
    m_coveState = CoVeState::None; // Reset after completion
    m_isRunning = false;

    ResponseParser::ParseResult parseResult = ResponseParser::parse(currentResp);

    // --- Phase 2: Confidence Anchor ---
    if (parseResult.confidenceScore < 5 && parseResult.toolCalls.isEmpty()) {
        qDebug() << "[AgentEngine] Low confidence detected:" << parseResult.confidenceScore;
        emit statusChanged("⚠️ Low confidence. Nudging for research...");
        sendContinueRequest(QString("Your confidence score is low (%1/10). "
                                    "Please perform additional research using Grep, ReadFile, or SearchRepo before making changes.")
                            .arg(parseResult.confidenceScore));
        return;
    }

    // --- Phase 5: Auto-walkthrough generation is now triggered by CompleteTask tool ---
    
    m_lastRawResponse = currentResp;
    buildAssistantMessage(parseResult, currentResp);

    qDebug() << "[AgentEngine] onRunnerFinished: parsedCalls.size()=" << parseResult.toolCalls.size();
    if (!parseResult.toolCalls.isEmpty()) {
        qDebug() << "[AgentEngine] Dispatching tool:" << parseResult.toolCalls.first().name;
        onToolCallReady(parseResult.toolCalls.first());
    } else {
        qDebug() << "[AgentEngine] No tool calls found in response";
        emit statusChanged("");
        
        if (m_requestQueue.isEmpty()) {
            cleanupScratchpad();
        }
        
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

    // 1. Add Thoughts
    for (const auto& thought : result.thoughts) {
        CodeBlock b;
        b.type = BlockType::Thinking;
        b.content = thought.content;
        b.isCollapsed = thought.isCollapsed;
        msg.contentBlocks << b;
        msg.contentTypes << Message::ContentType::Thinking;
    }
    
    // 2. Add Main Text
    if (!result.cleanText.isEmpty()) {
        msg.addText(result.cleanText);
    }

    // Handle session auto-rename if first message
    if (session->messages.size() <= 2) { 
        QString firstText = msg.textFromContentBlocks();
        QString title = firstText.section(QRegularExpression("[.!?]"), 0, 0).trimmed();
        title.remove(QRegularExpression("[^\\w\\s-]")); // Sanitize title (Item 43)
        if (title.length() > 40) title = title.left(37) + "...";
        if (title.trimmed().isEmpty()) title = "New Task";
        session->title = title.trimmed();
    }

    session->appendMessage(msg);
    emit responseComplete(msg);
    emit statusChanged("");
}


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
    if (dir.exists()) {
        dir.removeRecursively();
    }
    dir.mkpath(".");
    qDebug() << "[AgentEngine] Scratchpad cleaned up.";
}

void AgentEngine::saveDebugLog(const QString& targetDir) {
    QDir dir(targetDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    auto saveFile = [&](const QString& name, const QString& content) {
        QFile file(dir.filePath(name));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(content.toUtf8());
            file.close();
        }
    };

    saveFile("request_raw.txt", m_lastRawRequest);
    saveFile("response_raw.txt", m_lastRawResponse);
    
    // Copy the global application log
    QString logPath = QDir::homePath() + "/.codehex/application.log";
    if (QFile::exists(logPath)) {
        QFile::copy(logPath, dir.filePath("application.log"));
    }
    
    if (auto* session = m_sessions->currentSession()) {
        QJsonArray messages;
        for (const auto& msg : session->messages) {
            messages.append(msg.toJson());
        }
        QJsonObject sessionObj;
        sessionObj["id"] = session->id.toString();
        sessionObj["title"] = session->title;
        sessionObj["messages"] = messages;
        saveFile("session_state.json", QJsonDocument(sessionObj).toJson());
    }

    QString sysInfo = QString("OS: %1\nArch: %2\nProvider: %3\nModel: %4\nRole: %5\nDir: %6")
        .arg(QSysInfo::prettyProductName())
        .arg(QSysInfo::currentCpuArchitecture())
        .arg(m_config->activeProvider().name)
        .arg(m_config->activeProvider().selectedModel)
        .arg((int)m_currentRole)
        .arg(m_config->workingFolder());
    saveFile("system_info.txt", sysInfo);

    qInfo() << "Debug logs saved to" << targetDir;
}

void AgentEngine::resetStreamState() {
    m_filter->reset();
    m_pendingCalls.clear();
}

QString AgentEngine::getSystemPrompt() const {
    QString sp = m_prompts->buildSystemPrompt(m_currentRole, m_autoContext, m_activeTechniques, m_blackboard);
    
    // Add forced context from manual inclusion (Item 46)
    if (!m_forcedContextFiles.isEmpty()) {
        sp += "\n\n### FORCED CONTEXT (ATTACHED FILES):\n";
        for (const QString& path : m_forcedContextFiles) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                sp += QString("\n-- File: %1 --\n").arg(QDir(m_config->workingFolder()).relativeFilePath(path));
                sp += f.readAll();
                sp += "\n---------------------\n";
            }
        }
    }

    if (m_toolExecutor) {
        sp += "\n\n" + m_toolExecutor->getToolDefinitions();
    }
    return sp;
}

// ---------------------------------------------------------------------------
// Item #5: Multi-Agent Collaborative Mode
// ---------------------------------------------------------------------------

QString AgentEngine::consultCollaborator(const QString& prompt, const QString& role)
{
    // Build a role-specific system prompt for the collaborator.
    QString collaboratorSystemPrompt;
    
    // Attempt to load specialized role prompt if it matches our internal AgentRole enum
    AgentRole targetRole = AgentRole::Base;
    if (role.contains("Architect", Qt::CaseInsensitive)) targetRole = AgentRole::Architect;
    else if (role.contains("Debugger", Qt::CaseInsensitive)) targetRole = AgentRole::Debugger;
    else if (role.contains("Security Specialist", Qt::CaseInsensitive) || role.contains("Auditor", Qt::CaseInsensitive)) targetRole = AgentRole::SecurityAuditor;
    else if (role.contains("Reviewer", Qt::CaseInsensitive)) targetRole = AgentRole::Reviewer;
    else if (role.contains("Refactor", Qt::CaseInsensitive)) targetRole = AgentRole::Refactor;

    if (targetRole != AgentRole::Base) {
        collaboratorSystemPrompt = m_prompts->loadRolePrompt(targetRole);
    } else {
        collaboratorSystemPrompt = QString(
            "You are a specialized AI %1. Your task is to provide a concise, expert-level "
            "analysis of the problem presented to you. Be direct, critical, and constructive. "
            "Do NOT use tool calls. Respond in plain text with your analysis."
        ).arg(role);
    }

    // Acquire the active provider's profile for the collaborator runner.
    // This reuses the same LLM backend but with a fully independent context.
    LlmProvider provider = m_config->activeProvider();
    auto profile = ConfigurableProfile::fromProvider(provider);
    if (!m_selectedModel.isEmpty()) {
        profile->setModel(m_selectedModel);
    }
    m_collaboratorRunner->setProfile(std::move(profile));

    m_collaboratorResponse.clear();
    bool finished = false;
    QString errorMsg;

    // --- Synchronous bridge via Qt's meta-object connection system ---
    // We create temporary connections that feed output chunks into m_collaboratorResponse.
    // The QEventLoop below processes events (including signal deliveries) until 'finished'
    // transitions to true, then quits. A 60 s timeout guards against infinite hangs.
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(60000); // 60 seconds

    // Lambda captures by reference — safe because loop is on the stack.
    auto chunkConn = connect(m_collaboratorRunner, &CliRunner::outputChunk,
        [this](const QString& chunk) {
            m_collaboratorResponse += chunk;
        });

    auto finishedConn = connect(m_collaboratorRunner, &CliRunner::finished,
        [&loop, &finished](int /*exitCode*/) {
            finished = true;
            loop.quit();
        });

    auto errorConn = connect(m_collaboratorRunner, &CliRunner::errorChunk,
        [&errorMsg](const QString& err) {
            errorMsg += err;
        });

    auto timeoutConn = connect(&timeoutTimer, &QTimer::timeout,
        [&loop, &finished]() {
            finished = true;
            loop.quit();
        });

    // Fire the LLM call — history is empty (stateless collaborator)
    m_collaboratorRunner->send(prompt, m_config->workingFolder(), {}, {}, collaboratorSystemPrompt);
    timeoutTimer.start();

    // Block the CALLING thread's event loop until the collaborator is done.
    // NOTE: This MUST be called from a background thread (QtConcurrent / BashTool thread).
    // If called from the main thread it will block signal delivery and deadlock.
    loop.exec();

    // Tear down temporary connections
    disconnect(chunkConn);
    disconnect(finishedConn);
    disconnect(errorConn);
    disconnect(timeoutConn);
    timeoutTimer.stop();

    if (!timeoutTimer.isActive() && m_collaboratorResponse.isEmpty()) {
        // Timed out
        qWarning() << "[AgentEngine] consultCollaborator: timed out waiting for collaborator response.";
        return QString("[Collaborator timed out after 60 seconds. No response received.]");
    }

    if (m_collaboratorResponse.isEmpty() && !errorMsg.isEmpty()) {
        qWarning() << "[AgentEngine] consultCollaborator error:" << errorMsg;
        return QString("[Collaborator error: %1]").arg(errorMsg);
    }

    qInfo() << "[AgentEngine] consultCollaborator: got response of" << m_collaboratorResponse.size() << "chars.";
    return m_collaboratorResponse.trimmed();
}

} // namespace CodeHex
