#pragma once
#include <QList>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
#include <QStringDecoder>
#include <memory>
#include "CliProfile.h"
#include "../data/Message.h"
#include "../data/ToolCall.h"

namespace CodeHex {

class CliRunner : public QObject {
    Q_OBJECT
public:
    explicit CliRunner(QObject* parent = nullptr);
    ~CliRunner() override;

    void setProfile(std::unique_ptr<CliProfile> profile);
    CliProfile* profile() const;

    virtual void send(const QString& prompt,
              const QString& workDir         = {},
              const QStringList& imagePaths  = {},
              const QList<Message>& history  = {},
              const QString& systemPrompt    = {});

    virtual void sendJson(const QJsonObject& jsonRequest,
                        const QString& workDir = {});
    void stop();
    bool isRunning() const;
    bool isProfileRunning() const;

    void runSimpleCommand(const QString& command, const QString& workingDirectory);

signals:
    void outputChunk(const QString& chunk);
    void rawOutput(const QString& raw);
    void errorChunk(const QString& chunk);
    void finished(int exitCode);
    void started();
    void toolCallReady(const CodeHex::ToolCall& call);
    void tokenStats(int input, int output);
    void simpleCommandStarted();
    void simpleCommandFinished(int exitCode, const QString& output, const QString& errorOutput);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void retrySend();

private:
    void processChunk(const QString& chunk);

    QProcess m_process;
    std::unique_ptr<CliProfile> m_profile;
    QByteArray m_lineBuf;

    struct LastRequest {
        QString prompt;
        QString workDir;
        QStringList imagePaths;
        QList<Message> history;
        QString systemPrompt;
        bool isJson = false;
        QJsonObject jsonRequest;
    } m_lastRequest;

    int m_retryCount = 0;
    int m_maxRetries = 3;
    QTimer* m_backoffTimer = nullptr;

    QProcess m_simpleProcess;
    QByteArray m_simpleOutputBuf;
    QByteArray m_simpleErrorBuf;

    void startProcessInternal();
    QStringDecoder m_stdoutDecoder;

private slots: // New slots for simple commands
    void onSimpleReadyReadStdout();
    void onSimpleReadyReadStderr();
    void onSimpleProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onSimpleProcessError(QProcess::ProcessError error);
};

}  // namespace CodeHex