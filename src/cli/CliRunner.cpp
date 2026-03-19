#include "CliRunner.h"
#include <QDebug>

namespace CodeHex {

CliRunner::CliRunner(QObject* parent) : QObject(parent) {
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &CliRunner::onReadyReadStdout);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &CliRunner::onReadyReadStderr);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CliRunner::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &CliRunner::onProcessError);
}

CliRunner::~CliRunner() {
    stop();
}

void CliRunner::setProfile(std::unique_ptr<CliProfile> profile) {
    m_profile = std::move(profile);
}

CliProfile* CliRunner::profile() const {
    return m_profile.get();
}

void CliRunner::send(const QString& prompt, const QString& workDir) {
    if (!m_profile) {
        emit errorChunk("No CLI profile set.");
        return;
    }
    if (m_process.state() != QProcess::NotRunning) {
        stop();
    }

    if (!workDir.isEmpty()) {
        m_process.setWorkingDirectory(workDir);
    }

    const QStringList args = m_profile->buildArguments(prompt, workDir);
    m_process.start(m_profile->executable(), args);
    if (!m_process.waitForStarted(3000)) {
        emit errorChunk(QString("Failed to start '%1': %2")
                        .arg(m_profile->executable(), m_process.errorString()));
        return;
    }
    m_process.closeWriteChannel();
    emit started();
}

void CliRunner::stop() {
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(2000);
    }
}

bool CliRunner::isRunning() const {
    return m_process.state() != QProcess::NotRunning;
}

void CliRunner::onReadyReadStdout() {
    while (m_process.canReadLine()) {
        const QByteArray line = m_process.readLine();
        const QString parsed = m_profile ? m_profile->parseStreamChunk(line)
                                         : QString::fromUtf8(line);
        if (!parsed.isEmpty()) emit outputChunk(parsed);
    }
    // Also grab partial output (non-line-buffered CLIs)
    const QByteArray remaining = m_process.readAllStandardOutput();
    if (!remaining.isEmpty()) {
        const QString parsed = m_profile ? m_profile->parseStreamChunk(remaining)
                                         : QString::fromUtf8(remaining);
        if (!parsed.isEmpty()) emit outputChunk(parsed);
    }
}

void CliRunner::onReadyReadStderr() {
    const QString err = QString::fromUtf8(m_process.readAllStandardError());
    if (!err.isEmpty()) emit errorChunk(err);
}

void CliRunner::onProcessFinished(int exitCode, QProcess::ExitStatus /*status*/) {
    // Drain any remaining stdout
    onReadyReadStdout();
    emit finished(exitCode);
}

void CliRunner::onProcessError(QProcess::ProcessError error) {
    emit errorChunk(QString("Process error: %1").arg(static_cast<int>(error)));
}

}  // namespace CodeHex
