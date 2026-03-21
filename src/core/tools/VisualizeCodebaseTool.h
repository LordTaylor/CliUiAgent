#pragma once
#include "../Tool.h"
#include "CodebaseVisualizer.h"
#include <QJsonArray>
#include <QJsonObject>

namespace CodeHex {

/**
 * @brief Tool for generating a Mermaid diagram of the project structure.
 */
class VisualizeCodebaseTool : public Tool {
public:
    QString name() const override { return "VisualizeCodebase"; }
    QString description() const override {
        return "Generates a Mermaid diagram of the project's directory structure and class dependencies. "
               "Useful for understanding the overall architecture.";
    }

    QJsonObject parameters() const override {
        return {
            {"type", "object"},
            {"properties", QJsonObject{
                {"includeFiles", QJsonObject{
                    {"type", "boolean"},
                    {"description", "Whether to include individual files in the diagram (default: true)."}
                }}
            }}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        bool includeFiles = input.value("includeFiles").toBool(true);
        
        QString diagram = CodebaseVisualizer::generateMermaid(workDir, includeFiles);
        
        ToolResult res;
        res.content = "### CODEBASE VISUALIZATION (Mermaid):\n\n```mermaid\n" + diagram + "\n```\n";
        return res;
    }
};

} // namespace CodeHex
