#pragma once
#include <QObject>
#include <QString>
#include <QMap>
#include "AgentRole.h"

namespace CodeHex {

class AppConfig;

/**
 * @brief Coordinates which LLM model/profile to use for a given role or sub-task.
 */
class ModelRouter : public QObject {
    Q_OBJECT
public:
    explicit ModelRouter(AppConfig* config, QObject* parent = nullptr);

    /**
     * @brief Returns a profile ID (from AppConfig) for the given role.
     */
    QString getProfileIdForRole(AgentRole role) const;

    /**
     * @brief Overrides the profile for a specific role.
     */
    void setProfileIdForRole(AgentRole role, const QString& profileId);

private:
    AppConfig* m_config;
    QMap<AgentRole, QString> m_roleMap;
    
    void loadDefaults();
};

} // namespace CodeHex
