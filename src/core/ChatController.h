#pragma once
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QRegularExpression> // Added
#include <QString>
#include "../data/Attachment.h"
#include "../data/Message.h"
#include "../data/ToolCall.h"
#include "ToolCall.h"

namespace CodeHex {

class CliRunner;
class SessionManager;
class ScriptManager;
class ToolExecutor;
class AgentEngine;
class AppConfig;

class ChatController : public QObject {
    Q_OBJECT
public:
    explicit ChatController(AppConfig* config,
                            SessionManager* sessions,
                            CliRunner* runner,
                            ScriptManager* scripts,
                            QObject* parent = nullptr);

public slots:
    void sendMessage(const QString& text, const QList<Attachment>& attachments = {});
    void stopGeneration();
    void resetAgent();
    bool isRunning() const;
    void setManualApproval(bool enabled);
    void approveToolCall(const CodeHex::ToolCall& call);
    void setSelectedModel(const QString& model);
    void onProviderChanged();
    AgentEngine* agent() const { return m_agent; }

signals:
    void userMessageReady(const Message& msg);
    void tokenReceived(const QString& token);
    void tokenStatsUpdated(int input, int output);
    void responseComplete(const Message& msg);
    void errorOccurred(const QString& error);
    void consoleOutput(const QString& raw);
    void cliOutputReceived(const QString& text);
    void cliErrorReceived(const QString& text);
    void generationStarted();
    void generationStopped();
    void statusChanged(const QString& status);
    
    // UI-specific signals (re-emitted from AgentEngine)
    void toolCallStarted(const QString& toolName, const QJsonObject& input);
    void toolApprovalRequested(const CodeHex::ToolCall& call);
    
    void sessionRenamed(const QString& sessionId, const QString& newTitle);

private slots:
    void onOutputChunk(const QString& chunk);
    void onRawOutput(const QString& raw);
    void onErrorChunk(const QString& chunk);
    void onRunnerFinished(int exitCode);
    void onToolCallReady(const CodeHex::ToolCall& call);
    // New slot for simple command results
    void onSimpleCommandFinished(int exitCode, const QString& output, const QString& errorOutput);
    // New slot for ToolExecutor results
    void onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result); // Modified signature

private:
    void buildAssistantMessage(const QList<CodeBlock>& contentBlocks,
                               const QList<Message::ContentType>& contentTypes,
                               const QString& plainText); // Modified signature
    void executeBashCommand(const QString& command); // New: for executing simple bash commands
    void onToolCallReadyAndExecute(const CodeHex::ToolCall& call); // New: intermediate slot for ToolExecutor
    static QString formatToolCallLog(const ToolCall& call);

    AppConfig*      m_config;
    SessionManager* m_sessions;
    CliRunner*      m_runner;
    ScriptManager*  m_scripts;
    ToolExecutor*   m_toolExecutor;
    AgentEngine*    m_agent;

    QString m_currentResponse;

    // Added empty comment to trigger MOC regeneration
};

}  // namespace CodeHex