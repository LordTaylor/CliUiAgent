#pragma once
#include <QScreen>
#include <QGuiApplication>
#include <QDir>
#include <QUuid>
#include <QPixmap>
#include <QJsonDocument>
#include "../Tool.h"

namespace CodeHex {

/**
 * @brief Tool to capture a screenshot of the primary screen.
 * Roadmap Item #2: Autonomous Vision.
 */
class TakeScreenshotTool : public Tool {
public:
    QString name() const override { return "TakeScreenshot"; }
    
    QString description() const override {
        return "Captures a screenshot of the entire primary screen and returns the file path. "
               "Use this to identify UI bugs, layout issues, or to 'see' what is currently on the screen. "
               "The screenshot is saved as a PNG file.";
    }

    QJsonObject parameters() const override {
        // No parameters required for full screen capture
        return QJsonObject();
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        Q_UNUSED(input);
        Q_UNUSED(workDir);

        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            ToolResult res;
            res.content = "Error: No primary screen found for capture.";
            res.isError = true;
            return res;
        }

        // Capture the entire primary screen
        QPixmap screenshot = screen->grabWindow(0);
        if (screenshot.isNull()) {
            ToolResult res;
            res.content = "Error: Failed to capture screenshot (null pixmap).";
            res.isError = true;
            return res;
        }

        // Generate a unique filename and ensure directory exists
        QString fileName = QString("screenshot_%1.png").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        QString dirPath = QDir::tempPath() + "/codehex_screenshots";
        QDir().mkpath(dirPath);
        QString filePath = dirPath + "/" + fileName;

        // Save to PNG
        if (!screenshot.save(filePath, "PNG")) {
            ToolResult res;
            res.content = "Error: Failed to save screenshot to " + filePath;
            res.isError = true;
            return res;
        }

        QJsonObject out;
        out["path"] = filePath;
        out["status"] = "success";
        
        ToolResult res;
        res.content = QJsonDocument(out).toJson(QJsonDocument::Indented);
        res.isError = false;
        return res;
    }

    void abort() override {
        // Capture is nearly instantaneous, no specific abort logic needed
    }
};

} // namespace CodeHex
