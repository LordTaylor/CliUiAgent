#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include "../data/Message.h"
#include "../data/ToolCall.h"
#include "../data/Attachment.h"

namespace CodeHex {

class CliRunner;
class ToolExecutor;
class SessionManager;
class AppConfig;

/**
 * @brief The AgentEngine class encapsulates the core "thinking" loop of the agent.
 * 
 * It coordinates between the LLM (CliRunner) and the ToolExecutor.
 * It manages the recursive tool call loop and maintains the conversation state.
 */
class AgentEngine : public QObject {
    Q_OBJECT
public:
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

    bool isRunning() const;

    void setManualApproval(bool enabled) { m_manualApproval = enabled; }
    void approveToolCall(const ToolCall& call);

signals:
    void statusChanged(const QString& status);
    void tokenReceived(const QString& token);
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

private:
    void runLoop(const QString& prompt, const QStringList& imagePaths);
    void buildAssistantMessage(const QString& plainText);

    AppConfig*      m_config;
    SessionManager* m_sessions;
    CliRunner*      m_runner;
    ToolExecutor*   m_toolExecutor;

    bool m_manualApproval = false;
    QString m_currentResponse;
    QList<ToolCall> m_pendingCalls;
};

} // namespace CodeHex
