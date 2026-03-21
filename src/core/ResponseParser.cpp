#include "ResponseParser.h"
#include <QRegularExpression>
#include <QUuid>
#include <QJsonDocument>
#include <QDebug>

namespace CodeHex {

ResponseParser::ParseResult ResponseParser::parse(const QString& response) {
    ParseResult result;
    
    // 1. Extract Thoughts
    QRegularExpression thoughtRe("<thought>(.*?)</thought>", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator thoughtIt = thoughtRe.globalMatch(response);
    while (thoughtIt.hasNext()) {
        QRegularExpressionMatch match = thoughtIt.next();
        ThoughtBlock block;
        block.content = match.captured(1).trimmed();
        if (!block.content.isEmpty()) {
            result.thoughts << block;
        }
    }

    // 2. Extract Tool Calls (XML)
    QRegularExpression toolRe("<name>\\s*([^<\\s]+)\\s*</name>\\s*<input>\\s*(.*?)\\s*</input>", 
                             QRegularExpression::DotMatchesEverythingOption);
    
    QRegularExpressionMatchIterator toolIt = toolRe.globalMatch(response);
    int lastPos = 0;
    while (toolIt.hasNext()) {
        QRegularExpressionMatch match = toolIt.next();
        QString tname = match.captured(1).trimmed();
        QString jsonStr = match.captured(2).trimmed();
        
        // Capture explanation (text between previous tool and this one)
        QString rawExplanation = response.mid(lastPos, match.capturedStart() - lastPos).trimmed();
        // Clean the explanation (remove thoughts and XML tags)
        QString cleanExpl = cleanText(rawExplanation).trimmed();
        lastPos = match.capturedEnd();

        ToolCall call;
        call.id = QUuid::createUuid().toString();
        call.name = tname;
        call.explanation = cleanExpl;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (!doc.isNull() && doc.isObject()) {
            call.input = doc.object();
            call.valid = true;
        } else {
            // Mark as invalid so AgentEngine can return a clear error instead of
            // executing the tool with empty parameters.
            call.valid = false;
            qWarning() << "[ResponseParser] Invalid JSON for tool" << tname
                       << "—" << err.errorString() << ". Marking call as invalid.";
        }
        result.toolCalls << call;
    }

    // 3. Fallback: Bash Markdown Blocks
    if (result.toolCalls.isEmpty()) {
        QRegularExpression bashRe("```bash\\n(.*?)```", 
                                 QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatchIterator bashIt = bashRe.globalMatch(response);
        while (bashIt.hasNext()) {
            QRegularExpressionMatch match = bashIt.next();
            ToolCall call;
            call.id = QUuid::createUuid().toString();
            call.name = "Bash";
            QJsonObject input;
            input["command"] = match.captured(1).trimmed();
            call.input = input;
            result.toolCalls << call;
            break; // Stop after first Bash block to match legacy behavior
        }
    }

    // 4. Extract Confidence Score
    QRegularExpression confidenceRe("<confidence>\\s*(\\d+)\\s*</confidence>");
    QRegularExpressionMatch confidenceMatch = confidenceRe.match(response);
    if (confidenceMatch.hasMatch()) {
        result.confidenceScore = confidenceMatch.captured(1).toInt();
    }

    // 5. Clean Text for Display
    result.cleanText = cleanText(response);

    return result;
}

QString ResponseParser::cleanText(const QString& rawResponse) {
    QString clean = rawResponse;

    // Remove thinking tags
    clean.remove(QRegularExpression("<thought>.*?</thought>", QRegularExpression::DotMatchesEverythingOption));
    
    // Remove tool-related tags
    clean.remove(QRegularExpression("<name>.*?</name>", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("<input>.*?</input>", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("<confidence>.*?</confidence>", QRegularExpression::DotMatchesEverythingOption));
    clean.remove("<tool_call>");
    clean.remove("</tool_call>");
    
    // Remove tool-related markdown blocks (Legacy/Fallback Bash)
    clean.remove(QRegularExpression("```bash\\n.*?```", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("```xml\\n.*?```", QRegularExpression::DotMatchesEverythingOption));
    
    // Remove stray completion markers often hallucinations in local models
    clean.remove(QRegularExpression("\\*\\*Task Completed:\\*\\*.*", QRegularExpression::CaseInsensitiveOption));
    clean.remove(QRegularExpression("\\*\\*Task Finalized:\\*\\*.*", QRegularExpression::CaseInsensitiveOption));

    return clean.trimmed();
}

} // namespace CodeHex
