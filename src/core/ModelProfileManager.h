#pragma once
#include <QObject>
#include <QList>
#include <QFileSystemWatcher>
#include "../data/ModelProfile.h"

namespace CodeHex {

class AppConfig;

/**
 * @brief Manages per-model JSON profiles — loads, hot-reloads, and matches them.
 *
 * Loading priority (higher index wins):
 *   1. Builtin profiles embedded in the app (resources/profiles/ in QRC)
 *   2. User/agent profiles on disk (~/.codehex/profiles/)
 *
 * When the agent writes a new profile via WriteFileTool, QFileSystemWatcher
 * detects the change and emits profilesChanged() — no restart required.
 */
class ModelProfileManager : public QObject {
    Q_OBJECT
public:
    explicit ModelProfileManager(AppConfig* config, QObject* parent = nullptr);

    /**
     * Find the best-matching profile for a model name.
     * Matching is done case-insensitively against ModelProfile::matchPatterns.
     * Returns ModelProfile::defaultProfile() if no profile matches.
     */
    ModelProfile findProfile(const QString& modelName) const;

    /** Save a profile to the user profiles dir (triggers hot-reload via watcher). */
    bool saveProfile(const ModelProfile& profile);

    QList<ModelProfile> allProfiles() const { return m_profiles; }

    /** Re-read all profile files from disk. Called automatically by watcher. */
    void reload();

signals:
    void profilesChanged();

private:
    AppConfig*          m_config;
    QList<ModelProfile> m_profiles;
    QFileSystemWatcher* m_watcher = nullptr;

    void loadBuiltinProfiles();
    void loadUserProfiles();
    void loadProfileFile(const QString& path);
    void upsertProfile(const ModelProfile& profile);
};

} // namespace CodeHex
