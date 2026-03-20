#pragma once
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include "../data/Attachment.h"
#include "../data/Message.h"
#include "../data/ToolCall.h"

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
    void toolApprovalRequested(const QString& toolName, const QJsonObject& input);
    
    void sessionRenamed(const QString& sessionId, const QString& newTitle);

private:
    AppConfig*      m_config;
    SessionManager* m_sessions;
    CliRunner*      m_runner;
    ScriptManager*  m_scripts;
    ToolExecutor*   m_toolExecutor;
    AgentEngine*    m_agent;
};

}  // namespace CodeHex