#include "CliRunner.h"
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStandardPaths>
#include "CliProfile.h"
#include "../data/ToolCall.h"
#include <QJsonObject>
#include <cmath>

namespace CodeHex {

CliRunner::CliRunner(QObject* parent) 
    : QObject(parent), m_stdoutDecoder(QStringDecoder::Utf8) {
    m_backoffTimer = new QTimer(this);
    m_backoffTimer->setSingleShot(true);
    connect(m_backoffTimer, &QTimer::timeout, this, &CliRunner::retrySend);

    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &CliRunner::onReadyReadStdout);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &CliRunner::onReadyReadStderr);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CliRunner::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &CliRunner::onProcessError);

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
    m_simpleProcess.terminate();
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
        emit errorChunk("Error: No AI profile selected.");
        emit finished(1);
        return;
    }

    if (isRunning()) {
        emit errorChunk("Error: Already generating a response.");
        emit finished(1);
        return;
    }

    m_retryCount = 0;
    m_lastRequest.prompt = prompt;
    m_lastRequest.workDir = workDir;
    m_lastRequest.imagePaths = imagePaths;
    m_lastRequest.history = history;
    m_lastRequest.systemPrompt = systemPrompt;
    m_lastRequest.isJson = false;

    startProcessInternal();
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

    m_retryCount = 0;
    m_lastRequest.isJson = true;
    m_lastRequest.jsonRequest = jsonRequest;
    m_lastRequest.workDir = workDir;

    startProcessInternal();
}

