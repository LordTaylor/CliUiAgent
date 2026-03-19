#include "SessionManager.h"
#include <QDir>
#include "AppConfig.h"

namespace CodeHex {

SessionManager::SessionManager(AppConfig* config, QObject* parent)
    : QObject(parent), m_config(config) {}

SessionManager::~SessionManager() {
    qDeleteAll(m_sessions);
}

void SessionManager::loadAll() {
    qDeleteAll(m_sessions);
    m_sessions.clear();
    m_current = nullptr;

    QDir dir(m_config->sessionsDir());
    for (const QString& fname : dir.entryList({"*.json"}, QDir::Files, QDir::Time)) {
        auto* s = new Session(Session::load(dir.filePath(fname)));
        if (!s->id.isNull()) {
            m_sessions.append(s);
        } else {
            delete s;
        }
    }
    emit sessionsLoaded();
}

Session* SessionManager::createSession(const QString& profileName, const QString& modelName) {
    auto* s = new Session(Session::createNew(profileName, modelName));
    s->filePath = sessionFilePath(s->id.toString(QUuid::WithoutBraces));
    s->save();
    m_sessions.prepend(s);
    emit sessionCreated(s->id.toString(QUuid::WithoutBraces));
    return s;
}

Session* SessionManager::openSession(const QString& id) {
    for (auto* s : m_sessions) {
        if (s->id.toString(QUuid::WithoutBraces) == id) return s;
    }
    // Try loading from disk if not in memory
    const QString path = sessionFilePath(id);
    if (QFile::exists(path)) {
        auto* s = new Session(Session::load(path));
        if (!s->id.isNull()) {
            m_sessions.append(s);
            return s;
        }
        delete s;
    }
    return nullptr;
}

void SessionManager::deleteSession(const QString& id) {
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i]->id.toString(QUuid::WithoutBraces) == id) {
            QFile::remove(m_sessions[i]->filePath);
            if (m_current == m_sessions[i]) {
                m_current = nullptr;
                emit currentSessionChanged(nullptr);
            }
            delete m_sessions.takeAt(i);
            emit sessionDeleted(id);
            return;
        }
    }
}

QList<Session*> SessionManager::allSessions() const { return m_sessions; }
Session* SessionManager::currentSession() const { return m_current; }

void SessionManager::setCurrentSession(Session* s) {
    if (m_current == s) return;
    m_current = s;
    emit currentSessionChanged(s);
}

QString SessionManager::sessionFilePath(const QString& id) const {
    return m_config->sessionsDir() + "/" + id + ".json";
}

}  // namespace CodeHex
