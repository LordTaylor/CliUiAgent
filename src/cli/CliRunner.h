#pragma once
#include <QObject>
#include <QProcess>
#include <memory>
#include "CliProfile.h"

namespace CodeHex {

class CliRunner : public QObject {
    Q_OBJECT
public:
    explicit CliRunner(QObject* parent = nullptr);
    ~CliRunner() override;

    void setProfile(std::unique_ptr<CliProfile> profile);
    CliProfile* profile() const;

    void send(const QString& prompt, const QString& workDir = {});
    void stop();
    bool isRunning() const;

signals:
    void outputChunk(const QString& chunk);
    void errorChunk(const QString& chunk);
    void finished(int exitCode);
    void started();

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess m_process;
    std::unique_ptr<CliProfile> m_profile;
};

}  // namespace CodeHex
