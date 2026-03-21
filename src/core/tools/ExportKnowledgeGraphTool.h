#pragma once
#include "../Tool.h"
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>
#include "AppConfig.h"

namespace CodeHex {

/**
 * @brief Tool for Roadmap Functionality #17: Knowledge Graph.
 * Generates a Mermaid graph of project structure (classes and relationships).
 */
class ExportKnowledgeGraphTool : public Tool {
public:
    explicit ExportKnowledgeGraphTool(AppConfig* config) : m_config(config) {}

    QString name() const override { return "ExportKnowledgeGraph"; }

    QString description() const override {
        return "Generates a Mermaid class diagram representing the project's knowledge graph. "
               "Analyzes headers to find class inheritance and composition.";
    }

    QJsonObject parameters() const override {
        QJsonObject obj;
        obj["type"] = "object";
        QJsonObject props;
        QJsonObject path;
        path["type"] = "string";
        path["description"] = "Directory to analyze (default: project root/src).";
        props["path"] = path;
        obj["properties"] = props;
        return obj;
    }

    ToolResult execute(const QJsonObject& input, const QString& /*context*/) override {
        QString relPath = input["path"].toString();
        QString fullPath = m_config->workingFolder();
        if (!relPath.isEmpty()) {
            fullPath = QDir(fullPath).filePath(relPath);
        } else {
            fullPath = QDir(fullPath).filePath("src");
        }

        if (!QDir(fullPath).exists()) {
            return { "ExportKnowledgeGraph", "Error: Path not found: " + fullPath, true };
        }

        QString graph = "classDiagram\n";
        QDirIterator it(fullPath, QStringList() << "*.h" << "*.hpp", QDir::Files, QDirIterator::Subdirectories);
        
        while (it.hasNext()) {
            QString file = it.next();
            processFile(file, graph);
        }

        return { "ExportKnowledgeGraph", "```mermaid\n" + graph + "```", false };
    }

private:
    void processFile(const QString& path, QString& graph) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QString content = QString::fromUtf8(file.readAll());
        QRegularExpression classRegex("class\\s+(\\w+)(?:\\s*:\\s*(?:public|protected|private)?\\s*(\\w+))?");
        auto matchIt = classRegex.globalMatch(content);

        while (matchIt.hasNext()) {
            auto match = matchIt.next();
            QString className = match.captured(1);
            QString baseClass = match.captured(2);

            if (!baseClass.isEmpty()) {
                graph += QString("    %1 --|> %2\n").arg(className).arg(baseClass);
            } else {
                graph += QString("    class %1\n").arg(className);
            }
        }
    }

    AppConfig* m_config;
};

} // namespace CodeHex
