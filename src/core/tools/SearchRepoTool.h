#pragma once
#include "../Tool.h"
#include "../rag/CodebaseIndexer.h"
#include <QJsonArray>

namespace CodeHex {

class SearchRepoTool : public Tool {
public:
    explicit SearchRepoTool(CodebaseIndexer* indexer) : m_indexer(indexer) {}

    QString name() const override { return "SearchRepo"; }
    QString description() const override {
        return "Search the entire codebase using semantic search (RAG). "
               "Returns relevant code snippets and their file paths.";
    }

    QJsonObject parameters() const override {
        return {
            {"type", "object"},
            {"properties", QJsonObject{
                {"query", QJsonObject{
                    {"type", "string"},
                    {"description", "The natural language search query"}
                }},
                {"limit", QJsonObject{
                    {"type", "integer"},
                    {"description", "Maximum number of results to return (default: 5)"}
                }}
            }},
            {"required", QJsonArray{"query"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& /*workDir*/) override {
        QString query = input.value("query").toString();
        int limit = input.value("limit").toInt(5);

        if (query.isEmpty()) {
            ToolResult err;
            err.content = "Query cannot be empty";
            err.isError = true;
            return err;
        }

        auto results = m_indexer->search(query, limit);
        
        QJsonArray jsonResults;
        QString textResult = "Found " + QString::number(results.size()) + " relevant snippets:\n\n";

        for (const auto& chunk : results) {
            QJsonObject obj;
            obj["file"] = chunk.filePath;
            obj["lines"] = QString("%1-%2").arg(chunk.startLine).arg(chunk.endLine);
            obj["content"] = chunk.content;
            jsonResults.append(obj);

            textResult += QString("--- %1 (Lines %2-%3) ---\n%4\n\n")
                            .arg(chunk.filePath)
                            .arg(chunk.startLine)
                            .arg(chunk.endLine)
                            .arg(chunk.content);
        }

        ToolResult res;
        res.content = textResult;
        // We could also store structured data if needed
        return res;
    }

private:
    CodebaseIndexer* m_indexer;
};

} // namespace CodeHex
