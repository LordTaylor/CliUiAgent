#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include "../Tool.h"

namespace CodeHex {

class AgentEngine;

/**
 * @brief Tool that consults a second, independent LLM context ("Collaborator").
 * Roadmap Item #5: Multi-Agent Collaborative Mode.
 *
 * The tool sends a prompt to a separate LLM session (via AgentEngine::consultCollaborator)
 * and returns its response. This allows the primary agent to debate, review, or
 * cross-check ideas before committing to an action.
 *
 * The call is SYNCHRONOUS from the tool's perspective - it blocks until the
 * collaborator responds (the event loop spins inside consultCollaborator).
 */
class ConsultCollaboratorTool : public Tool {
public:
    explicit ConsultCollaboratorTool(AgentEngine* engine)
        : m_engine(engine) {}

    QString name() const override { return "ConsultCollaborator"; }

    QString description() const override {
        return "Consults an independent AI collaborator to review, debate, or validate your "
               "current reasoning. Use this BEFORE making critical decisions (complex refactors, "
               "architectural changes, security-sensitive operations) to get a second opinion. "
               "The collaborator has NO history of the current conversation — provide all "
               "relevant context in the 'prompt'. Returns the collaborator's analysis.";
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

        // Delegate synchronous LLM call to AgentEngine
        QString response = m_engine->consultCollaborator(prompt, role);

        ToolResult res;
        res.isError = false;
        res.content = QString("[%1 Response]\n%2").arg(role, response);
        return res;
    }

    void abort() override {
        // consultCollaborator uses a QEventLoop; aborting from outside is not
        // trivially supported. The timeout in consultCollaborator handles runaway calls.
    }

private:
    AgentEngine* m_engine = nullptr;
};

} // namespace CodeHex
