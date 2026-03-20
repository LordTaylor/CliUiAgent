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

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString query = input["query"].toString();
        if (query.isEmpty()) return ToolUtils::errResult("Search: 'query' parameter is required");

        const QString root = ToolUtils::resolvePath(input.contains("root") ? input["root"].toString() : ".", workDir);
        const bool caseSensitive = input.contains("case_sensitive") ? input["case_sensitive"].toBool() : false;

        QStringList results;
        QDirIterator it(root, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (it.hasNext() && results.size() < 100) {
            const QString filePath = it.next();
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
