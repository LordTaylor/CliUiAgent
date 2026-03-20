#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace CodeHex {

class WriteFileTool : public Tool {
public:
    QString name() const override { return "WriteFile"; }
    QString description() const override { return "Writes content to a file in the work directory."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"path", QJsonObject{
                    {"type", "string"},
                    {"description", "Path to the file to write"}
                }},
                {"content", QJsonObject{
                    {"type", "string"},
                    {"description", "Full content to write to the file"}
                }}
            }},
            {"required", QJsonArray{"path", "content"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString path    = ToolUtils::resolvePath(input["path"].toString(), workDir);
        const QString content = input["content"].toString();
        if (path.isEmpty())
            return ToolUtils::errResult("WriteFile: 'path' parameter is required");
        if (!ToolUtils::isPathSafe(path, workDir))
            return ToolUtils::errResult(QString("WriteFile: permission denied for path: %1").arg(path));

        // Create parent directories if they don't exist
        const QDir dir = QFileInfo(path).dir();
        if (!dir.exists() && !QDir().mkpath(dir.path()))
            return ToolUtils::errResult(QString("WriteFile: cannot create directory '%1'").arg(dir.path()));

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return ToolUtils::errResult(QString("WriteFile: cannot open '%1' for writing: %2")
                             .arg(path, file.errorString()));

        file.write(content.toUtf8());
        return ToolUtils::okResult(QString("Written %1 bytes to %2").arg(content.toUtf8().size()).arg(path));
    }
};

} // namespace CodeHex
