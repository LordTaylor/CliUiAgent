#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QFile>

namespace CodeHex {

class ReadFileTool : public Tool {
public:
    QString name() const override { return "ReadFile"; }
    QString description() const override { return "Reads a file from the work directory (max 100 KB)."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"path", QJsonObject{
                    {"type", "string"},
                    {"description", "Path to the file to read"}
                }}
            }},
            {"required", QJsonArray{"path"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString path = ToolUtils::resolvePath(input["path"].toString(), workDir);
        if (path.isEmpty())
            return ToolUtils::errResult("ReadFile: 'path' parameter is required");
        if (!ToolUtils::isPathSafe(path, workDir))
            return ToolUtils::errResult(QString("ReadFile: permission denied for path: %1").arg(path));

        QFile file(path);
        if (!file.exists())
            return ToolUtils::errResult(QString("ReadFile: file not found: %1").arg(path));
        if (!file.open(QIODevice::ReadOnly))
            return ToolUtils::errResult(QString("ReadFile: cannot open '%1': %2").arg(path, file.errorString()));

        constexpr qint64 kMaxBytes = 100 * 1024;  // 100 KB
        qint64 size = file.size();
        qint64 mapSize = std::min(size, kMaxBytes);

        QString content;
        if (mapSize > 0) {
            uchar* memory = file.map(0, mapSize);
            if (memory) {
                // Item 52: Use mmap for file reading
                content = QString::fromLocal8Bit(reinterpret_cast<const char*>(memory), mapSize);
                file.unmap(memory);
            } else {
                // Fallback to standard read
                QByteArray data = file.read(mapSize);
                content = QString::fromLocal8Bit(data);
            }
        }

        if (size > kMaxBytes)
            content += "\n\n[... file truncated at 100 KB ...]";
        return ToolUtils::okResult(content);
    }
};

} // namespace CodeHex
