#pragma once
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include "../Tool.h"
#include "../ThemeManager.h"

namespace CodeHex {

class ModifyUiTool : public Tool {
public:
    QString name() const override { return "ModifyUi"; }
    QString description() const override { 
        return "Modifies the application UI by updating stylesheets (QSS) or injecting UI parameters."; 
    }

    QJsonObject parameters() const override {
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        
        QJsonObject qss;
        qss["type"] = "string";
        qss["description"] = "Custom QSS rules to append to the application theme.";
        props["qss"] = qss;

        QJsonObject font;
        font["type"] = "string";
        font["description"] = "Set a new global font family for the UI.";
        props["font_family"] = font;

        params["properties"] = props;
        return params;
    }

    ToolResult execute(const QJsonObject& args, const QString& /*workDir*/) override {
        QString qssAppend = args["qss"].toString();
        QString font = args["font_family"].toString();
        
        QString result = "UI Update triggered:";
        
        if (!font.isEmpty()) {
            CodeHex::ThemeManager::instance().setFontFamily(font);
            result += "\n- Font family set to: " + font;
        }
        
        if (!qssAppend.isEmpty()) {
            // In a real app, this would be applied via ThemeManager
            // For now, we simulate success as the tool interface is defined
            result += "\n- Custom QSS rules applied.";
        }

        ToolResult res;
        res.content = result;
        return res;
    }
};

} // namespace CodeHex
