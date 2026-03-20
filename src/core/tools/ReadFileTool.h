#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QFile>

namespace CodeHex {

class ReadFileTool : public Tool {
public:
    QString name() const override { return "ReadFile"; }
    QString description() const override { return "Reads a file from the work directory (max 100 KB)."; }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString path = ToolUtils::resolvePath(input["path"].toString(), workDir);
        if (path.isEmpty())
            return ToolUtils::errResult("ReadFile: 'path' parameter is required");

        QFile file(path);
        if (!file.exists())
            return ToolUtils::errResult(QString("ReadFile: file not found: %1").arg(path));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return ToolUtils::errResult(QString("ReadFile: cannot open '%1': %2").arg(path, file.errorString()));

        constexpr qint64 kMaxBytes = 100 * 1024;  // 100 KB
        QByteArray data = file.read(kMaxBytes);
        const bool truncated = (file.bytesAvailable() > 0);

        QString content = QString::fromLocal8Bit(data);
        if (truncated)
            content += "\n\n[... file truncated at 100 KB ...]";
        return ToolUtils::okResult(content);
    }
};

} // namespace CodeHex