void CliRunner::startProcessInternal() {
    if (m_simpleProcess.state() == QProcess::Running) {
        m_simpleProcess.terminate();
        m_simpleProcess.waitForFinished(1000);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList commonPaths = {"/usr/local/bin", "/usr/bin", "/bin", "/usr/sbin", "/sbin", "/opt/homebrew/bin"};
    for (const QString& cp : commonPaths) {
        if (!path.contains(cp)) {
            if (!path.isEmpty()) path += ":";
            path += cp;
        }
    }
    env.insert("PATH", path);
    
    // Also set some common environment variables for better compatibility
    if (!env.contains("TERM")) env.insert("TERM", "xterm-256color");
    if (!env.contains("LANG")) env.insert("LANG", "en_US.UTF-8");

    const auto extra = m_profile->extraEnvironment();
    if (!extra.isEmpty()) {
        for (auto it = extra.cbegin(); it != extra.cend(); ++it)
            env.insert(it.key(), it.value());
    }
    m_process.setProcessEnvironment(env);

    QString promptToUse = m_lastRequest.prompt;
    if (m_lastRequest.isJson) {
        QJsonDocument doc(m_lastRequest.jsonRequest);
        promptToUse = doc.toJson(QJsonDocument::Compact);
    }

    QStringList baseArgs = m_profile->buildArguments(promptToUse, m_lastRequest.workDir, m_lastRequest.history, m_lastRequest.systemPrompt);
    if (!m_lastRequest.imagePaths.isEmpty()) {
        const QStringList imgArgs = m_profile->imageArguments(m_lastRequest.imagePaths);
        if (!imgArgs.isEmpty()) {
            const int pIdx = baseArgs.indexOf("-p");
            if (pIdx >= 0) {
                for (int i = imgArgs.size() - 1; i >= 0; --i)
                    baseArgs.insert(pIdx, imgArgs[i]);
            } else {
                baseArgs << imgArgs;
            }
        }
    }

    QString program = m_profile->executable();
    // Try to resolve absolute path if it's just a name
    if (!program.contains('/')) {
        QString fullPath = QStandardPaths::findExecutable(program);
        if (!fullPath.isEmpty()) {
            program = fullPath;
        }
    }
    m_process.setProgram(program);
    m_process.setArguments(baseArgs);
    
    QString workDir = m_lastRequest.workDir;
    if (workDir.isEmpty() || !QDir(workDir).exists()) {
        workDir = QDir::currentPath();
    }
    m_process.setWorkingDirectory(workDir);

    m_lineBuf.clear();
    m_stdoutDecoder = QStringDecoder(QStringDecoder::Utf8); // Reset state
    m_process.start();

    if (!m_process.waitForStarted()) {
        qWarning() << "CliRunner: failed to start process:" << m_process.errorString() << "Program:" << program;
        emit errorChunk(QString("Error: Failed to start CLI process (%1): %2").arg(program).arg(m_process.errorString()));
        emit finished(1);
        return;
    }
    emit started();
}

void CliRunner::retrySend() {
    startProcessInternal();
}

void CliRunner::stop() {
    m_backoffTimer->stop();
    if (m_process.state() == QProcess::Running) {
        m_process.terminate();
        m_process.waitForFinished(1000);
    }
    if (m_simpleProcess.state() == QProcess::Running) {
        m_simpleProcess.terminate();
        m_simpleProcess.waitForFinished(1000);
    }
}

bool CliRunner::isRunning() const {
    return m_process.state() != QProcess::NotRunning || m_simpleProcess.state() != QProcess::NotRunning || m_backoffTimer->isActive();
}

bool CliRunner::isProfileRunning() const {
    return m_process.state() != QProcess::NotRunning || m_backoffTimer->isActive();
}

void CliRunner::runSimpleCommand(const QString& command, const QString& workingDirectory) {
    if (m_simpleProcess.state() == QProcess::Running || m_process.state() == QProcess::Running) {
        emit simpleCommandFinished(1, "", "Error: Another command is already running.");
        return;
    }

    m_simpleOutputBuf.clear();
    m_simpleErrorBuf.clear();

    m_simpleProcess.setProgram("bash");
    m_simpleProcess.setArguments({"-c", command});
    m_simpleProcess.setWorkingDirectory(workingDirectory.isEmpty() ? QDir::homePath() : workingDirectory);

    m_simpleProcess.start();
    if (!m_simpleProcess.waitForStarted()) {
        emit simpleCommandFinished(1, "", QString("Error: Failed to start simple CLI process: %1").arg(m_simpleProcess.errorString()));
        return;
    }
    emit simpleCommandStarted();
}

void CliRunner::onReadyReadStdout() {
    QByteArray raw = m_process.readAllStandardOutput();
    QString decoded = m_stdoutDecoder(raw);
    
    if (decoded.isEmpty()) return;
    processChunk(decoded);
}

void CliRunner::onReadyReadStderr() {
    const QString err = QString::fromLocal8Bit(m_process.readAllStandardError());
    if (!err.isEmpty()) emit errorChunk(err);
}

void CliRunner::onProcessFinished(int exitCode, QProcess::ExitStatus) {
    onReadyReadStdout();

    if (exitCode != 0 && m_retryCount < m_maxRetries) {
        bool retryable = (exitCode == 429 || exitCode == 503 || exitCode == 1);
        if (retryable) {
            m_retryCount++;
            int delayMs = (int)std::pow(2, m_retryCount) * 1000;
            emit errorChunk(QString("\n[RETRY] Process failed (exit %1). Retrying in %2ms (Attempt %3/%4)...")
                            .arg(exitCode).arg(delayMs).arg(m_retryCount).arg(m_maxRetries));
            m_backoffTimer->start(delayMs);
            return;
        }
    }

    emit finished(exitCode);
}

void CliRunner::onProcessError(QProcess::ProcessError error) {
    emit errorChunk(QString("Process error: %1").arg(static_cast<int>(error)));
}

void CliRunner::processChunk(const QString& chunk) {
    if (!m_profile) {
        emit outputChunk(chunk);
        return;
    }
    StreamResult res = m_profile->parseLine(chunk.toUtf8());
    if (!res.textToken.isEmpty()) emit outputChunk(res.textToken);
    if (res.toolCall) emit toolCallReady(*res.toolCall);
    if (res.inputTokens || res.outputTokens) {
        emit tokenStats(res.inputTokens.value_or(0), res.outputTokens.value_or(0));
    }
}

void CliRunner::onSimpleReadyReadStdout() {
    m_simpleOutputBuf.append(m_simpleProcess.readAllStandardOutput());
}

void CliRunner::onSimpleReadyReadStderr() {
    m_simpleErrorBuf.append(m_simpleProcess.readAllStandardError());
}

void CliRunner::onSimpleProcessFinished(int exitCode, QProcess::ExitStatus) {
    emit simpleCommandFinished(exitCode, QString::fromLocal8Bit(m_simpleOutputBuf), QString::fromLocal8Bit(m_simpleErrorBuf));
    m_simpleOutputBuf.clear();
    m_simpleErrorBuf.clear();
}

void CliRunner::onSimpleProcessError(QProcess::ProcessError error) {
    emit simpleCommandFinished(1, "", QString("Simple CLI process error: %1").arg(m_simpleProcess.errorString()));
    m_simpleOutputBuf.clear();
    m_simpleErrorBuf.clear();
}

} // namespace CodeHex