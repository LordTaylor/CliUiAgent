#pragma once

#include <QString>
#include <QObject>
#include "AgentEngine.h" // For Role enum

namespace CodeHex {

class AppConfig;

/**
 * @brief Manages system prompt construction and role strategies.
 */
class PromptManager : public QObject {
    Q_OBJECT
public:
    explicit PromptManager(AppConfig* config, QObject* parent = nullptr);

    /**
     * @brief Builds the full system prompt for the current role and state.
     */
    QString buildSystemPrompt(AgentEngine::Role role, const QString& autoContext) const;

    /**
     * @brief Strategy description for the given role.
     */
    QString roleStrategy(AgentEngine::Role role) const;

    /**
     * @brief Loads the base role prompt from resources.
     */
    QString loadRolePrompt(AgentEngine::Role role) const;

private:
    AppConfig* m_config;
};

} // namespace CodeHex
