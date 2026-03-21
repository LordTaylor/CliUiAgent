#pragma once
#include <QString>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include "../Tool.h"

namespace CodeHex {

class AnalyzePerformanceTool : public Tool {
public:
    QString name() const override { return "AnalyzePerformance"; }
    QString description() const override { 
        return "Runs system performance analysis (CPU, memory, process stats) to identify bottlenecks."; 
    }

    QJsonObject parameters() const override {
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        
        QJsonObject target;
        target["type"] = "string";
        target["description"] = "Specific process or system metric to analyze (e.g., 'cpu', 'memory', 'top').";
        props["target"] = target;

        params["properties"] = props;
        return params;
    }

    ToolResult execute(const QJsonObject& args, const QString& /*workDir*/) override {
        QString target = args["target"].toString("top");
        
        QProcess process;
        QStringList processArgs;
        
#ifdef Q_OS_MAC
        if (target == "cpu" || target == "top") {
            processArgs << "-l" << "1" << "-n" << "10" << "-o" << "cpu";
            process.start("top", processArgs);
        } else if (target == "memory") {
            process.start("vm_stat");
        } else {
            processArgs << "-ax" << "-o" << "pid,ppid,%cpu,%mem,command";
            process.start("ps", processArgs);
        }
#else
        processArgs << "-b" << "-n" << "1";
        process.start("top", processArgs);
#endif

        if (!process.waitForFinished(5000)) {
            ToolResult err;
            err.content = "Performance analysis timed out.";
            err.isError = true;
            return err;
        }

        ToolResult res;
        res.content = QString::fromUtf8(process.readAllStandardOutput());
        return res;
    }
};

} // namespace CodeHex
