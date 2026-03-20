#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QProcess>

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

        QProcess proc;
        if (!workDir.isEmpty())
            proc.setWorkingDirectory(workDir);
        proc.start("bash", {"-c", command});

        if (!proc.waitForStarted(5000))
            return ToolUtils::errResult(QString("Bash: failed to start: %1").arg(proc.errorString()));

        if (!proc.waitForFinished(timeout)) {
            proc.kill();
            return ToolUtils::errResult(QString("Bash: timed out after %1 ms").arg(timeout));
        }

        const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
        const QString err = QString::fromLocal8Bit(proc.readAllStandardError());

        QString combined;
        if (!out.isEmpty()) combined += out;
        if (!err.isEmpty()) combined += (combined.isEmpty() ? "" : "\n[stderr]\n") + err;
        if (combined.isEmpty()) combined = "(no output)";

        if (proc.exitCode() != 0)
            return ToolResult{ {}, combined, /*isError=*/true };

        return ToolUtils::okResult(combined);
    }
};

} // namespace CodeHex
