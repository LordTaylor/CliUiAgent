#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../data/ModelProfile.h"

namespace CodeHex {

/**
 * @brief Downloads tokenizer_config.json (and generation_config.json) from
 *        HuggingFace Hub and creates a ModelProfile saved to
 *        ~/.codehex/profiles/{id}.json (hot-reloaded by ModelProfileManager).
 *
 * Usage:
 *   auto* imp = new HuggingFaceImporter(this);
 *   connect(imp, &HuggingFaceImporter::profileImported, ...);
 *   imp->fetchAndImport("Qwen/Qwen3-Coder-Next");
 *
 * Public repos: no apiToken needed.
 * Private repos: pass a HuggingFace access token.
 */
class HuggingFaceImporter : public QObject {
    Q_OBJECT
public:
    explicit HuggingFaceImporter(QObject* parent = nullptr);

    /**
     * Asynchronously downloads tokenizer_config.json + generation_config.json
     * from huggingface.co/{repoId}/resolve/main/ and saves a ModelProfile.
     * @param repoId  "Owner/RepoName" e.g. "Qwen/Qwen3-Coder-Next"
     * @param apiToken  HuggingFace token (empty for public repos)
     */
    void fetchAndImport(const QString& repoId, const QString& apiToken = {});

    /** Synchronous convenience wrapper (runs a local QEventLoop — call from non-GUI threads). */
    bool fetchAndImportSync(const QString& repoId,
                            const QString& apiToken,
                            ModelProfile&  outProfile,
                            QString&       outError);

signals:
    void progressMessage(const QString& msg);
    void profileImported(const ModelProfile& profile, const QString& savedPath);
    void errorOccurred(const QString& error);

private:
    QNetworkAccessManager* m_network;

    static QString baseUrl(const QString& repoId) {
        return QString("https://huggingface.co/%1/resolve/main/").arg(repoId);
    }

    QNetworkRequest buildRequest(const QString& url, const QString& token) const;
    ModelProfile    buildProfile(const QByteArray& tcData,
                                 const QByteArray& gcData) const;
    bool            saveProfile(const ModelProfile& profile,
                                QString& savedPath) const;
};

} // namespace CodeHex
