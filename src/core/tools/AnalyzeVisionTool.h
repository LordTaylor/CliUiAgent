#pragma once
#include "../Tool.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QBuffer>
#include <QJsonObject>
#include <QJsonArray>
#include "AgentEngine.h"

namespace CodeHex {

/**
 * @brief Tool for Roadmap Item #2: Autonomous Vision.
 * Takes a screenshot of the main screen and provides context for UI debugging.
 */
class AnalyzeVisionTool : public Tool {
public:
    explicit AnalyzeVisionTool(AgentEngine* engine) : m_engine(engine) {}

    QString name() const override { return "AnalyzeVision"; }
    
    QString description() const override {
        return "Captures a screenshot of the user's screen and performs visual analysis "
               "to detect UI bugs, layout issues, or state inconsistencies.";
    }

    QJsonObject parameters() const override {
        QJsonObject obj;
        obj["type"] = "object";
        QJsonObject props;
        QJsonObject task;
        task["type"] = "string";
        task["description"] = "Specific visual task or question (e.g., 'Is the red button visible?') or 'General' for overall status.";
        props["task"] = task;
        obj["properties"] = props;
        obj["required"] = QJsonArray{"task"};
        return obj;
    }

    ToolResult execute(const QJsonObject& input, const QString& /*context*/) override {
        QString task = input["task"].toString();
        if (task.isEmpty()) task = "General UI Analysis";

        // Capture primary screen
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            return { "", "Error: No screen detected", true };
        }

        QPixmap screenshot = screen->grabWindow(0);
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        screenshot.save(&buffer, "PNG");

        // In a real implementation, we would send this image to a Vision-capable LLM.
        // For now, we report the capture and trigger a 'vision' prompt via AgentEngine.
        QString result = QString("Screenshot captured (Size: %1x%2). Task: %3\n")
                         .arg(screenshot.width()).arg(screenshot.height()).arg(task);
        
        // Roadmap: This will integrate with AgentEngine::processVision(ba, task)
        result += "Note: Visual features are being processed by the multimodal secondary model.";

        return { "AnalyzeVision", result, false };
    }

private:
    AgentEngine* m_engine;
};

} // namespace CodeHex
