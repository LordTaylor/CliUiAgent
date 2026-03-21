#pragma once
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>

namespace CodeHex {

/**
 * @brief Persists AI thinking/reasoning blocks across application restarts.
 * 
 * Roadmap Item #9: Cache persistence.
 */
class ThinkingCache {
public:
    static ThinkingCache& instance() {
        static ThinkingCache inst;
        return inst;
    }

    void insert(const QString& prompt, const QString& thought) {
        QString key = hash(prompt);
        m_cache[key] = thought;
        m_dirty = true;
    }

    QString lookup(const QString& prompt) const {
        return m_cache.value(hash(prompt));
    }

    void load(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return;
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            m_cache[it.key()] = it.value().toString();
        }
        m_storagePath = path;
        m_dirty = false;
    }

    void save() {
        if (!m_dirty || m_storagePath.isEmpty()) return;
        
        QJsonObject obj;
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            obj[it.key()] = it.value();
        }
        
        QFile file(m_storagePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(obj).toJson());
            m_dirty = false;
        }
    }

private:
    ThinkingCache() = default;
    
    QString hash(const QString& input) const {
        return QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256).toHex();
    }

    QMap<QString, QString> m_cache;
    QString m_storagePath;
    bool m_dirty = false;
};

} // namespace CodeHex
