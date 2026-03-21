#pragma once
#include <QList>
#include <QObject>
#include <QString>
#include "../data/Session.h"

namespace CodeHex {

class AppConfig;

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(AppConfig* config, QObject* parent = nullptr);
    ~SessionManager() override;

    void loadAll();
    Session* createSession(const QString& profileName, const QString& modelName);
    Session* openSession(const QString& id);
    void deleteSession(const QString& id);

    QList<Session*> allSessions() const;
    Session* currentSession() const;
    void setCurrentSession(Session* s);

    struct SearchResult {
        QString sessionId;
        QString sessionTitle;
        QString messageText;
    };
    QList<SearchResult> searchAllSessions(const QString& query) const;
    void autoArchiveOldSessions();

signals:
    void sessionCreated(const QString& id);
    void sessionDeleted(const QString& id);
    void currentSessionChanged(Session* session);
    void sessionsLoaded();
    void sessionRenamed(const QString& id, const QString& title);

private:
    QString sessionFilePath(const QString& id) const;

    AppConfig* m_config;
    QList<Session*> m_sessions;
    Session* m_current = nullptr;
};

}  // namespace CodeHex
