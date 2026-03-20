#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QDir>
#include <QDateTime>
#include <functional>

namespace CodeHex {

class ListDirectoryTool : public Tool {
public:
    QString name() const override { return "ListDirectory"; }
    QString description() const override { return "Lists directory contents with details (max depth 3)."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"path", QJsonObject{
                    {"type", "string"},
                    {"description", "Path to the directory to list"}
                }},
                {"depth", QJsonObject{
                    {"type", "integer"},
                    {"description", "Maximum depth to list (default: 3)"}
                }}
            }},
            {"required", QJsonArray{}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString rawPath = input["path"].toString();
        const QString root    = ToolUtils::resolvePath(rawPath.isEmpty() ? "." : rawPath, workDir);
        const int     maxDepth = input.contains("depth") ? input["depth"].toInt() : 3;

        if (!QDir(root).exists())
            return ToolUtils::errResult(QString("ListDirectory: path not found: %1").arg(root));

        if (!ToolUtils::isPathSafe(root, workDir))
            return ToolUtils::errResult(QString("ListDirectory: permission denied for path: %1").arg(root));

        static const QStringList kSkip = {
            ".git", "node_modules", "build", "target", "__pycache__", ".build",
            ".gradle", ".idea", ".vscode", "dist", "out"
        };

        QStringList lines;
        std::function<void(const QString&, int)> walk = [&](const QString& dir, int depth) {
            if (depth > maxDepth) return;
            const QString indent(static_cast<int>((depth - 1) * 2), ' ');
            const auto entries = QDir(dir).entryInfoList(
                QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            for (const QFileInfo& fi : entries) {
                if (fi.isDir() && kSkip.contains(fi.fileName())) continue;
                QString details;
                if (!fi.isDir()) {
                    const double kb = fi.size() / 1024.0;
                    details = QString(" (%1 KB, %2)").arg(kb, 0, 'f', 1).arg(fi.lastModified().toString("yyyy-MM-dd HH:mm"));
                }
                lines << indent + (fi.isDir() ? "[D] " : "    ") + fi.fileName() + details;
                if (fi.isDir()) walk(fi.filePath(), depth + 1);
            }
        };
        walk(root, 1);

        return ToolUtils::okResult(lines.isEmpty() ? "(empty directory)" : lines.join('\n'));
    }
};

} // namespace CodeHex
