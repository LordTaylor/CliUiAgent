#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include "../../data/ModelProfile.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace CodeHex {

/**
 * @brief Agent-callable tool that reads a HuggingFace tokenizer_config.json
 *        (and optional generation_config.json), auto-detects the model family
 *        and prompt format, then saves a ModelProfile JSON to
 *        ~/.codehex/profiles/{id}.json (hot-reloaded instantly — no restart).
 *
 * Usage example (agent):
 *   ImportModelProfile({
 *     "tokenizerConfigPath": "~/Downloads/tokenizer_config.json",
 *     "generationConfigPath": "~/Downloads/generation_config.json"
 *   })
 *
 * Detection rules:
 *   <|im_start|> in tokens → Qwen  (qwen-xml tool format)
 *   <｜User｜> in template  → DeepSeek (deepseek-native)
 *   [TOOL_CALLS] in extras  → Mistral (mistral-native)
 *   LlamaTokenizer fallback → llama (codehex-xml)
 */
class ImportModelProfileTool : public Tool {
public:
    QString name() const override { return "ImportModelProfile"; }
    QString description() const override {
        return "Reads a HuggingFace tokenizer_config.json (and optional generation_config.json), "
               "auto-detects the model family and prompt/tool format, then saves a ModelProfile to "
               "~/.codehex/profiles/{id}.json. Hot-reloaded instantly — no app restart required.";
    }

    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"tokenizerConfigPath", QJsonObject{
                    {"type", "string"},
                    {"description", "Absolute or workdir-relative path to tokenizer_config.json"}
                }},
                {"generationConfigPath", QJsonObject{
                    {"type", "string"},
                    {"description", "(Optional) Path to generation_config.json for temperature/top_p/top_k"}
                }},
                {"profileId", QJsonObject{
                    {"type", "string"},
                    {"description", "(Optional) Override the profile id (e.g. 'qwen3-custom')"}
                }},
                {"displayName", QJsonObject{
                    {"type", "string"},
                    {"description", "(Optional) Human-readable display name"}
                }}
            }},
            {"required", QJsonArray{"tokenizerConfigPath"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        const QString tcPath = resolveTilde(
            ToolUtils::resolvePath(input["tokenizerConfigPath"].toString(), workDir));
        if (tcPath.isEmpty())
            return ToolUtils::errResult("ImportModelProfile: 'tokenizerConfigPath' is required");

        QFile tcFile(tcPath);
        if (!tcFile.open(QIODevice::ReadOnly))
            return ToolUtils::errResult(
                QString("ImportModelProfile: cannot open '%1'").arg(tcPath));

        QJsonParseError err;
        const QJsonDocument tcDoc = QJsonDocument::fromJson(tcFile.readAll(), &err);
        if (tcDoc.isNull())
            return ToolUtils::errResult(
                QString("ImportModelProfile: JSON parse error: %1").arg(err.errorString()));

        ModelProfile profile = detectProfile(tcDoc.object());

        if (const QString ov = input["profileId"].toString().trimmed(); !ov.isEmpty())
            profile.id = ov;
        if (const QString ov = input["displayName"].toString().trimmed(); !ov.isEmpty())
            profile.displayName = ov;

        // Optional generation_config.json
        const QString gcPath = resolveTilde(
            ToolUtils::resolvePath(input["generationConfigPath"].toString(), workDir));
        if (!gcPath.isEmpty()) {
            QFile gcFile(gcPath);
            if (gcFile.open(QIODevice::ReadOnly)) {
                const QJsonDocument gcDoc = QJsonDocument::fromJson(gcFile.readAll());
                if (!gcDoc.isNull())
                    applyGenerationConfig(gcDoc.object(), profile);
            }
        }

        // Write to ~/.codehex/profiles/{id}.json
        const QString profilesDir = QDir::homePath() + "/.codehex/profiles";
        QDir().mkpath(profilesDir);
        const QString outPath = profilesDir + "/" + profile.id + ".json";

        QFile out(outPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return ToolUtils::errResult(
                QString("ImportModelProfile: cannot write '%1'").arg(outPath));

        out.write(QJsonDocument(profile.toJson()).toJson(QJsonDocument::Indented));

        return ToolUtils::okResult(
            QString("Saved profile '%1' (%2) → %3\n"
                    "  contextWindow   : %4 tokens\n"
                    "  toolCallFormat  : %5\n"
                    "  toolRespFormat  : %6\n"
                    "  thoughtTags     : %7 ... %8\n"
                    "  matchPatterns   : [%9]\n"
                    "  generation      : temp=%10  top_p=%11  top_k=%12")
            .arg(profile.id, profile.displayName, outPath)
            .arg(profile.contextWindow)
            .arg(profile.toolCallFormat, profile.toolResponseFormat)
            .arg(profile.thoughtOpenTag, profile.thoughtCloseTag)
            .arg(profile.matchPatterns.join(", "))
            .arg(profile.temperature).arg(profile.topP).arg(profile.topK)
        );
    }

private:
    // ── Profile detection from tokenizer_config ──────────────────────────────

    ModelProfile detectProfile(const QJsonObject& tc) {
        ModelProfile p;
        const QString chatTemplate   = tc["chat_template"].toString();
        const QString tokenizerClass = tc["tokenizer_class"].toString();

        // Collect all special token strings for quick membership tests
        QStringList allTokens;
        for (const auto& v : tc["extra_special_tokens"].toArray())
            allTokens << v.toString();
        const QJsonObject addedDecoder = tc["added_tokens_decoder"].toObject();
        for (auto it = addedDecoder.constBegin(); it != addedDecoder.constEnd(); ++it)
            allTokens << it.value().toObject()["content"].toString();

        const QString bosContent = extractTokenContent(tc["bos_token"]);
        const QString eosContent = extractTokenContent(tc["eos_token"]);

        // ── Qwen (ChatML: <|im_start|>) ──────────────────────────────────────
        if (allTokens.contains("<|im_start|>") || chatTemplate.contains("<|im_start|>")) {
            p.id          = "qwen3";
            p.displayName = "Qwen3 / Qwen3-Coder (auto-imported)";
            p.matchPatterns   = {"qwen3", "qwen-coder", "qwen2.5-coder", "qwen2-coder"};
            p.toolCallFormat     = "qwen-xml";
            p.toolResponseFormat = "qwen-tool-role";
            p.thoughtOpenTag  = "<think>";
            p.thoughtCloseTag = "</think>";
            p.hasGenerationParams = true;
            p.temperature = 1.0;
            p.topP  = 0.95;
            p.topK  = 40;
        }
        // ── DeepSeek (Unicode full-width pipe separators) ─────────────────────
        else if (bosContent.contains("\uff5c") ||            // ｜ full-width
                 chatTemplate.contains("\uff5cUser\uff5c") || // <｜User｜>
                 chatTemplate.contains("tool\u25b2calls\u25b2begin")) {
            p.id          = "deepseek";
            p.displayName = "DeepSeek V3 / R1 / Coder (auto-imported)";
            p.matchPatterns   = {"deepseek"};
            p.toolCallFormat     = "deepseek-native";
            p.toolResponseFormat = "deepseek-output";
            p.thoughtOpenTag  = "<think>";
            p.thoughtCloseTag = "</think>";
            p.hasGenerationParams = true;
            p.temperature = 0.0;
            p.topP  = 1.0;
            p.topK  = -1;
        }
        // ── Mistral ([INST] / [TOOL_CALLS] special tokens) ───────────────────
        else if (allTokens.contains("[TOOL_CALLS]") ||
                 allTokens.contains("[INST]") ||
                 chatTemplate.contains("[INST]")) {
            p.id          = "mistral";
            p.displayName = "Mistral / Codestral / Pixtral (auto-imported)";
            p.matchPatterns   = {"mistral", "codestral", "pixtral", "nemo", "mixtral"};
            p.toolCallFormat     = "mistral-native";
            p.toolResponseFormat = "mistral-results";
            // Some Mistral variants use [THINK]/[/THINK] tokens
            p.thoughtOpenTag  = allTokens.contains("[THINK]")  ? "[THINK]"  : "<think>";
            p.thoughtCloseTag = allTokens.contains("[/THINK]") ? "[/THINK]" : "</think>";
            p.hasGenerationParams = true;
            p.temperature = 0.7;
            p.topP  = 1.0;
            p.topK  = -1;
        }
        // ── LLaMA fallback ────────────────────────────────────────────────────
        else {
            p.id          = "llama";
            p.displayName = "LLaMA / LLaMA-2 / CodeLlama (auto-imported)";
            p.matchPatterns   = {"llama", "codellama", "code-llama"};
            p.toolCallFormat     = "codehex-xml";
            p.toolResponseFormat = "qwen-tool-role";
            p.thoughtOpenTag  = "<think>";
            p.thoughtCloseTag = "</think>";
            p.hasGenerationParams = false;
        }

        // ── Context window ────────────────────────────────────────────────────
        // model_max_length can be a huge sentinel (1e36) meaning "no limit" → cap to 1M
        const double rawMax = tc["model_max_length"].toDouble(32768.0);
        p.contextWindow = (rawMax > 1048576.0 || rawMax <= 0)
                          ? 32768
                          : static_cast<int>(rawMax);

        // EOS as stop sequence
        if (!eosContent.isEmpty())
            p.stopSequences = QStringList{eosContent};

        return p;
    }

    void applyGenerationConfig(const QJsonObject& gc, ModelProfile& p) {
        if (gc.contains("temperature")) {
            p.temperature = gc["temperature"].toDouble(p.temperature);
            p.hasGenerationParams = true;
        }
        if (gc.contains("top_p")) {
            p.topP = gc["top_p"].toDouble(p.topP);
            p.hasGenerationParams = true;
        }
        if (gc.contains("top_k")) {
            p.topK = gc["top_k"].toInt(p.topK);
            p.hasGenerationParams = true;
        }
    }

    static QString extractTokenContent(const QJsonValue& val) {
        if (val.isString()) return val.toString();
        if (val.isObject()) return val.toObject()["content"].toString();
        return {};
    }

    // Expand leading ~ to home directory
    static QString resolveTilde(const QString& path) {
        if (path.startsWith("~/"))
            return QDir::homePath() + path.mid(1);
        return path;
    }
};

} // namespace CodeHex
