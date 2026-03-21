#pragma once
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace CodeHex {

/**
 * @brief Simple utility to check for application updates.
 * 
 * Roadmap Item #36: Update checker.
 */
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(const QString& currentVersion, QObject* parent = nullptr);

    void checkForUpdates();

signals:
    void updateAvailable(const QString& newVersion, const QString& url);
    void noUpdateAvailable();
    void errorOccurred(const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_currentVersion;
    QNetworkAccessManager* m_network;
};

} // namespace CodeHex
