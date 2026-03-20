#pragma once
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include "../Tool.h"
#include "ToolUtils.h"

namespace CodeHex {

/**
 * @brief Tool for precise text replacement using search and replace blocks.
 * Avoids the pitfalls of regex for complex code edits.
 */
class SearchReplaceTool : public Tool {
public:
    QString name() const override { return "SearchReplace"; }
    QString description() const override { return "Surgically replace a exact text block with another in a file."; }
    
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"path", QJsonObject{
                    {"type", "string"},
                    {"description", "Path to the file to modify"}
                }},
                {"search", QJsonObject{
                    {"type", "string"},
                    {"description", "The exact block of text to search for. Must be unique in the file."}
                }},
                {"replace", QJsonObject{
                    {"type", "string"},
                    {"description", "The replacement text block."}
                }}
            }},
            {"required", QJsonArray{"path", "search", "replace"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString path   = ToolUtils::resolvePath(input["path"].toString(), workDir);
        const QString search  = input["search"].toString();
        const QString replace = input["replace"].toString();

        if (path.isEmpty() || search.isEmpty())
            return ToolUtils::errResult("SearchReplace: 'path' and 'search' parameters are required");

        if (!ToolUtils::isPathSafe(path, workDir))
            return ToolUtils::errResult(QString("SearchReplace: permission denied for path: %1").arg(path));

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return ToolUtils::errResult(QString("SearchReplace: cannot open '%1' for reading").arg(path));

        QString content = QString::fromUtf8(file.readAll());
        file.close();

        // Count occurrences to ensure uniqueness
        int count = content.count(search);
        if (count == 0) {
            return ToolUtils::errResult("SearchReplace: the search block was not found in the file.");
        }
        if (count > 1) {
            return ToolUtils::errResult(QString("SearchReplace: the search block is not unique (%1 occurrences found). "
                                                "Provide more context in the search block.").arg(count));
        }

        content.replace(search, replace);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return ToolUtils::errResult(QString("SearchReplace: cannot open '%1' for writing").arg(path));

        file.write(content.toUtf8());
        return ToolUtils::okResult(QString("Successfully replaced block in %1").arg(path));
    }
};

} // namespace CodeHex
