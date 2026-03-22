#pragma once
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

namespace CodeHex {

/**
 * @brief Runtime configuration for a specific LLM model family.
 *
 * Profiles are loaded from:
 *   1. :/resources/profiles/ (builtin, embedded in the app)
 *   2. ~/.codehex/profiles/  (user-defined or agent-written, higher priority)
 *
 * The agent can create new profiles by calling WriteFileTool on
 * ~/.codehex/profiles/{id}.json and ModelProfileManager hot-reloads automatically.
 */
struct ModelProfile {
    QString     id;                 // "qwen3", "deepseek", "mistral"
    QString     displayName;
    QStringList matchPatterns;      // lowercase substrings matched against model name

    int         contextWindow = 32768;

    // ── Thinking block tags ─────────────────────────────────────────────────
    QString     thoughtOpenTag  = "<think>";
    QString     thoughtCloseTag = "</think>";

    // Tool call format emitted BY the model.
    // Valid values:
    //   "codehex-xml"     - <name>...</name> <input>{json}</input>
    //   "qwen-xml"        - <tool_call><function=Name><parameter=key>val</parameter>
    //   "deepseek-native" - DeepSeek Unicode token separators + ```json block
    //   "mistral-native"  - [TOOL_CALLS][{"name":..., "arguments":{...}}]
    //   "pure-xml"        - <tool_name>...</tool_name> <parameters>...</parameters>
    //   "openai-json"     - standard OpenAI function call JSON
    QString     toolCallFormat = "codehex-xml";

    // Tool result format fed BACK to the model.
    // Valid values:
    //   "anthropic-block" - Anthropic tool_result content blocks
    //   "qwen-tool-role"  - role:tool + <tool_response>content</tool_response>
    //   "deepseek-output" - DeepSeek Unicode tool output markers
    //   "mistral-results" - [TOOL_RESULTS][{"call_id":"...", "content":"..."}][/TOOL_RESULTS]
    QString     toolResponseFormat = "qwen-tool-role";

    // ── Generation parameters ───────────────────────────────────────────────
    // If hasGenerationParams=false, these fields are NOT added to the API request.
    bool        hasGenerationParams = false;
    double      temperature = 0.7;
    double      topP        = 1.0;
    int         topK        = -1;   // -1 means: do not include in request

    QStringList stopSequences;

    // ── Serialization ───────────────────────────────────────────────────────
    static ModelProfile fromJson(const QJsonObject& obj) {
        ModelProfile p;
        p.id          = obj["id"].toString();
        p.displayName = obj["displayName"].toString();

        for (const auto& v : obj["matchPatterns"].toArray())
            p.matchPatterns << v.toString();

        p.contextWindow = obj["contextWindow"].toInt(32768);

        p.thoughtOpenTag  = obj["thoughtOpenTag"].toString("<think>");
        p.thoughtCloseTag = obj["thoughtCloseTag"].toString("</think>");

        p.toolCallFormat     = obj["toolCallFormat"].toString("codehex-xml");
        p.toolResponseFormat = obj["toolResponseFormat"].toString("qwen-tool-role");

        p.hasGenerationParams = obj["hasGenerationParams"].toBool(false);
        p.temperature = obj["temperature"].toDouble(0.7);
        p.topP        = obj["topP"].toDouble(1.0);
        p.topK        = obj["topK"].toInt(-1);

        for (const auto& v : obj["stopSequences"].toArray())
            p.stopSequences << v.toString();

        return p;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"]          = id;
        obj["displayName"] = displayName;

        QJsonArray pats;
        for (const auto& s : matchPatterns) pats.append(s);
        obj["matchPatterns"] = pats;

        obj["contextWindow"]   = contextWindow;
        obj["thoughtOpenTag"]  = thoughtOpenTag;
        obj["thoughtCloseTag"] = thoughtCloseTag;
        obj["toolCallFormat"]  = toolCallFormat;
        obj["toolResponseFormat"] = toolResponseFormat;
        obj["hasGenerationParams"] = hasGenerationParams;
        obj["temperature"] = temperature;
        obj["topP"]        = topP;
        obj["topK"]        = topK;

        QJsonArray stops;
        for (const auto& s : stopSequences) stops.append(s);
        obj["stopSequences"] = stops;

        return obj;
    }

    /** Returns true if this profile should be used for the given (lowercased) model name. */
    bool matches(const QString& modelNameLower) const {
        for (const auto& pat : matchPatterns) {
            if (modelNameLower.contains(pat))
                return true;
        }
        return false;
    }

    /** Sensible defaults for unknown models (Qwen-style params, codehex-xml tool format). */
    static ModelProfile defaultProfile() {
        ModelProfile p;
        p.id          = "default";
        p.displayName = "Default";
        p.contextWindow = 32768;
        p.toolCallFormat     = "codehex-xml";
        p.toolResponseFormat = "qwen-tool-role";
        p.hasGenerationParams = true;
        p.temperature = 0.7;
        p.topP        = 1.0;
        p.topK        = -1;
        return p;
    }
};

} // namespace CodeHex
