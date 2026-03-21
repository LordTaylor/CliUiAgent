#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include "../Tool.h"
#include "../AgentEngine.h"

namespace CodeHex {

/**
 * @brief Tool that activates a specialized behavioral technique (e.g. TDD, Clean Code).
 */
class ActivateTechniqueTool : public Tool {
public:
    explicit ActivateTechniqueTool(AgentEngine* engine) : m_engine(engine) {}

    QString name() const override { return "ActivateTechnique"; }

    QString description() const override {
        return "Activates a specialized architectural or methodology-based technique "
               "to guide your reasoning. Available techniques: 'tdd', 'clean_code', 'performance'.";
    }

    QJsonObject parameters() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        QJsonObject nameProp;
        nameProp["type"] = "string";
        nameProp["enum"] = QJsonArray({"tdd", "clean_code", "performance"});
        nameProp["description"] = "The name of the technique to activate.";
        props["name"] = nameProp;
        schema["properties"] = props;
        QJsonArray required;
        required.append("name");
        schema["required"] = required;
        return schema;
    }

    ToolResult execute(const QJsonObject& input, const QString& /*workDir*/) override {
        QString name = input.value("name").toString();
        m_engine->activateTechnique(name);
        return ToolResult{ {}, QString("Technique '%1' activated successfully.").arg(name), false };
    }

private:
    AgentEngine* m_engine;
};

} // namespace CodeHex
