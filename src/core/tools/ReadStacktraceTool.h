#pragma once
#include "../Tool.h"
#include <QProcess>
#include <QStringList>
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>

namespace CodeHex {

/**
 * @brief Tool for Roadmap Item #17: Debugger.
 * Reads system crash logs or provides a way to capture diagnostic information.
 */
class ReadStacktraceTool : public Tool {
public:
    QString name() const override { return "ReadStacktrace"; }

    QString description() const override {
        return "Reads recent system crash logs or capturing diagnostic information "
               "about the current environment to help debug crashes and panics.";
    }

    QJsonObject parameters() const override {
        QJsonObject obj;
        obj["type"] = "object";
        QJsonObject props;
        QJsonObject limit;
        limit["type"] = "integer";
        limit["description"] = "Number of recent logs to scan (default: 1).";
        props["limit"] = limit;
        obj["properties"] = props;
        return obj;
    }

    ToolResult execute(const QJsonObject& input, const QString& /*context*/) override {
        int limit = input.contains("limit") ? input["limit"].toInt() : 1;
        if (limit <= 0) limit = 1;

#ifdef Q_OS_MAC
        // On macOS, we can look at ~/Library/Logs/DiagnosticReports/
        QString logDir = QDir::homePath() + "/Library/Logs/DiagnosticReports/";
        QDir dir(logDir);
        QStringList filters;
        filters << "*.ips" << "*.crash";
        dir.setNameFilters(filters);
        dir.setSorting(QDir::Time);

        QStringList entryList = dir.entryList();
        if (entryList.isEmpty()) {
            return { "ReadStacktrace", "No recent crash logs found in " + logDir, false };
        }

        QString result = "Recent Crash Logs found:\n";
        for (int i = 0; i < qMin(limit, (int)entryList.size()); ++i) {
            QString fileName = entryList.at(i);
            QFile file(logDir + fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                result += "--- FILE: " + fileName + " ---\n";
                // Only read first 4KB to avoid context overflow, but enough for stacktrace
                result += QString::fromUtf8(file.read(4096));
                result += "\n[... truncated ...]\n";
                file.close();
            }
        }
        return { "ReadStacktrace", result, false };
#else
        return { "ReadStacktrace", "Stacktrace reading not implemented for this OS yet.", false };
#endif
    }
};

} // namespace CodeHex
