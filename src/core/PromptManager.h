#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "AgentEngine.h"
#include "Message.h"

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
     * @brief Builds a structured JSON request for Claude Code API.
     */
    QJsonObject buildRequestJson(AgentEngine::Role role, 
                               const QString& userInput, 
                               const QList<Message>& history, 
                               const QJsonArray& tools,
                               int thinkingBudget = 0,
                               bool useCache = false) const;

    /**
     * @brief Strategy description for the given role.
     */
    QString roleStrategy(AgentEngine::Role role) const;

    /**
     * @brief Analyzes the query for implicit goals and returns a context string.
     */
    QString detectImplicitGoals(const QString& query) const;

    /**
     * @brief Loads the base role prompt from resources.
     */
    QString loadRolePrompt(AgentEngine::Role role) const;

private:
    AppConfig* m_config;
};

} // namespace CodeHex
