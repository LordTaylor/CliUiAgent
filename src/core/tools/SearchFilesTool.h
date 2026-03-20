#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QDirIterator>

namespace CodeHex {

class SearchFilesTool : public Tool {
public:
    QString name() const override { return "SearchFiles"; }
    QString description() const override { return "Glob search for files matching a pattern."; }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString pattern = input["pattern"].toString();
        if (pattern.isEmpty())
            return ToolUtils::errResult("SearchFiles: 'pattern' parameter is required");

        const QString rawRoot = input.contains("root") ? input["root"].toString() : ".";
        const QString root    = ToolUtils::resolvePath(rawRoot, workDir);

        QStringList matches;
        QDirIterator it(root, QStringList{pattern},
                        QDir::Files | QDir::NoSymLinks,
                        QDirIterator::Subdirectories);
        while (it.hasNext() && matches.size() < 100)
            matches << QDir(workDir).relativeFilePath(it.next());

        if (matches.isEmpty())
            return ToolUtils::okResult("(no files matched the pattern)");
        return ToolUtils::okResult(matches.join('\n'));
    }
};

} // namespace CodeHex
