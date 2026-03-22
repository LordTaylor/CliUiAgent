#include "ResponseParser.h"
#include <QRegularExpression>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>

namespace CodeHex {

ResponseParser::ParseResult ResponseParser::parse(const QString& response,
                                                   const QString& preferredToolFormat) {
    ParseResult result;
    
    // 1. Extract Thoughts — handles:
    //    <thought>/<think>  (CodeHex / Qwen3 / DeepSeek)
    //    [THINK]...[/THINK] (Mistral Large/Nemo)
    static const QRegularExpression thoughtRe(
        "(?:<(?:thought|think)>(.*?)</(?:thought|think)>|\\[THINK\\](.*?)\\[/THINK\\])",
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator thoughtIt = thoughtRe.globalMatch(response);
    while (thoughtIt.hasNext()) {
        QRegularExpressionMatch match = thoughtIt.next();
        ThoughtBlock block;
        // Group 1 = <thought>/<think>, Group 2 = [THINK]
        block.content = match.captured(1).isEmpty()
                      ? match.captured(2).trimmed()
                      : match.captured(1).trimmed();
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
        
        // --- Robustness Tip: Strip markdown blocks if the agent hallucinated them inside <input> ---
        if (jsonStr.startsWith("```")) {
            // Remove starting block (e.g., ```json or just ```)
            QRegularExpression startBlockRe("^```(?:json|xml|bash)?\\s*");
            jsonStr.remove(startBlockRe);
            // Remove ending block (e.g., ```)
            if (jsonStr.endsWith("```")) {
                jsonStr.chop(3);
            }
            jsonStr = jsonStr.trimmed();
        }
        
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

    // 2b. Preferred-format fast path (profile-driven)
    //     Runs the model's native parser before the generic fallback chain
    //     so unrelated formats don't trigger false-positive matches.
    //     The full fallback chain still runs afterwards as a safety net.
    if (result.toolCalls.isEmpty() && !preferredToolFormat.isEmpty()
            && preferredToolFormat != "codehex-xml") {

        auto tryQwenXml = [&]() {
            static const QRegularExpression re(
                "<tool_call>\\s*<function=([^>]+)>(.*?)</function>\\s*</tool_call>",
                QRegularExpression::DotMatchesEverythingOption);
            static const QRegularExpression paramRe(
                "<parameter=([^>]+)>\\n?(.*?)\\n?</parameter>",
                QRegularExpression::DotMatchesEverythingOption);
            auto it = re.globalMatch(response);
            while (it.hasNext()) {
                auto bm = it.next();
                ToolCall call;
                call.id    = QUuid::createUuid().toString();
                call.name  = bm.captured(1).trimmed();
                call.valid = !call.name.isEmpty();
                QJsonObject input;
                auto pit = paramRe.globalMatch(bm.captured(2));
                while (pit.hasNext()) { auto pm = pit.next(); input[pm.captured(1).trimmed()] = pm.captured(2).trimmed(); }
                call.input = input;
                if (call.valid) result.toolCalls << call;
            }
        };

        auto tryDeepSeekNative = [&]() {
            static const QRegularExpression re(
                "<\uFF5Ctool\u2581call\u2581begin\uFF5C>[^<]*<\uFF5Ctool\u2581sep\uFF5C>([^\n<\uFF5C]+)\\n```json\\n(.*?)\\n```\\s*<\uFF5Ctool\u2581call\u2581end\uFF5C>",
                QRegularExpression::DotMatchesEverythingOption);
            auto it = re.globalMatch(response);
            while (it.hasNext()) {
                auto bm = it.next();
                ToolCall call;
                call.id   = QUuid::createUuid().toString();
                call.name = bm.captured(1).trimmed();
                QJsonParseError err;
                auto doc = QJsonDocument::fromJson(bm.captured(2).trimmed().toUtf8(), &err);
                call.valid = !call.name.isEmpty() && !doc.isNull() && doc.isObject();
                if (call.valid) { call.input = doc.object(); result.toolCalls << call; }
            }
        };

        auto tryMistralNative = [&]() {
            static const QRegularExpression re(
                "\\[TOOL_CALLS\\]\\s*(\\[.*?\\])",
                QRegularExpression::DotMatchesEverythingOption);
            auto mm = re.match(response);
            if (!mm.hasMatch()) return;
            QJsonParseError err;
            auto doc = QJsonDocument::fromJson(mm.captured(1).toUtf8(), &err);
            if (doc.isNull() || !doc.isArray()) return;
            for (const QJsonValue& v : doc.array()) {
                QJsonObject obj = v.toObject();
                ToolCall call;
                call.id    = QUuid::createUuid().toString();
                call.name  = obj["name"].toString().trimmed();
                call.valid = !call.name.isEmpty();
                QJsonValue args = obj["arguments"];
                call.input = args.isObject() ? args.toObject() : QJsonObject();
                if (call.valid) result.toolCalls << call;
            }
        };

        if      (preferredToolFormat == "qwen-xml")        tryQwenXml();
        else if (preferredToolFormat == "deepseek-native") tryDeepSeekNative();
        else if (preferredToolFormat == "mistral-native")  tryMistralNative();
    }

    // 3. Fallback: Qwen3-Coder-Next native XML format
    //    <tool_call><function=ToolName><parameter=key>\nvalue\n</parameter></function></tool_call>
    if (result.toolCalls.isEmpty()) {
        static const QRegularExpression qwenBlockRe(
            "<tool_call>\\s*<function=([^>]+)>(.*?)</function>\\s*</tool_call>",
            QRegularExpression::DotMatchesEverythingOption);
        static const QRegularExpression qwenParamRe(
            "<parameter=([^>]+)>\\n?(.*?)\\n?</parameter>",
            QRegularExpression::DotMatchesEverythingOption);

        QRegularExpressionMatchIterator blockIt = qwenBlockRe.globalMatch(response);
        while (blockIt.hasNext()) {
            QRegularExpressionMatch bm = blockIt.next();
            ToolCall call;
            call.id    = QUuid::createUuid().toString();
            call.name  = bm.captured(1).trimmed();
            call.valid = !call.name.isEmpty();

            QJsonObject input;
            QRegularExpressionMatchIterator paramIt = qwenParamRe.globalMatch(bm.captured(2));
            while (paramIt.hasNext()) {
                QRegularExpressionMatch pm = paramIt.next();
                input[pm.captured(1).trimmed()] = pm.captured(2).trimmed();
            }
            call.input = input;
            if (call.valid) result.toolCalls << call;
        }
    }

    // 4. Fallback: Pure XML Format (<tool_name>...<parameters>)
    if (result.toolCalls.isEmpty()) {
        QRegularExpression pureXmlRe("<tool_name>\\s*([^<\\s]+)\\s*</tool_name>\\s*<parameters>\\s*(.*?)\\s*</parameters>", 
                                    QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatchIterator pureIt = pureXmlRe.globalMatch(response);
        while (pureIt.hasNext()) {
            QRegularExpressionMatch match = pureIt.next();
            QString tname = match.captured(1).trimmed();
            QString paramsXml = match.captured(2).trimmed();

            ToolCall call;
            call.id = QUuid::createUuid().toString();
            call.name = tname;
            call.valid = true;

            // Simple XML parameter parser: <Key>Value</Key>
            QRegularExpression paramRe("<([^>\\s]+)>\\s*(.*?)\\s*</\\1>", 
                                      QRegularExpression::DotMatchesEverythingOption);
            QRegularExpressionMatchIterator paramIt = paramRe.globalMatch(paramsXml);
            QJsonObject input;
            while (paramIt.hasNext()) {
                QRegularExpressionMatch pMatch = paramIt.next();
                input[pMatch.captured(1)] = pMatch.captured(2).trimmed();
            }
            call.input = input;
            result.toolCalls << call;
        }
    }

    // 5. Fallback: Bash Markdown Blocks
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

    // 6. Fallback: DeepSeek native format
    //    <｜tool▁call▁begin｜>function<｜tool▁sep｜>ToolName\n```json\n{...}\n```<｜tool▁call▁end｜>
    if (result.toolCalls.isEmpty()) {
        static const QRegularExpression deepseekRe(
            "<\uFF5Ctool\u2581call\u2581begin\uFF5C>[^<]*<\uFF5Ctool\u2581sep\uFF5C>([^\n<\uFF5C]+)\\n```json\\n(.*?)\\n```\\s*<\uFF5Ctool\u2581call\u2581end\uFF5C>",
            QRegularExpression::DotMatchesEverythingOption);

        QRegularExpressionMatchIterator dsIt = deepseekRe.globalMatch(response);
        while (dsIt.hasNext()) {
            QRegularExpressionMatch bm = dsIt.next();
            ToolCall call;
            call.id    = QUuid::createUuid().toString();
            call.name  = bm.captured(1).trimmed();
            call.valid = !call.name.isEmpty();

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(bm.captured(2).trimmed().toUtf8(), &err);
            if (!doc.isNull() && doc.isObject()) {
                call.input = doc.object();
            } else {
                call.valid = false;
                qWarning() << "[ResponseParser] DeepSeek invalid JSON for tool" << call.name
                           << "—" << err.errorString();
            }
            if (call.valid) result.toolCalls << call;
        }
    }

    // 7. Fallback: Mistral native format
    //    [TOOL_CALLS][{"name": "...", "arguments": {...}}]
    if (result.toolCalls.isEmpty()) {
        static const QRegularExpression mistralRe(
            "\\[TOOL_CALLS\\]\\s*(\\[.*?\\])",
            QRegularExpression::DotMatchesEverythingOption);

        QRegularExpressionMatch mm = mistralRe.match(response);
        if (mm.hasMatch()) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(mm.captured(1).toUtf8(), &err);
            if (!doc.isNull() && doc.isArray()) {
                for (const QJsonValue& v : doc.array()) {
                    QJsonObject obj = v.toObject();
                    ToolCall call;
                    call.id    = QUuid::createUuid().toString();
                    call.name  = obj["name"].toString().trimmed();
                    call.valid = !call.name.isEmpty();
                    QJsonValue args = obj["arguments"];
                    call.input = args.isObject() ? args.toObject() : QJsonObject();
                    if (call.valid) result.toolCalls << call;
                }
            } else {
                qWarning() << "[ResponseParser] Mistral [TOOL_CALLS] invalid JSON —" << err.errorString();
            }
        }
    }

    // 8. Extract Confidence Score
    QRegularExpression confidenceRe("<confidence>\\s*(\\d+)\\s*</confidence>");
    QRegularExpressionMatch confidenceMatch = confidenceRe.match(response);
    if (confidenceMatch.hasMatch()) {
        result.confidenceScore = confidenceMatch.captured(1).toInt();
    }

    // 9. Clean Text for Display
    result.cleanText = cleanText(response);

    return result;
}

QString ResponseParser::cleanText(const QString& rawResponse) {
    QString clean = rawResponse;

    // Remove thinking tags — <thought>/<think> (CodeHex/Qwen3/DeepSeek) and [THINK] (Mistral)
    clean.remove(QRegularExpression("<(?:thought|think)>.*?</(?:thought|think)>", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("\\[THINK\\].*?\\[/THINK\\]", QRegularExpression::DotMatchesEverythingOption));

    // Remove Qwen3 native tool call blocks (entire JSON block)
    clean.remove(QRegularExpression("<tool_call>.*?</tool_call>", QRegularExpression::DotMatchesEverythingOption));

    // Remove tool-related tags (CodeHex XML format)
    clean.remove(QRegularExpression("<name>.*?</name>", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("<input>.*?</input>", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("<confidence>.*?</confidence>", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("<tool_response>.*?</tool_response>", QRegularExpression::DotMatchesEverythingOption));
    
    // Remove tool-related markdown blocks (Legacy/Fallback Bash)
    clean.remove(QRegularExpression("```bash\\n.*?```", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("```xml\\n.*?```", QRegularExpression::DotMatchesEverythingOption));
    
    // Remove DeepSeek tool call blocks (entire block including Unicode separators)
    clean.remove(QRegularExpression("<\uFF5Ctool\u2581calls\u2581begin\uFF5C>.*?<\uFF5Ctool\u2581calls\u2581end\uFF5C>", QRegularExpression::DotMatchesEverythingOption));
    // Remove DeepSeek tool output blocks
    clean.remove(QRegularExpression("<\uFF5Ctool\u2581outputs\u2581begin\uFF5C>.*?<\uFF5Ctool\u2581outputs\u2581end\uFF5C>", QRegularExpression::DotMatchesEverythingOption));
    // Remove Mistral tool call and result blocks
    clean.remove(QRegularExpression("\\[TOOL_CALLS\\]\\s*\\[.*?\\]", QRegularExpression::DotMatchesEverythingOption));
    clean.remove(QRegularExpression("\\[TOOL_RESULTS\\].*?\\[/TOOL_RESULTS\\]", QRegularExpression::DotMatchesEverythingOption));

    // Remove stray completion markers often hallucinations in local models
    clean.remove(QRegularExpression("\\*\\*Task Completed:\\*\\*.*", QRegularExpression::CaseInsensitiveOption));
    clean.remove(QRegularExpression("\\*\\*Task Finalized:\\*\\*.*", QRegularExpression::CaseInsensitiveOption));

    return clean.trimmed();
}

} // namespace CodeHex
