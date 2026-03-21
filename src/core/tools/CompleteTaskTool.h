#pragma once
#include "../Tool.h"
#include "WalkthroughGenerator.h"
#include "../SessionManager.h"
#include <QJsonArray>
#include <QJsonObject>

namespace CodeHex {

/**
 * @brief Tool for marking a task as complete and generating a final walkthrough.
 */
class CompleteTaskTool : public Tool {
public:
    explicit CompleteTaskTool(SessionManager* sessions, AppConfig* config) 
        : m_sessions(sessions), m_config(config) {}

    QString name() const override { return "CompleteTask"; }
    QString description() const override {
        return "Call this tool when you have finished all sub-tasks and the overall objective. "
               "It generates an automated walkthrough of your actions. "
               "Include a final 'summary' of what you achieved.";
    }

    QJsonObject parameters() const override {
        return {
            {"type", "object"},
            {"properties", QJsonObject{
                {"summary", QJsonObject{
                    {"type", "string"},
                    {"description", "A brief summary of the accomplishments and final state."}
                }}
            }},
            {"required", QJsonArray{"summary"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& /*workDir*/) override {
        QString summary = input.value("summary").toString();
        
        auto* session = m_sessions->currentSession();
        if (session) {
            WalkthroughGenerator::generate(session, m_config, summary);
        }

        ToolResult res;
        res.content = "Task marked as complete. Walkthrough generated at walkthrough.md.";
        return res;
    }

private:
    SessionManager* m_sessions;
    AppConfig* m_config;
};

} // namespace CodeHex
