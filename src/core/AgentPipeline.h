#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "AgentRole.h"

namespace CodeHex {

class AgentEngine;

/**
 * @brief P-4: Orchestrates a multi-stage pipeline of agent roles.
 *
 * Complex tasks are decomposed into stages, each handled by a specialized role.
 * Each stage's output becomes context for the next stage.
 *
 * Example pipeline:
 *   Architect (plan) → Explorer (gather context) → Executor (implement) → Reviewer (verify)
 */
class AgentPipeline : public QObject {
    Q_OBJECT
public:
    struct Stage {
        AgentRole role;
        QString   prompt;     // template — may contain {{PREV_OUTPUT}} placeholder
        QString   output;     // filled after execution
        bool      completed = false;
    };

    explicit AgentPipeline(AgentEngine* engine, QObject* parent = nullptr);

    /** @brief Builds a pipeline from a high-level task description using heuristics. */
    QList<Stage> buildPipeline(const QString& taskDescription) const;

    /** @brief Starts executing the given pipeline stages sequentially. */
    void execute(const QList<Stage>& stages);

    /** @brief Called when the current stage's LLM response arrives. */
    void onStageComplete(const QString& output);

    /** @brief Returns true if the pipeline is currently running. */
    bool isRunning() const { return m_running; }

    /** @brief Aborts the current pipeline. */
    void abort();

    int currentStageIndex() const { return m_currentStage; }
    int totalStages() const { return m_stages.size(); }

signals:
    void stageStarted(int index, CodeHex::AgentRole role, const QString& prompt);
    void stageFinished(int index, const QString& output);
    void pipelineComplete(const QString& finalOutput);
    void pipelineAborted();

private:
    void runNextStage();

    AgentEngine* m_engine;
    QList<Stage> m_stages;
    int  m_currentStage = -1;
    bool m_running = false;
};

} // namespace CodeHex
