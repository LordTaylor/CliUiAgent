#include "ModelProfileManager.h"
#include "AppConfig.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace CodeHex {

ModelProfileManager::ModelProfileManager(AppConfig* config, QObject* parent)
    : QObject(parent), m_config(config)
{
    // Load builtin profiles first (lower priority)
    loadBuiltinProfiles();
    // Load user profiles (higher priority — may override builtins)
    loadUserProfiles();

    // Watch ~/.codehex/profiles/ for hot-reload
    const QString dir = m_config->profilesDir();
    QDir().mkpath(dir);

    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(dir);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString&) {
        reload();
        emit profilesChanged();
    });
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString&) {
        reload();
        emit profilesChanged();
    });

    qDebug() << "[ModelProfileManager] Loaded" << m_profiles.size() << "profiles.";
}

void ModelProfileManager::loadBuiltinProfiles() {
    // Embedded profiles compiled into the app via resources.qrc
    const QStringList builtins = {
        ":/resources/profiles/qwen3.json",
        ":/resources/profiles/deepseek.json",
        ":/resources/profiles/mistral.json",
        ":/resources/profiles/llama.json"
    };
    for (const auto& path : builtins)
        loadProfileFile(path);
}

void ModelProfileManager::loadUserProfiles() {
    const QString dir = m_config->profilesDir();
    QDir d(dir);
    if (!d.exists()) return;

    for (const QString& fname : d.entryList({"*.json"}, QDir::Files)) {
        loadProfileFile(d.filePath(fname));
        // Track individual files for change notifications
        if (m_watcher && !m_watcher->files().contains(d.filePath(fname)))
            m_watcher->addPath(d.filePath(fname));
    }
}

void ModelProfileManager::loadProfileFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ModelProfileManager] Cannot open:" << path;
        return;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[ModelProfileManager] Invalid JSON in" << path << "—" << err.errorString();
        return;
    }
    ModelProfile profile = ModelProfile::fromJson(doc.object());
    if (profile.id.isEmpty()) {
        qWarning() << "[ModelProfileManager] Profile missing 'id' in" << path;
        return;
    }
    upsertProfile(profile);
    qDebug() << "[ModelProfileManager] Loaded profile:" << profile.id << "from" << path;
}

void ModelProfileManager::upsertProfile(const ModelProfile& profile) {
    // Replace existing profile with same id, or append
    for (auto& p : m_profiles) {
        if (p.id == profile.id) {
            p = profile;
            return;
        }
    }
    m_profiles.append(profile);
}

void ModelProfileManager::reload() {
    qDebug() << "[ModelProfileManager] Hot-reloading profiles...";
    m_profiles.clear();
    loadBuiltinProfiles();
    loadUserProfiles();
    qDebug() << "[ModelProfileManager] After reload:" << m_profiles.size() << "profiles.";
}

ModelProfile ModelProfileManager::findProfile(const QString& modelName) const {
    const QString lower = modelName.toLower();

    // User profiles were loaded after builtins — iterate in reverse so user
    // profiles (appended last) take precedence when there's a pattern conflict.
    for (int i = m_profiles.size() - 1; i >= 0; --i) {
        if (m_profiles[i].matches(lower))
            return m_profiles[i];
    }
    return ModelProfile::defaultProfile();
}

bool ModelProfileManager::saveProfile(const ModelProfile& profile) {
    if (profile.id.isEmpty()) {
        qWarning() << "[ModelProfileManager] Cannot save profile with empty id.";
        return false;
    }
    const QString dir  = m_config->profilesDir();
    const QString path = dir + "/" + profile.id + ".json";
    QDir().mkpath(dir);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "[ModelProfileManager] Cannot write:" << path;
        return false;
    }
    f.write(QJsonDocument(profile.toJson()).toJson(QJsonDocument::Indented));
    qDebug() << "[ModelProfileManager] Saved profile:" << profile.id << "→" << path;
    // QFileSystemWatcher will pick up the change and call reload() automatically.
    return true;
}

} // namespace CodeHex
