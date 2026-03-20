#include "CliRunner.h"
#include <QDebug>
#include <QDir> // Added for QDir::homePath()
#include <QJsonDocument>
#include <QRegularExpression>
//#include <QTextCodec> // Removed
#include "CliProfile.h"
#include "../data/ToolCall.h" // Ensure this is included for ToolCall type

#include <QJsonDocument>
#include <QJsonObject>

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

    // New connections for simple process
    connect(&m_simpleProcess, &QProcess::readyReadStandardOutput,
            this, &CliRunner::onSimpleReadyReadStdout);
    connect(&m_simpleProcess, &QProcess::readyReadStandardError,
            this, &CliRunner::onSimpleReadyReadStderr);
    connect(&m_simpleProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CliRunner::onSimpleProcessFinished);
    connect(&m_simpleProcess, &QProcess::errorOccurred,
            this, &CliRunner::onSimpleProcessError);
}

CliRunner::~CliRunner() {
    stop();
    m_simpleProcess.terminate(); // Terminate simple process as well
    m_simpleProcess.waitForFinished(100);
}

void CliRunner::setProfile(std::unique_ptr<CliProfile> profile) {
    m_profile = std::move(profile);
}

CliProfile* CliRunner::profile() const {
    return m_profile.get();
}

void CliRunner::send(const QString& prompt,
                      const QString& workDir,
                      const QStringList& imagePaths,
                      const QList<Message>& history,
                      const QString& systemPrompt) {
    if (!m_profile) {
        qWarning() << "CliRunner: no profile set";
        emit errorChunk("Error: No AI profile selected.");
        emit finished(1);
        return;
    }

    if (isRunning()) {
        qWarning() << "CliRunner: already running";
        emit errorChunk("Error: Already generating a response.");
        emit finished(1);
        return;
    }

    // Stop m_simpleProcess if it's running
    if (m_simpleProcess.state() == QProcess::Running) {
        m_simpleProcess.terminate();
        m_simpleProcess.waitForFinished(1000);
    }

    // Apply any extra environment variables declared by the profile.
    const auto extra = m_profile->extraEnvironment();
    if (!extra.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (auto it = extra.cbegin(); it != extra.cend(); ++it)
            env.insert(it.key(), it.value());
        m_process.setProcessEnvironment(env);
    }

    // Build final argument list:
    // Strategy: find -p in baseArgs and splice imgArgs just before it.
    QStringList baseArgs = m_profile->buildArguments(prompt, workDir, history, systemPrompt);
    if (!imagePaths.isEmpty()) {
        const QStringList imgArgs = m_profile->imageArguments(imagePaths);
        if (!imgArgs.isEmpty()) {
            const int pIdx = baseArgs.indexOf("-p");
            if (pIdx >= 0) {
                for (int i = imgArgs.size() - 1; i >= 0; --i)
                    baseArgs.insert(pIdx, imgArgs[i]);
            } else {
                baseArgs << imgArgs;  // fallback: append
            }
        }
    }

    m_process.setProgram(m_profile->executable()); // Corrected cliPath to executable
    m_process.setArguments(baseArgs); // Use baseArgs here
    m_process.setWorkingDirectory(workDir);

    m_lineBuf.clear();
    m_process.start();

    if (!m_process.waitForStarted()) {
        qWarning() << "CliRunner: failed to start process:" << m_process.errorString();
        emit errorChunk(QString("Error: Failed to start CLI process: %1").arg(m_process.errorString()));
        emit finished(1);
        return;
    }
    emit started();
}

void CliRunner::stop() {
    if (m_process.state() == QProcess::Running) {
        m_process.terminate();
        m_process.waitForFinished(1000); // Wait up to 1 second
    }
    if (m_simpleProcess.state() == QProcess::Running) {
        m_simpleProcess.terminate();
        m_simpleProcess.waitForFinished(1000);
    }
}

bool CliRunner::isRunning() const {
    return m_process.state() != QProcess::NotRunning || m_simpleProcess.state() != QProcess::NotRunning;
}

bool CliRunner::isProfileRunning() const {
    return m_process.state() != QProcess::NotRunning;
}

