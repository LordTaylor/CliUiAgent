#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QDirIterator>

namespace CodeHex {

class SearchFilesTool : public Tool {
public:
    QString name() const override { return "SearchFiles"; }
    QString description() const override { return "Glob search for files matching a pattern."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"pattern", QJsonObject{
                    {"type", "string"},
                    {"description", "Glob pattern (e.g. *.cpp)"}
                }},
                {"DirectoryPath", QJsonObject{
                    {"type", "string"},
                    {"description", "Directory to search in (default: .)"}
                }}
            }},
            {"required", QJsonArray{"pattern"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString pattern = input["pattern"].toString();
        if (pattern.isEmpty())
            return ToolUtils::errResult("SearchFiles: 'pattern' parameter is required");

        const QString rawRoot = input.contains("DirectoryPath") ? input["DirectoryPath"].toString() : ".";
        const QString root    = ToolUtils::resolvePath(rawRoot, workDir);

        if (!ToolUtils::isPathSafe(root, workDir))
            return ToolUtils::errResult(QString("SearchFiles: permission denied for path: %1").arg(root));

        QStringList matches;
        QDirIterator it(root, QStringList{pattern},
                        QDir::Files | QDir::NoSymLinks,
                        QDirIterator::Subdirectories);
        while (it.hasNext() && matches.size() < 100) {
            const QString filePath = it.next();
            const QString relPath = QDir(workDir).relativeFilePath(filePath);
            if (ToolUtils::isIgnored(relPath)) continue;
            matches << relPath;
        }

        if (matches.isEmpty())
            return ToolUtils::okResult("(no files matched the pattern)");
        return ToolUtils::okResult(matches.join('\n'));
    }
};

} // namespace CodeHex
