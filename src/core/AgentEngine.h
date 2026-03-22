#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QJsonObject>
#include <QQueue>
#include <QTimer>
#include <QElapsedTimer>
#include <atomic>
#include "Message.h"
#include "ToolCall.h"
#include "Attachment.h"
#include "AgentRole.h"

#include "ContextManager.h"
#include "ResponseParser.h"

namespace CodeHex {

class CliRunner;
class ToolExecutor;
class SessionManager;
class AppConfig;
class CodebaseIndexer;
class EmbeddingManager;
class ProjectAuditor;
class ResponseFilter;
class PromptManager;
class EnsembleManager;
class ModelRouter;
class AgentPipeline;
class AgentGraph;

/**
 * @brief The AgentEngine class encapsulates the core "thinking" loop of the agent.
 * 
 * It coordinates between the LLM (CliRunner) and the ToolExecutor.
 * It manages the recursive tool call loop and maintains the conversation state.
 */
class AgentEngine : public QObject {
    Q_OBJECT
public:
    enum class Permission { Allow, Ask, Deny };

    explicit AgentEngine(AppConfig* config, 
                         SessionManager* sessions, 
                         CliRunner* runner, 
                         ToolExecutor* toolExecutor,
                         QObject* parent = nullptr);

    /**
     * @brief Start a new interaction with the agent.
     */
    void process(const QString& userInput, const QList<Attachment>& attachments = {});

    /**
     * @brief Stop current processing.
     */
    void stop();

    /**
     * @brief Reset the agent state, clearing queues and pending calls.
     */
    void reset();

    bool isRunning() const;
    void setRunningForTest(bool r) { m_isRunning = r; }
    void setSyncTools(bool s) { m_syncTools = s; }

    void setManualApproval(bool enabled);
    void approveToolCall(const CodeHex::ToolCall& call);
    
    // Permission Management
    void setToolPermission(const QString& toolName, Permission p);
    Permission toolPermission(const QString& toolName) const;

    void setRole(AgentRole role);
    AgentRole currentRole() const { return m_currentRole; }

    /**
     * @brief Delegates a sub-task to a different role using the collaborator runner.
     * Non-blocking: result is delivered via the delegationComplete signal.
     * @param targetRole  Role to use for the sub-task.
     * @param prompt      The task/question for the delegated role.
     */
    void delegateToRole(AgentRole targetRole, const QString& prompt);

    void setSelectedModel(const QString& model) { m_selectedModel = model; }
    QString selectedModel() const { return m_selectedModel; }

    AgentPipeline* pipeline() const { return m_pipeline; }

    /**
     * @brief Saves the last raw request, response, and session state to a directory.
     */
    void saveDebugLog(const QString& targetDir);

    /**
     * @brief Sets the list of files to be injected as mandatory context in the next prompt.
     */
    void setForcedContextFiles(const QSet<QString>& files) { m_forcedContextFiles = files; }

    void loadPersistence();
    void savePersistence();

    /**
     * @brief Consults an independent LLM collaborator synchronously (legacy).
     * @deprecated Use consultCollaboratorAsync() for non-blocking operation.
     */
    QString consultCollaborator(const QString& prompt, const QString& role = "Collaborator");

    /**
     * @brief Starts an async collaborator query. When finished, emits toolFinished
     * with a synthetic ToolResult containing the collaborator's response.
     * Does NOT block the calling thread.
     *
     * @param call  The original AskAgent tool call (used for result routing).
     * @param prompt   The question to send.
     * @param role     Role description (e.g. "Architect", "Security Auditor").
     */
    void consultCollaboratorAsync(const ToolCall& call, const QString& prompt, const QString& role = "Collaborator");
    
    // CoVe State Machine
    enum class CoVeState { None, Drafting, VerifyingQuestions, Answering, Finalizing };
    bool isCoVeActive() const { return m_coveState != CoVeState::None; }

signals:
    void statusChanged(const QString& status);
    void tokenReceived(const QString& token);
    void tokenStatsUpdated(int input, int output);
    void contextStatsUpdated(const CodeHex::ContextManager::ContextStats& stats);
    void consoleOutput(const QString& raw);
    void terminalOutput(const QString& line);   // formatted log line for TerminalPanel
    void terminalError(const QString& line);    // error line for TerminalPanel
    void toolCallStarted(const QString& toolName, const QJsonObject& input);
    void toolApprovalRequested(const CodeHex::ToolCall& call);
    void responseComplete(const Message& msg);
    void errorOccurred(const QString& error);
    void potentialLoopDetected(const QString& message);

    /**
     * @brief Emitted when auto-detection switches the role away from Base.
     * @param role   The detected role.
     * @param score  Keyword match count (confidence indicator).
     */
    void roleAutoDetected(CodeHex::AgentRole role, int score);

