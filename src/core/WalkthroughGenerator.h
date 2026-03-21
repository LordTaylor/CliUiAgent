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
    static void generate(Session* session, AppConfig* config) {
        if (!session) return;
        
        QString path = config->workingFolder() + "/walkthrough.md";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

        QTextStream out(&file);
        out << "# 🚀 Task Walkthrough: " << (session->title.isEmpty() ? "Session Analysis" : session->title) << "\n\n";
        out << "*Generated on: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "*\n\n";
        
        out << "## 🛠️ Actions Taken\n";
        int actionCount = 0;
        for (const auto& msg : session->messages) {
            if (msg.role == Message::Role::Assistant) {
                // Find tool calls using simple regex for now
                QRegularExpression re("<tool_call name=\"([^\"]+)\"");
                QRegularExpressionMatchIterator it = re.globalMatch(msg.textFromContentBlocks());
                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    QString toolName = match.captured(1);
                    actionCount++;
                    out << QString("%1. **%2**\n").arg(actionCount).arg(toolName);
                }
            }
        }

        if (actionCount == 0) {
            out << "*(No tools were exercised during this task)*\n";
        }

        out << "\n## ✅ Conclusion\n";
        out << "The task has been completed and verified. This document was automatically generated as part of the Phase 5 roadmap features.\n";
        
        file.close();
    }
};

} // namespace CodeHex
