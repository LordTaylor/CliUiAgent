#pragma once
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QRegularExpression> // Added
#include <QString>
#include "../data/Attachment.h"
#include "../data/CodeBlock.h" // Added
#include "../data/Message.h"
#include "../data/ToolCall.h"

namespace CodeHex {

class CliRunner;
class SessionManager;
class ScriptManager;
class ToolExecutor;
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

signals:
    void userMessageReady(const Message& msg);
    void tokenReceived(const QString& token);
    void responseComplete(const Message& msg);
    void errorOccurred(const QString& error);
    void consoleOutput(const QString& raw);
    void generationStarted();
    void generationStopped();

    // Emitted when a tool_use block is fully parsed from the stream.
    // For Claude CLI profiles the CLI executes the tool itself; this signal
    // is used to update the UI and console display.
    void toolCallStarted(const QString& toolName, const QJsonObject& input);

    // Emitted after first assistant response renames the session automatically.
    void sessionRenamed(const QString& sessionId, const QString& newTitle);

private slots:
    void onOutputChunk(const QString& chunk);
    void onRawOutput(const QString& raw);
    void onErrorChunk(const QString& chunk);
    void onRunnerFinished(int exitCode);
    void onToolCallReady(const CodeHex::ToolCall& call);
    // New slot for simple command results
    void onSimpleCommandFinished(int exitCode, const QString& output, const QString& errorOutput);

private:
    void buildAssistantMessage(const QList<CodeBlock>& contentBlocks,
                               const QList<Message::ContentType>& contentTypes,
                               const QString& plainText); // Modified signature
    void executeBashCommand(const QString& command); // New: for executing simple bash commands
    static QString formatToolCallLog(const ToolCall& call);

    AppConfig*      m_config;
    SessionManager* m_sessions;
    CliRunner*      m_runner;
    ScriptManager*  m_scripts;
    ToolExecutor*   m_toolExecutor;  // available for custom (non-Claude) tool loops

    QString m_currentResponse;
};

}  // namespace CodeHex