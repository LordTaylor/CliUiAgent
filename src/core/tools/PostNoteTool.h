#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include "../Tool.h"
#include "../AgentEngine.h"

namespace CodeHex {

/**
 * @brief Tool that posts a note to the session blackboard (Short-Term Memory).
 */
class PostNoteTool : public Tool {
public:
    explicit PostNoteTool(AgentEngine* engine) : m_engine(engine) {}

    QString name() const override { return "PostNote"; }

    QString description() const override {
        return "Posts a concise note to the session blackboard to keep track of progress, "
               "pending items, or key discoveries during a complex task. These notes are "
               "injected into your system prompt for the duration of the session.";
    }

    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        
        QJsonObject keyProp;
        keyProp["type"] = "string";
        keyProp["description"] = "A short descriptive key for the note (e.g. 'todo', 'discovery', 'blocker').";
        
        QJsonObject valProp;
        valProp["type"] = "string";
        valProp["description"] = "The content of the note.";
        
        props["key"] = keyProp;
        props["value"] = valProp;
        schema["properties"] = props;
        
        QJsonArray required;
        required.append("key");
        required.append("value");
        schema["required"] = required;
        return schema;
    }

    ToolResult execute(const QJsonObject& input, const QString& /*workDir*/) override {
        QString key = input.value("key").toString();
        QString value = input.value("value").toString();
        m_engine->postNote(key, value);
        return ToolResult{ {}, QString("Note recorded: [%1] %2").arg(key, value), false };
    }

private:
    AgentEngine* m_engine;
};

} // namespace CodeHex
