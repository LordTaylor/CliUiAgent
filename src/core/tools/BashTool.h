#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QProcess>
#include <atomic>

namespace CodeHex {

class BashTool : public Tool {
public:
    QString name() const override { return "Bash"; }
    QString description() const override { return "Runs a command via bash -c (default timeout: 30 s)."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"command", QJsonObject{
                    {"type", "string"},
                    {"description", "The shell command to execute"}
                }},
                {"timeout_ms", QJsonObject{
                    {"type", "integer"},
                    {"description", "Timeout in milliseconds (default: 30000)"}
                }}
            }},
            {"required", QJsonArray{"command"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString command = input["command"].toString();
        if (command.isEmpty())
            return ToolUtils::errResult("Bash: 'command' parameter is required");

        const int timeout = input.contains("timeout_ms") ? input["timeout_ms"].toInt() : 30000;

        QProcess* proc = new QProcess();
        m_activeProcess = proc;
        if (!workDir.isEmpty())
            proc->setWorkingDirectory(workDir);
        proc->start("bash", {"-c", command});

        if (!proc->waitForStarted(5000)) {
            ToolResult res = ToolUtils::errResult(QString("Bash: failed to start: %1").arg(proc->errorString()));
            delete proc;
            m_activeProcess = nullptr;
            return res;
        }

        if (!proc->waitForFinished(timeout)) {
            proc->kill();
            ToolResult res = ToolUtils::errResult(QString("Bash: timed out after %1 ms").arg(timeout));
            delete proc;
            m_activeProcess = nullptr;
            return res;
        }

        const QString out = QString::fromLocal8Bit(proc->readAllStandardOutput());
        const QString err = QString::fromLocal8Bit(proc->readAllStandardError());

        QString combined;
        if (!out.isEmpty()) combined += out;
        if (!err.isEmpty()) combined += (combined.isEmpty() ? "" : "\n[stderr]\n") + err;
        if (combined.isEmpty()) combined = "(no output)";

        bool isError = (proc->exitCode() != 0);
        
        delete proc;
        m_activeProcess = nullptr;

        if (isError)
            return ToolResult{ {}, combined, /*isError=*/true };

        return ToolUtils::okResult(combined);
    }

    void abort() override {
        QProcess* proc = m_activeProcess.load();
        if (proc) {
            proc->kill();
        }
    }

private:
    std::atomic<QProcess*> m_activeProcess{nullptr};
};

} // namespace CodeHex
