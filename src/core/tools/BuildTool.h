#pragma once
#include "../Tool.h"
#include <QProcess>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include "AppConfig.h"

namespace CodeHex {

/**
 * @brief Tool for Roadmap Item #18: Build.
 * Executes a build command and streams results to the agent.
 */
class BuildTool : public Tool {
public:
    explicit BuildTool(AppConfig* config) : m_config(config) {}

    QString name() const override { return "Build"; }

    QString description() const override {
        return "Executes a build command (e.g., cmake, make, ninja) in the working folder "
               "and returns the compilation logs. Useful for autonomous error fixing.";
    }

    QJsonObject parameters() const override {
        QJsonObject obj;
        obj["type"] = "object";
        QJsonObject props;
        QJsonObject target;
        target["type"] = "string";
        target["description"] = "Optional build target (e.g., 'all', 'clean', 'codehex_tests').";
        props["target"] = target;
        obj["properties"] = props;
        return obj;
    }

    ToolResult execute(const QJsonObject& input, const QString& /*context*/) override {
        QString target = input["target"].toString();
        QString workingFolder = m_config->workingFolder();

        // Detect project type (for now we assume CMake/Ninja as per project context)
        QStringList args;
        QString program = "cmake";
        
        // Try to locate build directory
        QString buildDir = workingFolder + "/build";
        if (!QDir(buildDir).exists()) {
            buildDir = workingFolder + "/build/Debug"; // Default preset path
        }

        if (QDir(buildDir).exists()) {
            args << "--build" << buildDir;
            if (!target.isEmpty()) {
                args << "--target" << target;
            }
        } else {
            return { "Build", "Error: Could not find build directory in " + workingFolder + ". Please configure CMake first.", true };
        }

        QProcess process;
        process.setWorkingDirectory(workingFolder);
        process.start(program, args);

        if (!process.waitForStarted()) {
            return { "Build", "Error: Failed to start build process: " + program, true };
        }

        if (!process.waitForFinished(300000)) { // 5 minute timeout
            process.terminate();
            return { "Build", "Error: Build process timed out after 5 minutes.", true };
        }

        QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
        QString error = QString::fromLocal8Bit(process.readAllStandardOutput());

        bool isError = (process.exitCode() != 0);
        QString fullLog = output + "\n" + error;

        return { "Build", fullLog.trimmed(), isError };
    }

private:
    AppConfig* m_config;
};

} // namespace CodeHex
