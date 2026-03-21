#pragma once
#include "../Tool.h"
#include "../AppConfig.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace CodeHex {

/**
 * @brief Tool for searching the internet using the Tavily API.
 */
class WebSearchTool : public Tool {
public:
    explicit WebSearchTool(AppConfig* config) : m_config(config) {}

    QString name() const override { return "WebSearch"; }
    QString description() const override {
        return "Search the internet for technical information, documentation, or code examples. "
               "Requires a Tavily API key in Settings > Advanced.";
    }

    QJsonObject parameters() const override {
        return {
            {"type", "object"},
            {"properties", QJsonObject{
                {"query", QJsonObject{
                    {"type", "string"},
                    {"description", "The search query (e.g., 'Qt6 QNetworkAccessManager example')"}
                }},
                {"search_depth", QJsonObject{
                    {"type", "string"},
                    {"enum", QJsonArray{"basic", "advanced"}},
                    {"description", "The depth of the search (default: basic)"}
                }}
            }},
            {"required", QJsonArray{"query"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& /*workDir*/) override {
        QString apiKey = m_config->tavilyApiKey();
        if (apiKey.isEmpty()) {
            ToolResult err;
            err.content = "Error: Tavily API key is not set. Please go to Settings > Advanced and enter your API key from tavily.com.";
            err.isError = true;
            return err;
        }

        QString query = input.value("query").toString();
        QString depth = input.value("search_depth").toString("basic");
        
        qDebug() << "[WebSearchTool] Searching for:" << query << "depth:" << depth;

        // Create the JSON payload for Tavily
        QJsonObject payload;
        payload["api_key"] = apiKey;
        payload["query"] = query;
        payload["search_depth"] = depth;
        payload["max_results"] = 5;

        QJsonDocument payloadDoc(payload);
        QByteArray payloadData = payloadDoc.toJson(QJsonDocument::Compact);

        // Use curl for a synchronous, easy-to-implement network call
        QProcess proc;
        QStringList args;
        args << "-s" << "-X" << "POST" << "https://api.tavily.com/search"
             << "-H" << "Content-Type: application/json"
             << "-d" << QString::fromUtf8(payloadData);
        
        proc.start("curl", args);
        if (!proc.waitForFinished(10000)) {
            ToolResult err;
            err.content = "Error: Web search timed out (10s). Check your internet connection.";
            err.isError = true;
            return err;
        }

        QByteArray output = proc.readAllStandardOutput();
        QJsonDocument doc = QJsonDocument::fromJson(output);
        
        if (doc.isNull() || !doc.isObject()) {
            ToolResult err;
            err.content = "Error: Failed to parse Tavily response. Raw output: " + QString::fromLocal8Bit(output);
            err.isError = true;
            return err;
        }

        QJsonObject obj = doc.object();
        if (obj.contains("error")) {
            ToolResult err;
            err.content = "Tavily API Error: " + obj["error"].toString();
            err.isError = true;
            return err;
        }

        QJsonArray results = obj["results"].toArray();
        QString textResult = QString("### WEB SEARCH RESULTS FOR: %1\n\n").arg(query);
        
        if (results.isEmpty()) {
            textResult += "No relevant results found.";
        } else {
            for (int i = 0; i < results.size(); ++i) {
                QJsonObject res = results[i].toObject();
                QString title = res["title"].toString();
                QString url = res["url"].toString();
                QString snippet = res["content"].toString();
                
                textResult += QString("[%1](%2)\n> %3\n\n").arg(title).arg(url).arg(snippet);
            }
        }

        ToolResult res;
        res.content = textResult;
        return res;
    }

private:
    AppConfig* m_config;
};

} // namespace CodeHex
