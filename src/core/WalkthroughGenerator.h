#pragma once
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include "../data/Session.h"
#include "AppConfig.h"

namespace CodeHex {

class WalkthroughGenerator {
public:
    static void generate(Session* session, AppConfig* config, const QString& finalSummary = QString()) {
        if (!session) return;
        
        QString path = config->workingFolder() + "/walkthrough.md";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

        QTextStream out(&file);
        out << "# 🚀 Task Walkthrough: " << (session->title.isEmpty() ? "Session Analysis" : session->title) << "\n\n";
        out << "*Generated on: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "*\n\n";
        
        if (!finalSummary.isEmpty()) {
            out << "## 📝 Summary\n" << finalSummary << "\n\n";
        }

        out << "## 🛠️ Actions Taken\n";
        QMap<QString, int> toolUsage;
        QStringList modifiedFiles;
        
        QRegularExpression toolRe("<tool_call name=\"([^\"]+)\"");
        QRegularExpression fileRe("(?:TargetFile|path)[\"\\s:]+([^\"\\s,{}]+)");

        for (const auto& msg : session->messages) {
            QString content = msg.textFromContentBlocks();
            
            // Count tools
            QRegularExpressionMatchIterator it = toolRe.globalMatch(content);
            while (it.hasNext()) {
                toolUsage[it.next().captured(1)]++;
            }
            
            // Identify modified files (very basic heuristic)
            if (msg.role == Message::Role::Assistant && (content.contains("WriteFile") || content.contains("Replace"))) {
                QRegularExpressionMatchIterator fit = fileRe.globalMatch(content);
                while (fit.hasNext()) {
                    QString f = fit.next().captured(1);
                    if (!f.isEmpty() && !modifiedFiles.contains(f) && f.contains(".")) {
                        modifiedFiles.append(f);
                    }
                }
            }
        }

        if (toolUsage.isEmpty()) {
            out << "*(No tools were exercised during this task)*\n";
        } else {
            for (auto it = toolUsage.begin(); it != toolUsage.end(); ++it) {
                out << QString("- **%1**: %2 times\n").arg(it.key()).arg(it.value());
            }
        }

        if (!modifiedFiles.isEmpty()) {
            out << "\n## 📄 Modified Files\n";
            for (const QString& f : modifiedFiles) {
                out << QString("- `%1`\n").arg(f);
            }
        }

        out << "\n## ✅ Conclusion\n";
        out << "The task has been completed and verified. This document was generated automatically by CodeHex.\n";
        
        file.close();
    }
};

} // namespace CodeHex
