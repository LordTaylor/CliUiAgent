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

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString path    = ToolUtils::resolvePath(input["path"].toString(), workDir);
        const QString pattern = input["pattern"].toString();
        const QString replacement = input["replacement"].toString();

        if (path.isEmpty() || pattern.isEmpty())
            return ToolUtils::errResult("Replace: 'path' and 'pattern' parameters are required");

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return ToolUtils::errResult(QString("Replace: cannot open '%1' for reading").arg(path));

        QString content = QString::fromLocal8Bit(file.readAll());
        file.close();

        QRegularExpression re(pattern);
        if (!re.isValid())
            return ToolUtils::errResult(QString("Replace: invalid regex pattern: %1").arg(re.errorString()));

        const QString newContent = content.replace(re, replacement);
        if (newContent == content)
            return ToolUtils::okResult("(no changes made, pattern not found)");

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return ToolUtils::errResult(QString("Replace: cannot open '%1' for writing").arg(path));

        file.write(newContent.toUtf8());
        return ToolUtils::okResult(QString("Successfully replaced occurrences in %1").arg(path));
    }
};

} // namespace CodeHex
