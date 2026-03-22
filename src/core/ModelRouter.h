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

    /**
     * @brief Detects the most appropriate agent role based on keyword heuristics.
     * Returns AgentRole::Base if no clear match.
     */
    AgentRole detectRoleFromPrompt(const QString& userInput) const;

    /**
     * @brief Returns scores for all roles based on keyword matches.
     * Use for debugging/UI display of detection confidence.
     */
    QMap<AgentRole, int> scoreRolesFromPrompt(const QString& userInput) const;

    /**
     * @brief Returns the set of tool names available for the given role.
     * Empty list means all tools are allowed (Base/Executor).
     */
    QStringList allowedToolsForRole(AgentRole role) const;

    /**
     * @brief Returns the default technique names to activate for a given role.
     * These are auto-activated when the role is set/auto-detected.
     */
    QStringList defaultTechniquesForRole(AgentRole role) const;

private:
    AppConfig* m_config;
    QMap<AgentRole, QString> m_roleMap;

    void loadDefaults();
};

} // namespace CodeHex
