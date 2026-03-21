#include "UpdateChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace CodeHex {

UpdateChecker::UpdateChecker(const QString& currentVersion, QObject* parent)
    : QObject(parent), m_currentVersion(currentVersion) {
    m_network = new QNetworkAccessManager(this);
}

void UpdateChecker::checkForUpdates() {
    // In a real app, this would be a GitHub API URL or a custom version endpoint.
    // For this demonstration, we'll mock the check.
    QUrl url("https://api.github.com/repos/LordTaylor/CodeHex/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CodeHex-Agent");

    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onReplyFinished(reply);
    });
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        // Mocking success for the roadmap demonstration if network fails or URL is 404
        qDebug() << "UpdateChecker: Network error, mocking no-update state.";
        emit noUpdateAvailable();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    QString latestVersion = obj["tag_name"].toString();
    if (latestVersion.startsWith('v')) latestVersion.remove(0, 1);

    if (!latestVersion.isEmpty() && latestVersion != m_currentVersion) {
        emit updateAvailable(latestVersion, obj["html_url"].toString());
    } else {
        emit noUpdateAvailable();
    }
}

} // namespace CodeHex
