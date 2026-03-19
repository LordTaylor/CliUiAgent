#pragma once
#include <QList>
#include <QObject>
#include <QString>
#include "../data/Attachment.h"
#include "../data/Message.h"

namespace CodeHex {

class CliRunner;
class SessionManager;
class ScriptManager;
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

private slots:
    void onOutputChunk(const QString& chunk);
    void onErrorChunk(const QString& chunk);
    void onRunnerFinished(int exitCode);

private:
    void buildAssistantMessage();

    AppConfig* m_config;
    SessionManager* m_sessions;
    CliRunner* m_runner;
    ScriptManager* m_scripts;

    QString m_currentResponse;
};

}  // namespace CodeHex
