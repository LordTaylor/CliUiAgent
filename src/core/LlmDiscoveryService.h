#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace CodeHex {

/**
 * @brief Service to discover available models from local or remote LLM providers.
 */
class LlmDiscoveryService : public QObject {
    Q_OBJECT
public:
    explicit LlmDiscoveryService(QObject* parent = nullptr);

    /**
     * @brief Fetch models from a provider URL.
     * @param url Base URL (e.g. http://localhost:11434 or https://api.openai.com/v1)
     * @param type Provider type ("ollama", "openai", etc.)
     * @param apiKey Optional API key for remote providers.
     */
    void fetchModels(const QString& url, const QString& type, const QString& apiKey = "");

signals:
    void modelsReady(const QStringList& models);
    void errorOccurred(const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_network;
    QString m_providerType;
};

} // namespace CodeHex