// New: for running simple bash commands
void CliRunner::runSimpleCommand(const QString& command, const QString& workingDirectory) {
    if (m_simpleProcess.state() == QProcess::Running || m_process.state() == QProcess::Running) {
        qWarning() << "CliRunner: a command is already running (simple or profile-based)";
        emit simpleCommandFinished(1, "", "Error: Another command is already running.");
        return;
    }

    m_simpleOutputBuf.clear();
    m_simpleErrorBuf.clear();

    m_simpleProcess.setProgram("bash");
    m_simpleProcess.setArguments({"-c", command});
    if (!workingDirectory.isEmpty()) {
        m_simpleProcess.setWorkingDirectory(workingDirectory);
    } else {
        m_simpleProcess.setWorkingDirectory(QDir::homePath()); // Default to home if not specified
    }

    m_simpleProcess.start();

    if (!m_simpleProcess.waitForStarted()) {
        qWarning() << "CliRunner: failed to start simple process:" << m_simpleProcess.errorString();
        emit simpleCommandFinished(1, "", QString("Error: Failed to start simple CLI process: %1").arg(m_simpleProcess.errorString()));
        return;
    }
    emit simpleCommandStarted();
}

void CliRunner::onReadyReadStdout() {
    m_lineBuf += m_process.readAllStandardOutput();
    int start = 0;
    while (true) {
        const int nl = m_lineBuf.indexOf('\n', start);
        if (nl < 0) break;
        processLine(m_lineBuf.mid(start, nl - start + 1));
        start = nl + 1;
    }
    m_lineBuf = m_lineBuf.mid(start);
}

void CliRunner::onReadyReadStderr() {
    const QString err = QString::fromLocal8Bit(m_process.readAllStandardError());
    if (!err.isEmpty()) emit errorChunk(err);
}

void CliRunner::onProcessFinished(int exitCode, QProcess::ExitStatus /*status*/) {
    onReadyReadStdout(); // Drain any remaining stdout
    if (!m_lineBuf.isEmpty()) { // Flush any leftover bytes
        processLine(m_lineBuf);
        m_lineBuf.clear();
    }
    emit finished(exitCode);
}

void CliRunner::onProcessError(QProcess::ProcessError error) {
    emit errorChunk(QString("Process error: %1").arg(static_cast<int>(error)));
}

void CliRunner::processLine(const QByteArray& line) {
    // Suppressed raw output to terminal for cleaner experience (JSON protocol hidden)
    // emit rawOutput(QString::fromLocal8Bit(line));

    if (!m_profile) {
        emit outputChunk(QString::fromLocal8Bit(line));
        return;
    }

    StreamResult res = m_profile->parseLine(line);

    // 1. Text streaming
    if (!res.textToken.isEmpty()) {
        emit outputChunk(res.textToken);
    }

    // 2. Tool calls (now handled by profile parser)
    if (res.toolCall) {
        emit toolCallReady(*res.toolCall);
    }

    // 3. Token usage
    if (res.inputTokens || res.outputTokens) {
        emit tokenStats(res.inputTokens.value_or(0), res.outputTokens.value_or(0));
    }
}

void CliRunner::sendJson(const QJsonObject& jsonRequest, const QString& workDir) {
    if (!m_profile) {
        emit errorChunk("Error: No AI profile selected.");
        emit finished(1);
        return;
    }

    if (isRunning()) {
        emit errorChunk("Error: Already generating a response.");
        emit finished(1);
        return;
    }

    // Default implementation: serialize to string and treat as prompt
    // Profiles can be updated later to handle JSON natively via specific flags
    QJsonDocument doc(jsonRequest);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    
    // For now, reuse the standard send mechanism but passing the serialized JSON as prompt
    send(jsonStr, workDir, {}, {}, "");
}

void CliRunner::onSimpleReadyReadStdout() {
    m_simpleOutputBuf.append(m_simpleProcess.readAllStandardOutput());
}

void CliRunner::onSimpleReadyReadStderr() {
    m_simpleErrorBuf.append(m_simpleProcess.readAllStandardError());
}

void CliRunner::onSimpleProcessFinished(int exitCode, QProcess::ExitStatus status) {
    Q_UNUSED(status);
    emit simpleCommandFinished(exitCode, QString::fromLocal8Bit(m_simpleOutputBuf), QString::fromLocal8Bit(m_simpleErrorBuf));
    m_simpleOutputBuf.clear();
    m_simpleErrorBuf.clear();
}

void CliRunner::onSimpleProcessError(QProcess::ProcessError error) {
    qWarning() << "CliRunner: simple process error:" << error << m_simpleProcess.errorString();
    emit simpleCommandFinished(1, "", QString("Simple CLI process error: %1").arg(m_simpleProcess.errorString()));
    m_simpleOutputBuf.clear();
    m_simpleErrorBuf.clear();
}

}  // namespace CodeHex