#pragma once
#include <QList>
#include <QObject>
#include <QProcess>
#include <memory>
#include "CliProfile.h"
#include "../data/Message.h"
#include "../data/ToolCall.h" // Ensure this is included for ToolCall type

namespace CodeHex {

class CliRunner : public QObject {
    Q_OBJECT
public:
    explicit CliRunner(QObject* parent = nullptr);
    ~CliRunner() override;

    void setProfile(std::unique_ptr<CliProfile> profile);
    CliProfile* profile() const;

    // imagePaths : local file paths for image attachments; injected via
    //              CliProfile::imageArguments() before the -p prompt flag.
    // history    : full session message list (last entry == current prompt);
    //              passed to CliProfile::buildArguments() for multi-turn context.
    virtual void send(const QString& prompt,
              const QString& workDir         = {},
              const QStringList& imagePaths  = {},
              const QList<Message>& history  = {},
              const QString& systemPrompt    = {});
    void stop();
    bool isRunning() const;
    bool isProfileRunning() const;

    // New: for running simple bash commands
    void runSimpleCommand(const QString& command, const QString& workingDirectory);

signals:
    void outputChunk(const QString& chunk);       // parsed text token (to ChatView)
    void rawOutput(const QString& raw);            // unparsed line (to Console)
    void errorChunk(const QString& chunk);
    void finished(int exitCode);
    void started();
    // Emitted when a complete tool-use block has been parsed from the stream.
    // Claude CLI executes the tool internally; this signal is used for display
    // and for custom tool loops in non-Claude profiles.
    void toolCallReady(const CodeHex::ToolCall& call);

    // New signals for simple commands
    void simpleCommandStarted();
    void simpleCommandFinished(int exitCode, const QString& output, const QString& errorOutput);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    // Process a complete line: emit rawOutput + parsed outputChunk
    void processLine(const QByteArray& line);

    QProcess m_process;
    std::unique_ptr<CliProfile> m_profile;
    QByteArray m_lineBuf;  // accumulates bytes until a full newline arrives

    // For simple commands
    QProcess m_simpleProcess;
    QByteArray m_simpleOutputBuf;
    QByteArray m_simpleErrorBuf;

private slots: // New slots for simple commands
    void onSimpleReadyReadStdout();
    void onSimpleReadyReadStderr();
    void onSimpleProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onSimpleProcessError(QProcess::ProcessError error);
};

}  // namespace CodeHex