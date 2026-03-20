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

void LlmDiscoveryService::fetchModels(const QString& url, const QString& type, const QString& apiKey) {
    m_providerType = type; // Store type for parsing
    QUrl qurl;
    
    if (type == "ollama") {
        QString cleanUrl = url;
        if (cleanUrl.endsWith("/")) cleanUrl.chop(1);
        qurl = QUrl(cleanUrl + "/api/tags");
    } else {
        QString cleanUrl = url;
        if (cleanUrl.endsWith("/")) cleanUrl.chop(1);
        qurl = QUrl(cleanUrl + "/models");
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

    if (m_providerType == "ollama") {
        QJsonObject root = doc.object();
        QJsonArray modelArr = root["models"].toArray();
        for (const auto& m : modelArr) {
            models << m.toObject()["name"].toString();
        }
    } else {
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
