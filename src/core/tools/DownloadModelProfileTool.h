#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include "../HuggingFaceImporter.h"
#include <QJsonArray>

namespace CodeHex {

/**
 * @brief Agent-callable tool that downloads tokenizer_config.json (and
 *        generation_config.json) from HuggingFace Hub and auto-creates a
 *        ModelProfile saved to ~/.codehex/profiles/{id}.json.
 *
 * Usage (agent):
 *   DownloadModelProfile({"repoId": "Qwen/Qwen3-Coder-Next"})
 *   DownloadModelProfile({"repoId": "deepseek-ai/DeepSeek-V3", "apiToken": "hf_..."})
 *
 * Hot-reloaded by ModelProfileManager — no restart required.
 */
class DownloadModelProfileTool : public Tool {
public:
    QString name() const override { return "DownloadModelProfile"; }
    QString description() const override {
        return "Downloads tokenizer_config.json and generation_config.json from HuggingFace "
               "Hub and auto-creates a ModelProfile in ~/.codehex/profiles/{id}.json "
               "(hot-reloaded instantly). Input: repoId as 'Owner/RepoName', "
               "e.g. 'Qwen/Qwen3-Coder-Next' or 'deepseek-ai/DeepSeek-V3'.";
    }

    QJsonObject parameters() const override {
        return QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"repoId", QJsonObject{
                    {"type", "string"},
                    {"description", "HuggingFace repo in 'Owner/RepoName' format (e.g. 'Qwen/Qwen3-Coder-Next')"}
                }},
                {"apiToken", QJsonObject{
                    {"type", "string"},
                    {"description", "(Optional) HuggingFace API token for private repositories"}
                }},
                {"profileId", QJsonObject{
                    {"type", "string"},
                    {"description", "(Optional) Override the auto-detected profile id"}
                }}
            }},
            {"required", QJsonArray{"repoId"}}
        };
    }

    ToolResult execute(const QJsonObject& input, const QString& /*workDir*/) override {
        const QString repoId   = input["repoId"].toString().trimmed();
        const QString apiToken = input["apiToken"].toString().trimmed();

        if (repoId.isEmpty() || !repoId.contains('/'))
            return ToolUtils::errResult(
                "DownloadModelProfile: 'repoId' must be in 'Owner/RepoName' format");

        ModelProfile profile;
        QString      error;

        HuggingFaceImporter importer;
        const bool ok = importer.fetchAndImportSync(repoId, apiToken, profile, error);

        if (!ok)
            return ToolUtils::errResult(
                QString("DownloadModelProfile: %1").arg(error));

        // Override id if requested
        if (!input["profileId"].toString().trimmed().isEmpty())
            profile.id = input["profileId"].toString().trimmed();

        return ToolUtils::okResult(
            QString("Downloaded and saved profile '%1' (%2)\n"
                    "  from          : https://huggingface.co/%3\n"
                    "  contextWindow : %4 tokens\n"
                    "  toolCallFormat: %5\n"
                    "  toolRespFormat: %6\n"
                    "  thoughtTags   : %7 ... %8\n"
                    "  matchPatterns : [%9]\n"
                    "  generation    : temp=%10  top_p=%11  top_k=%12")
            .arg(profile.id, profile.displayName, repoId)
            .arg(profile.contextWindow)
            .arg(profile.toolCallFormat, profile.toolResponseFormat)
            .arg(profile.thoughtOpenTag, profile.thoughtCloseTag)
            .arg(profile.matchPatterns.join(", "))
            .arg(profile.temperature).arg(profile.topP).arg(profile.topK)
        );
    }
};

} // namespace CodeHex
