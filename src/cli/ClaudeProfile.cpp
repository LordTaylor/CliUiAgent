#include "ClaudeProfile.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSysInfo>
#include "../core/TokenCounter.h"
#include "../data/Message.h"

namespace CodeHex {

static constexpr int kMaxHistoryMessages = 20;

// Shared flag builder — prompt is already fully assembled by callers.
static QStringList claudeBaseArgs(const QString& prompt, const QString& workDir) {
    QStringList args;
    // --verbose is required when using --output-format stream-json with --print.
    // --include-partial-messages enables real-time streaming tokens.
    // NOTE: --image flags are injected by CliRunner via imageArguments() BEFORE -p.
    args << "--print"
         << "--verbose"
         << "--output-format" << "stream-json"
         << "--include-partial-messages"
         << "-p" << prompt;
    if (!workDir.isEmpty())
        args << "--allowedTools" << "all";
    return args;
}

QStringList ClaudeProfile::buildArguments(const QString& prompt,
                                          const QString& workDir) const {
    return claudeBaseArgs(prompt, workDir);
}

QStringList ClaudeProfile::buildArguments(const QString& prompt,
                                          const QString& workDir,
                                          const QList<Message>& history,
                                          const QString& systemPrompt) const {
    // history contains all messages including the current user message at the end.
    // We format the prior exchanges as a conversation prefix and prepend to prompt.
    const int histEnd   = history.size() - 1;   // skip last (== current prompt)
    
    // Dynamic System Context
    QString systemContext;
    if (!systemPrompt.isEmpty()) {
        systemContext += systemPrompt + "\n\n";
    }
    systemContext += "## System Information\n";
    systemContext += "- OS: " + QSysInfo::productType() + " " + QSysInfo::productVersion() + "\n";
    systemContext += "- Current Time: " + QDateTime::currentDateTime().toString() + "\n";
    if (!workDir.isEmpty()) {
        systemContext += "- Working Directory: " + workDir + "\n";
        systemContext += "- Project Structure (Top Level):\n";
        QDir dir(workDir);
        for (const QString& entry : dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
            systemContext += "  - " + entry + (QFileInfo(workDir + "/" + entry).isDir() ? "/" : "") + "\n";
        }
    }
    systemContext += "\n";

    // Sliding Window with Token Counting
    constexpr int kMaxTokens = 100000;
    int currentTokens = TokenCounter::estimate(systemContext) + TokenCounter::estimate(prompt);
    
    QStringList historyBlocks;
    for (int i = histEnd - 1; i >= 0; --i) {
        const Message& msg = history.at(i);
        QString block;
        if (msg.role == Message::Role::User)
            block = "[User]: " + msg.textFromContentBlocks() + "\n";
        else if (msg.role == Message::Role::Assistant) {
            QString text = msg.textFromContentBlocks();
            block = (msg.contentTypes.contains(Message::ContentType::Output) ? "[Tool Result]: " : "[Assistant]: ") + text + "\n";
        }
        
        const int msgTokens = TokenCounter::estimate(block);
        if (currentTokens + msgTokens > kMaxTokens) break;
        
        currentTokens += msgTokens;
        historyBlocks.prepend(block);
    }

    const QString context = systemContext + historyBlocks.join("");
    const QString fullPrompt = context.isEmpty() ? prompt : context + "\n[User]: " + prompt;

    return claudeBaseArgs(fullPrompt, workDir);
}

QStringList ClaudeProfile::imageArguments(const QStringList& imagePaths) const {
    // claude CLI supports --image <path> for vision. Multiple images are supported.
    QStringList args;
    for (const QString& path : imagePaths)
        args << "--image" << path;
    return args;
}

QString ClaudeProfile::parseStreamChunk(const QByteArray& raw) const {
    // Legacy text-only parser — used by the default parseLine() base implementation.
    // ClaudeProfile overrides parseLine() directly, but this stays for back-compat.
    const auto doc = QJsonDocument::fromJson(raw.trimmed());
    if (doc.isNull()) return {};

    const auto obj  = doc.object();
    const QString tp = obj["type"].toString();

    if (tp == "stream_event") {
        const auto event = obj["event"].toObject();
        if (event["type"].toString() == "content_block_delta") {
            const auto delta = event["delta"].toObject();
            if (delta["type"].toString() == "text_delta")
                return delta["text"].toString();
        }
        return {};
    }
    // Fallback: full message without streaming
    if (tp == "assistant") {
        const auto content = obj["message"].toObject()["content"].toArray();
        if (!content.isEmpty())
            return content[0].toObject()["text"].toString();
    }
    return {};
}

// ── parseLine ─────────────────────────────────────────────────────────────────
// Stateful parser that handles BOTH text tokens and tool-call events.
//
// Claude CLI streaming format for tool use (with --allowedTools all):
//
//  Block start   {"type":"stream_event","event":{"type":"content_block_start",
//                  "index":1,"content_block":{"type":"tool_use",
//                  "id":"toolu_01…","name":"Bash","input":{}}}}
//
//  Input delta   {"type":"stream_event","event":{"type":"content_block_delta",
//                  "index":1,"delta":{"type":"input_json_delta",
//                  "partial_json":"{\"command\":\"ls \"}"}}}
//
//  Block stop    {"type":"stream_event","event":{"type":"content_block_stop",
//                  "index":1}}
//
// Multiple tool_use + text blocks can appear in a single response; each has its
// own index.  We only track one active block at a time (Claude emits them serially).
StreamResult ClaudeProfile::parseLine(const QByteArray& raw) const {
    const auto doc = QJsonDocument::fromJson(raw.trimmed());
    if (doc.isNull()) return {};

    const auto obj  = doc.object();
    const QString tp = obj["type"].toString();

    // Fallback for full-message format (no --include-partial-messages)
    if (tp == "assistant") {
        const auto content = obj["message"].toObject()["content"].toArray();
        if (!content.isEmpty())
            return StreamResult{ content[0].toObject()["text"].toString(), {} };
        return {};
    }

    if (tp != "stream_event") return {};

    const auto    event  = obj["event"].toObject();
    const QString evType = event["type"].toString();
    const int     evIdx  = event["index"].toInt(-1);

    // ── content_block_start ───────────────────────────────────────────────────
    if (evType == "content_block_start") {
        const auto cb = event["content_block"].toObject();
        if (cb["type"].toString() == "tool_use") {
            // Begin accumulating a new tool call
            m_toolBlockIdx = evIdx;
            m_toolId       = cb["id"].toString();
            m_toolName     = cb["name"].toString();
            m_inputAccum.clear();
        }
        return {};
    }

    // ── content_block_delta ───────────────────────────────────────────────────
    if (evType == "content_block_delta") {
        const auto delta     = event["delta"].toObject();
        const QString dType  = delta["type"].toString();

        // Text token from a text block
        if (dType == "text_delta")
            return StreamResult{ delta["text"].toString(), {} };

        // Tool input accumulation — only for the active tool block
        if (dType == "input_json_delta" && evIdx == m_toolBlockIdx)
            m_inputAccum += delta["partial_json"].toString();

        return {};
    }

    // ── content_block_stop ────────────────────────────────────────────────────
    if (evType == "content_block_stop" && m_toolBlockIdx >= 0 && evIdx == m_toolBlockIdx) {
        ToolCall tc;
        tc.id   = m_toolId;
        tc.name = m_toolName;
        const auto inputDoc = QJsonDocument::fromJson(m_inputAccum.toUtf8());
        tc.input = inputDoc.isObject() ? inputDoc.object() : QJsonObject{};

        // Reset accumulator state
        m_toolBlockIdx = -1;
        m_toolId.clear();
        m_toolName.clear();
        m_inputAccum.clear();

        return StreamResult{ {}, tc };
    }

    return {};
}

void ClaudeProfile::reset() {
    m_toolBlockIdx = -1;
    m_toolId.clear();
    m_toolName.clear();
    m_inputAccum.clear();
}

}  // namespace CodeHex
