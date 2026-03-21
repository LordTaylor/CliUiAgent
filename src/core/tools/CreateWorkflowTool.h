#pragma once
#include <QString>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../Tool.h"

namespace CodeHex {

class CreateWorkflowTool : public Tool {
public:
    QString name() const override { return "CreateWorkflow"; }
    QString description() const override { 
        return "Saves a sequence of actions/commands as a reusable skill/workflow in the project."; 
    }

    QJsonObject parameters() const override {
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        
        QJsonObject nameParam;
        nameParam["type"] = "string";
        nameParam["description"] = "The unique name of the workflow (e.g., 'deploy-prod', 'cleanup-build').";
        props["name"] = nameParam;

        QJsonObject descriptionParam;
        descriptionParam["type"] = "string";
        descriptionParam["description"] = "A short description of what this workflow does.";
        props["description"] = descriptionParam;

        QJsonObject steps;
        steps["type"] = "string";
        steps["description"] = "Markdown content detailing the steps (commands, tools to use).";
        props["steps"] = steps;

        params["properties"] = props;
        QJsonArray required;
        required.append("name");
        required.append("steps");
        params["required"] = required;

        return params;
    }

    ToolResult execute(const QJsonObject& args, const QString& /*workDir*/) override {
        QString wfName = args["name"].toString();
        QString desc = args["description"].toString();
        QString steps = args["steps"].toString();

        if (wfName.isEmpty() || steps.isEmpty()) {
            ToolResult err;
            err.content = "Workflow name and steps are required.";
            err.isError = true;
            return err;
        }

        QDir dir(".agents/workflows");
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString filePath = dir.filePath(wfName + ".md");
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ToolResult err;
            err.content = "Could not create workflow file at " + filePath;
            err.isError = true;
            return err;
        }

        QString content = "---\ndescription: " + desc + "\n---\n\n" + steps;
        file.write(content.toUtf8());
        file.close();

        ToolResult res;
        res.content = "Successfully created custom skill/workflow: " + wfName + " at " + filePath;
        return res;
    }
};

} // namespace CodeHex
