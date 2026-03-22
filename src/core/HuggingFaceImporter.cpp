#include "HuggingFaceImporter.h"
#include "tools/ImportModelProfileTool.h"   // reuses detectProfile() indirectly via tool logic

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

namespace CodeHex {

// ── Reuse detection logic from ImportModelProfileTool ──────────────────────
// We instantiate the tool temporarily to call execute() is not practical here,
// so we duplicate the minimal detect+save logic via a small local helper.
namespace {

QString extractTokenContent(const QJsonValue& val) {
    if (val.isString()) return val.toString();
    if (val.isObject()) return val.toObject()["content"].toString();
    return {};
}

ModelProfile detectFromTokenizerConfig(const QJsonObject& tc) {
    ModelProfile p;
    const QString chatTemplate = tc["chat_template"].toString();

    QStringList allTokens;
    for (const auto& v : tc["extra_special_tokens"].toArray())
        allTokens << v.toString();
    const QJsonObject addedDecoder = tc["added_tokens_decoder"].toObject();
    for (auto it = addedDecoder.constBegin(); it != addedDecoder.constEnd(); ++it)
        allTokens << it.value().toObject()["content"].toString();

    const QString bosContent = extractTokenContent(tc["bos_token"]);
    const QString eosContent = extractTokenContent(tc["eos_token"]);

    if (allTokens.contains("<|im_start|>") || chatTemplate.contains("<|im_start|>")) {
        p.id = "qwen3"; p.displayName = "Qwen3 / Qwen3-Coder (HuggingFace)";
        p.matchPatterns   = {"qwen3", "qwen-coder", "qwen2.5-coder", "qwen2-coder"};
        p.toolCallFormat  = "qwen-xml"; p.toolResponseFormat = "qwen-tool-role";
        p.thoughtOpenTag  = "<think>"; p.thoughtCloseTag = "</think>";
        p.hasGenerationParams = true; p.temperature = 1.0; p.topP = 0.95; p.topK = 40;
    } else if (bosContent.contains("\uff5c") ||
               chatTemplate.contains("\uff5cUser\uff5c")) {
        p.id = "deepseek"; p.displayName = "DeepSeek V3 / R1 / Coder (HuggingFace)";
        p.matchPatterns   = {"deepseek"};
        p.toolCallFormat  = "deepseek-native"; p.toolResponseFormat = "deepseek-output";
        p.thoughtOpenTag  = "<think>"; p.thoughtCloseTag = "</think>";
        p.hasGenerationParams = true; p.temperature = 0.0; p.topP = 1.0; p.topK = -1;
    } else if (allTokens.contains("[TOOL_CALLS]") ||
               allTokens.contains("[INST]") ||
               chatTemplate.contains("[INST]")) {
        p.id = "mistral"; p.displayName = "Mistral / Codestral / Pixtral (HuggingFace)";
        p.matchPatterns   = {"mistral", "codestral", "pixtral", "nemo", "mixtral"};
        p.toolCallFormat  = "mistral-native"; p.toolResponseFormat = "mistral-results";
        p.thoughtOpenTag  = allTokens.contains("[THINK]")  ? "[THINK]"  : "<think>";
        p.thoughtCloseTag = allTokens.contains("[/THINK]") ? "[/THINK]" : "</think>";
        p.hasGenerationParams = true; p.temperature = 0.7; p.topP = 1.0; p.topK = -1;
    } else {
        p.id = "llama"; p.displayName = "LLaMA / LLaMA-2 / CodeLlama (HuggingFace)";
        p.matchPatterns   = {"llama", "codellama", "code-llama"};
        p.toolCallFormat  = "codehex-xml"; p.toolResponseFormat = "qwen-tool-role";
        p.thoughtOpenTag  = "<think>"; p.thoughtCloseTag = "</think>";
        p.hasGenerationParams = false;
    }

    const double rawMax = tc["model_max_length"].toDouble(32768.0);
    p.contextWindow = (rawMax > 1048576.0 || rawMax <= 0)
                      ? 32768 : static_cast<int>(rawMax);

    if (!eosContent.isEmpty())
        p.stopSequences = QStringList{eosContent};

    return p;
}

void applyGenerationConfig(const QJsonObject& gc, ModelProfile& p) {
    if (gc.contains("temperature")) { p.temperature = gc["temperature"].toDouble(p.temperature); p.hasGenerationParams = true; }
    if (gc.contains("top_p"))       { p.topP = gc["top_p"].toDouble(p.topP);     p.hasGenerationParams = true; }
    if (gc.contains("top_k"))       { p.topK = gc["top_k"].toInt(p.topK);        p.hasGenerationParams = true; }
}

} // anonymous namespace

// ── HuggingFaceImporter ────────────────────────────────────────────────────

HuggingFaceImporter::HuggingFaceImporter(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{}

void HuggingFaceImporter::fetchAndImport(const QString& repoId, const QString& apiToken) {
    if (repoId.isEmpty() || !repoId.contains('/')) {
        emit errorOccurred("Invalid repoId — expected 'Owner/RepoName' format");
        return;
    }

    emit progressMessage(QString("Downloading tokenizer_config.json from %1…").arg(repoId));

    const QString tcUrl = baseUrl(repoId) + "tokenizer_config.json";
    const QString gcUrl = baseUrl(repoId) + "generation_config.json";

    auto* tcReply = m_network->get(buildRequest(tcUrl, apiToken));

    connect(tcReply, &QNetworkReply::finished, this, [this, tcReply, gcUrl, apiToken, repoId]() {
        tcReply->deleteLater();

        if (tcReply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QString("Failed to download tokenizer_config.json: %1")
                               .arg(tcReply->errorString()));
            return;
        }

        const QByteArray tcData = tcReply->readAll();
        emit progressMessage("Downloading generation_config.json…");

        auto* gcReply = m_network->get(buildRequest(gcUrl, apiToken));
        connect(gcReply, &QNetworkReply::finished, this, [this, gcReply, tcData, repoId]() {
            gcReply->deleteLater();
            // generation_config.json is optional — ignore errors
            const QByteArray gcData = (gcReply->error() == QNetworkReply::NoError)
                                      ? gcReply->readAll() : QByteArray{};

            ModelProfile profile = buildProfile(tcData, gcData);
            QString savedPath;
            if (!saveProfile(profile, savedPath)) {
                emit errorOccurred(QString("Cannot save profile to %1").arg(savedPath));
                return;
            }
            emit progressMessage(QString("Profile '%1' saved.").arg(profile.id));
            emit profileImported(profile, savedPath);
        });
    });
}

bool HuggingFaceImporter::fetchAndImportSync(const QString& repoId,
                                              const QString& apiToken,
                                              ModelProfile&  outProfile,
                                              QString&       outError) {
    bool done = false;
    bool ok   = false;
    QEventLoop loop;

    auto* tmp = new HuggingFaceImporter(nullptr);

    QObject::connect(tmp, &HuggingFaceImporter::profileImported,
                     &loop, [&](const ModelProfile& p, const QString&) {
        outProfile = p; ok = true; done = true; loop.quit();
    });
    QObject::connect(tmp, &HuggingFaceImporter::errorOccurred,
                     &loop, [&](const QString& err) {
        outError = err; ok = false; done = true; loop.quit();
    });

    tmp->fetchAndImport(repoId, apiToken);

    // 30-second timeout
    QTimer::singleShot(30000, &loop, [&]() {
        if (!done) { outError = "Timeout downloading from HuggingFace"; ok = false; loop.quit(); }
    });

    loop.exec();
    tmp->deleteLater();
    return ok;
}

QNetworkRequest HuggingFaceImporter::buildRequest(const QString& url,
                                                   const QString& token) const {
    QNetworkRequest req{QUrl{url}};
    req.setHeader(QNetworkRequest::UserAgentHeader, "CodeHex/1.0");
    if (!token.isEmpty())
        req.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    return req;
}

ModelProfile HuggingFaceImporter::buildProfile(const QByteArray& tcData,
                                                const QByteArray& gcData) const {
    QJsonParseError err;
    const QJsonDocument tcDoc = QJsonDocument::fromJson(tcData, &err);
    if (tcDoc.isNull()) {
        qWarning() << "[HuggingFaceImporter] tokenizer_config.json parse error:" << err.errorString();
        return ModelProfile::defaultProfile();
    }

    ModelProfile p = detectFromTokenizerConfig(tcDoc.object());

    if (!gcData.isEmpty()) {
        const QJsonDocument gcDoc = QJsonDocument::fromJson(gcData);
        if (!gcDoc.isNull())
            applyGenerationConfig(gcDoc.object(), p);
    }
    return p;
}

bool HuggingFaceImporter::saveProfile(const ModelProfile& profile,
                                       QString& savedPath) const {
    const QString dir = QDir::homePath() + "/.codehex/profiles";
    QDir().mkpath(dir);
    savedPath = dir + "/" + profile.id + ".json";

    QFile f(savedPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    f.write(QJsonDocument(profile.toJson()).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace CodeHex
