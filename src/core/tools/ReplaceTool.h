#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include <QRegularExpression>
#include <QFile>

namespace CodeHex {

class ReplaceTool : public Tool {
public:
    QString name() const override { return "Replace"; }
    QString description() const override { return "Regex-based text replacement in a file."; }
    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"TargetFile", QJsonObject{
                    {"type", "string"},
                    {"description", "Path to the file to modify"}
                }},
                {"pattern", QJsonObject{
                    {"type", "string"},
                    {"description", "Regex pattern to match"}
                }},
                {"replacement", QJsonObject{
                    {"type", "string"},
                    {"description", "Replacement text"}
                }}
            }},
            {"required", QJsonArray{"TargetFile", "pattern", "replacement"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString path    = ToolUtils::resolvePath(input["TargetFile"].toString(), workDir);
        const QString pattern = input["pattern"].toString();
        const QString replacement = input["replacement"].toString();

        if (path.isEmpty() || pattern.isEmpty())
            return ToolUtils::errResult("Replace: 'TargetFile' and 'pattern' parameters are required");

        if (!ToolUtils::isPathSafe(path, workDir))
            return ToolUtils::errResult(QString("Replace: permission denied for path: %1").arg(path));

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return ToolUtils::errResult(QString("Replace: cannot open '%1' for reading").arg(path));

        QString content = QString::fromLocal8Bit(file.readAll());
        file.close();

        QRegularExpression re(pattern);
        if (!re.isValid())
            return ToolUtils::errResult(QString("Replace: invalid regex pattern: %1").arg(re.errorString()));

        const QString oldContent = content;
        content.replace(re, replacement);
        if (content == oldContent)
            return ToolUtils::okResult("(no changes made, pattern not found)");

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return ToolUtils::errResult(QString("Replace: cannot open '%1' for writing").arg(path));

        file.write(content.toUtf8());
        return ToolUtils::okResult(QString("Successfully replaced occurrences in %1").arg(path));
    }
};

} // namespace CodeHex
