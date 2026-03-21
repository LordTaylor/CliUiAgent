#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include "../Tool.h"
#include "../AgentEngine.h"

namespace CodeHex {

/**
 * @brief Tool that consults a second, independent LLM context ("Collaborator").
 * Roadmap Item #5: Multi-Agent Collaborative Mode.
 *
 * P-3 refactor: The tool now uses the ASYNC path (consultCollaboratorAsync).
 * It returns a synthetic "pending" result immediately, and AgentEngine routes
 * the real collaborator response through the normal toolFinished pipeline
 * when it arrives. This avoids blocking the worker thread with QEventLoop.
 */
class AskAgentTool : public Tool {
public:
    explicit AskAgentTool(AgentEngine* engine)
        : m_engine(engine) {}

    QString name() const override { return "AskAgent"; }

    QString description() const override {
        return "Asks a specialized AI agent (e.g. 'Architect', 'Debugger', 'Security Auditor') "
               "for consultation or a second opinion on your current reasoning or technical approach. "
               "The target agent has NO history of the current conversation — provide all "
               "relevant context in the 'prompt'. Returns the agent's expert analysis.";
    }

    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = "object";

        QJsonObject promptProp;
        promptProp["type"] = "string";
        promptProp["description"] = "The full question or context to send to the collaborator. "
                                    "Include all relevant details since the collaborator has no conversation history.";

        QJsonObject roleProp;
        roleProp["type"] = "string";
        roleProp["description"] = "Optional role for the collaborator, e.g. 'Code Reviewer', "
                                  "'Security Expert', 'Architect'. Defaults to 'Collaborator'.";

        QJsonObject props;
        props["prompt"] = promptProp;
        props["role"] = roleProp;

        schema["properties"] = props;

        QJsonArray required;
        required.append("prompt");
        schema["required"] = required;

        return schema;
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        Q_UNUSED(workDir);

        QString prompt = input.value("prompt").toString().trimmed();
        if (prompt.isEmpty()) {
            ToolResult res;
            res.content = "Error: 'prompt' parameter is required and cannot be empty.";
            res.isError = true;
            return res;
        }

        QString role = input.value("role").toString("Collaborator").trimmed();
        if (role.isEmpty()) role = "Collaborator";

        // Build a ToolCall to pass through for result routing
        ToolCall call;
        call.name = "AskAgent";
        call.input = input;
        // ID will be set by the caller (ToolExecutor sets toolUseId)

        // Launch async — does NOT block this thread
        QMetaObject::invokeMethod(m_engine, [this, call, prompt, role]() {
            m_engine->consultCollaboratorAsync(call, prompt, role);
        }, Qt::QueuedConnection);

        // Return a pending marker — the real result arrives via toolFinished signal
        ToolResult res;
        res.isError = false;
        res.content = QString("[Consulting %1... response will arrive asynchronously]").arg(role);
        res.isPending = true;  // Signal that this is not the final result
        return res;
    }

    void abort() override {
        // The collaborator runner can be stopped via AgentEngine::stop()
    }

private:
    AgentEngine* m_engine = nullptr;
};

} // namespace CodeHex
