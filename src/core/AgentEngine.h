#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QQueue>
#include "Message.h"
#include "ToolCall.h"
#include "Attachment.h"
#include "AgentRole.h" // Added

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

    void setRole(AgentRole role) { m_currentRole = role; }
    AgentRole currentRole() const { return m_currentRole; }

    void setSelectedModel(const QString& model) { m_selectedModel = model; }
    QString selectedModel() const { return m_selectedModel; }

    /**
     * @brief Saves the last raw request, response, and session state to a directory.
     */
    void saveDebugLog(const QString& targetDir);
    
    // CoVe State Machine
    enum class CoVeState { None, Drafting, VerifyingQuestions, Answering, Finalizing };

signals:
    void statusChanged(const QString& status);
    void tokenReceived(const QString& token);
    void tokenStatsUpdated(int input, int output);
    void consoleOutput(const QString& raw);
    void toolCallStarted(const QString& toolName, const QJsonObject& input);
    void toolApprovalRequested(const QString& toolName, const QJsonObject& input);
    void responseComplete(const Message& msg);
    void errorOccurred(const QString& error);

public slots:
    void onOutputChunk(const QString& chunk);
    void onRawOutput(const QString& raw);
    void onErrorChunk(const QString& chunk);
    void onToolCallReady(const CodeHex::ToolCall& call);
    void onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result);
    void onRunnerFinished(int exitCode);
    void onAuditSuggestion(const QString& suggestion);

private:
    void runLoop(const QString& prompt, const QStringList& imagePaths);
    void buildAssistantMessage(const ResponseParser::ParseResult& result);

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
};

}  // namespace CodeHex