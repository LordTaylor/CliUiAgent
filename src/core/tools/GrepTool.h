#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QDirIterator>
#include <QTextStream>

namespace CodeHex {

class GrepTool : public Tool {
public:
    QString name() const override { return "Search"; }
    QString description() const override { return "Grep-like content search in files."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"query", QJsonObject{
                    {"type", "string"},
                    {"description", "Text pattern to search for"}
                }},
                {"DirectoryPath", QJsonObject{
                    {"type", "string"},
                    {"description", "Directory to search in (default: .)"}
                }},
                {"case_sensitive", QJsonObject{
                    {"type", "boolean"},
                    {"description", "Whether search is case-sensitive (default: false)"}
                }}
            }},
            {"required", QJsonArray{"query"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString query = input["query"].toString();
        if (query.isEmpty()) return ToolUtils::errResult("Search: 'query' parameter is required");

        const QString root = ToolUtils::resolvePath(input.contains("DirectoryPath") ? input["DirectoryPath"].toString() : ".", workDir);
        if (!ToolUtils::isPathSafe(root, workDir))
            return ToolUtils::errResult(QString("Search: permission denied for path: %1").arg(root));
        const bool caseSensitive = input.contains("case_sensitive") ? input["case_sensitive"].toBool() : false;

        QStringList results;
        QDirIterator it(root, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (it.hasNext() && results.size() < 100) {
            const QString filePath = it.next();
            const QString relPath = QDir(workDir).relativeFilePath(filePath);
            if (ToolUtils::isIgnored(relPath)) continue;

            QFileInfo info(filePath);
            if (info.size() > 1024 * 1024) continue; // skip files > 1MB

            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                const QByteArray firstBytes = file.read(512);
                if (firstBytes.contains('\0')) {
                    file.close();
                    continue;
                }
                file.seek(0);

                QTextStream stream(&file);
                int lineNum = 1;
                while (!stream.atEnd()) {
                    const QString line = stream.readLine();
                    if (line.contains(query, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                        const QString relPath = QDir(workDir).relativeFilePath(filePath);
                        results << QString("%1:%2: %3").arg(relPath).arg(lineNum).arg(line.trimmed());
                    }
                    lineNum++;
                }
            }
        }

        if (results.isEmpty()) return ToolUtils::okResult("(no matches found)");
        return ToolUtils::okResult(results.join('\n'));
    }
};

} // namespace CodeHex