    /**
     * @brief Emitted when a delegateToRole() call completes.
     * @param targetRole  The role that was delegated to.
     * @param result      The delegated agent's response text.
     */
    void delegationComplete(CodeHex::AgentRole targetRole, const QString& result);

public slots:
    void onOutputChunk(const QString& chunk);
    void onRawOutput(const QString& raw);
    void onErrorChunk(const QString& chunk);
    void onToolCallReady(const CodeHex::ToolCall& call);
    void onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result);
    void onRunnerFinished(int exitCode);
    void onAuditSuggestion(const QString& suggestion);

    // Techniques & Blackboard
    void activateTechnique(const QString& name) { m_activeTechniques.append(name); m_activeTechniques.removeDuplicates(); }
    void deactivateTechnique(const QString& name) { m_activeTechniques.removeAll(name); }
    void postNote(const QString& key, const QString& value) { m_blackboard[key] = value; }
    QString getNote(const QString& key) const { return m_blackboard.value(key); }
    QMap<QString, QString> blackboard() const { return m_blackboard; }

private:
    void runLoop(const QString& prompt, const QStringList& imagePaths);
    void buildAssistantMessage(const ResponseParser::ParseResult& result, const QString& rawText);

    SessionManager* m_sessions;
    CliRunner*      m_runner;
    ToolExecutor*   m_toolExecutor;
    EmbeddingManager* m_embeddings;
    CodebaseIndexer*  m_indexer;
    ProjectAuditor*   m_auditor;
    ResponseFilter*   m_filter;
    PromptManager*    m_prompts;
    EnsembleManager*  m_ensemble;
    ModelRouter*      m_router; // Added

    bool m_manualApproval = false;
    CoVeState m_coveState = CoVeState::None;
    QMap<QString, Permission> m_toolPermissions;
    AgentRole m_currentRole = AgentRole::Base;
    AppConfig* m_config;
    QList<ToolCall> m_pendingCalls;
    struct PendingRequest {
        QString input;
        QList<Attachment> attachments;
    };
    QQueue<PendingRequest> m_requestQueue;
    bool m_isRunning = false;
    bool m_syncTools = false;
    QString m_selectedModel;

    QString m_autoContext; 
    QSet<QString> m_forcedContextFiles;
    bool m_isThinkingStream = false;
    QString m_thoughtBuffer;
    void sendContinueRequest(const QString& nudge);
    void injectAutoContext(const QString& query);

    bool isPathAllowed(const QString& path) const;
    void processNextQueueItem();
    void resetStreamState();
    QString getSystemPrompt() const;
    void cleanupScratchpad();

    QString m_lastRawRequest;
    QString m_lastRawResponse;

    // --- Collaborator (Item #5) ---
    // A dedicated secondary runner used for stateless collaborator queries.
    // Owned by AgentEngine; separate from m_runner to avoid interference.
    CliRunner* m_collaboratorRunner = nullptr;
    QString    m_collaboratorResponse;

    // --- Loop Detection (Item #13) ---
    QStringList m_lastToolResults;
    const int MAX_LOOP_RESULTS = 10;

    // --- Retry (#7): last executed call kept for transient-error retry ---
    ToolCall m_lastExecutedCall;
    /** Semantic fingerprint: "toolName:keyParam" pairs for each recent call. */
    QStringList m_lastToolCallFingerprints;

    // --- Circuit Breaker (#1) ---
    /** Iteration counter reset at the start of each user request. */
    int m_loopIterations = 0;
    static constexpr int MAX_LOOP_ITERATIONS = 12;

    /** Tracks consecutive exact repetitions for 'Hard Break' logic. */
    int m_consecutiveRepetitions = 0;

    // --- LLM Timeout (#2) + Adaptive Timeout (P-10) ---
    QTimer* m_llmTimeoutTimer = nullptr;
    static constexpr int LLM_TIMEOUT_DEFAULT_MS = 120'000; // 2 minutes
    int adaptiveLlmTimeout() const;
    QList<int> m_llmResponseTimesMs;             // rolling last 10 response times
    static constexpr int MAX_RESPONSE_HISTORY = 10;
    QElapsedTimer m_llmRequestTimer;             // measures current request duration

    // --- Collaborator Async (P-3) ---
    QTimer* m_collabTimeoutTimer = nullptr;
    ToolCall m_pendingCollabCall;                 // the AskAgent call awaiting response

    // --- Multi-Tool Batch Execution (P-1) ---
    void dispatchToolBatch(const QList<ToolCall>& calls);
    void onBatchToolFinished(const QString& toolName, const ToolResult& result);
    QList<ToolCall>   m_batchCalls;
    QList<ToolResult> m_batchResults;
    QMutex            m_batchMutex;
    std::atomic<int>  m_batchPending{0};

    // --- Role Pipeline (P-4) ---
    AgentPipeline* m_pipeline = nullptr;
    AgentGraph*    m_graph = nullptr;

    // --- Role auto-detection (#39): reset to Base after request completes ---
    bool m_roleWasAutoDetected = false;

    // --- Phase 2: Advanced Strategies ---
    QStringList m_activeTechniques;
    QMap<QString, QString> m_blackboard;
};

}  // namespace CodeHex