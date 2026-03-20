#include "LlmDiscoveryService.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

namespace CodeHex {

LlmDiscoveryService::LlmDiscoveryService(QObject* parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this)) {
    connect(m_network, &QNetworkAccessManager::finished, this, &LlmDiscoveryService::onReplyFinished);
}

void LlmDiscoveryService::fetchModels(const QString& url, const QString& apiKey) {
    m_currentUrl = url;
    QUrl qurl;
    
    // Auto-detect provider endpoint
    if (url.contains("11434") || url.toLower().contains("ollama")) {
        qurl = QUrl(url + "/api/tags");
    } else {
        qurl = QUrl(url + "/models"); // Standard OpenAI-style is /v1/models (but v1 is often in the base URL)
    }

    QNetworkRequest request(qurl);
    if (!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    }
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_network->get(request);
}

void LlmDiscoveryService::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QStringList models;

    if (m_currentUrl.contains("11434") || m_currentUrl.toLower().contains("ollama")) {
        // Parse Ollama response
        QJsonObject root = doc.object();
        QJsonArray modelArr = root["models"].toArray();
        for (const auto& m : modelArr) {
            models << m.toObject()["name"].toString();
        }
    } else {
        // Parse OpenAI-style response
        QJsonObject root = doc.object();
        QJsonArray modelArr = root["data"].toArray();
        for (const auto& m : modelArr) {
            models << m.toObject()["id"].toString();
        }
    }

    if (models.isEmpty()) {
        emit errorOccurred("No models found at this endpoint.");
    } else {
        models.sort();
        emit modelsReady(models);
    }
}

} // namespace CodeHex
