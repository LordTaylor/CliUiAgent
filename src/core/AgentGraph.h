/**
 * @file AgentGraph.h
 * @brief Graph-based orchestration for the agent.
 */
#pragma once
#include <QString>
#include <QMap>
#include <QList>
#include <QObject>
#include "AgentRole.h"

namespace CodeHex {

/**
 * @brief Represents the shared state of the agent during a graph-based task.
 * Similar to 'State' in LangGraph.
 */
struct AgentGraphState {
    QString task;
    QString plan;
    QString solution;
    bool hasErrors = false;
    int iteration = 0;
    QString nextNode; // Controls the flow between nodes
};

/**
 * @brief Orchestrates agent actions using a graph-based state machine.
 */
class AgentGraph : public QObject {
    Q_OBJECT
public:
    explicit AgentGraph(QObject* parent = nullptr);

    /** @brief Resets the graph for a new task. */
    void begin(const QString& task);

    /** @brief Executes the current node based on the state. */
    void step();

    /** @brief Updates state after a node completes. */
    void updateState(const AgentGraphState& newState);

    /** @brief Updates state based on LLM output and triggers next node. */
    void onNodeFinished(const QString& output);

    AgentGraphState state() const { return m_state; }
    bool isComplete() const { return m_state.nextNode == "END" || m_state.nextNode == "FINALIZE"; }

signals:
    void nodeStarted(const QString& nodeName, CodeHex::AgentRole role, const QString& prompt);
    void graphFinished(const QString& finalSolution);

private:
    AgentGraphState m_state;
    void route(); // Core routing logic
};

} // namespace CodeHex
