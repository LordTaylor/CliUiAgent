/**
 * @file AgentGraph.cpp
 * @brief Implementation of the agent orchestration graph.
 */
/**
 * @file AgentGraph.cpp
 * @brief Implementation of the agent orchestration graph.
 */
#include "AgentGraph.h"
#include <QDebug>

namespace CodeHex {

AgentGraph::AgentGraph(QObject* parent) : QObject(parent) {
    m_state.nextNode = "START";
}

void AgentGraph::begin(const QString& task) {
    m_state = AgentGraphState();
    m_state.task = task;
    m_state.nextNode = "PLANNER"; // Entry point
    qInfo() << "[AgentGraph] Beginning task:" << task;
}

void AgentGraph::step() {
    if (isComplete()) return;

    QString node = m_state.nextNode;
    AgentRole role = AgentRole::Base;
    QString prompt;

    if (node == "PLANNER") {
        role = AgentRole::Architect;
        prompt = "### TASK: " + m_state.task + "\n\n"
                 "Analyze the request and create a detailed, step-by-step implementation plan. "
                 "Identify which files need to be created or modified, and what tools will be required.";
    } 
    else if (node == "EXECUTOR") {
        role = AgentRole::Executor;
        prompt = "### CURRENT PLAN:\n" + m_state.plan + "\n\n"
                 "Perform the implementation as described in the plan. Use the available tools "
                 "to read and write files. If you encounter issues, explain them clearly.";
        if (m_state.hasErrors) {
            prompt += "\n\n### PREVIOUS ATTEMPT FAILED. FIX THE FOLLOWING ERRORS:\n" + m_state.solution +
                      "\n\nRefine your approach to solve these issues.";
        }
    }
    else if (node == "VERIFIER") {
        role = AgentRole::Reviewer;
        prompt = "### IMPLEMENTATION TO VERIFY:\n" + m_state.solution + "\n\n"
                 "Review the code changes and verify they solve the original task: " + m_state.task + "\n"
                 "Check for correctness, potential bugs, and adherence to requirements.\n"
                 "IF EVERYTHING IS CORRECT, state 'SUCCESS'.\n"
                 "IF THERE ARE ERRORS, start your response with 'ERROR:' and describe what needs fixing.";
    }

    emit nodeStarted(node, role, prompt);
}

void AgentGraph::onNodeFinished(const QString& output) {
    if (m_state.nextNode == "PLANNER") {
        m_state.plan = output;
    } 
    else if (m_state.nextNode == "EXECUTOR") {
        m_state.solution = output;
    }
    else if (m_state.nextNode == "VERIFIER") {
        // Simple heuristic: check for error-like keywords in the verification output
        m_state.hasErrors = output.contains("FAIL", Qt::CaseInsensitive) || 
                            output.contains("ERROR", Qt::CaseInsensitive) ||
                            output.contains("BŁĄD", Qt::CaseInsensitive); // User-suggested Polish keyword
        m_state.iteration++;
    }

    route();
    if (!isComplete()) {
        step();
    } else {
        emit graphFinished(m_state.solution);
    }
}

void AgentGraph::updateState(const AgentGraphState& newState) {
    m_state = newState;
    route();
}

void AgentGraph::route() {
    // Dynamic routing logic
    if (m_state.nextNode == "PLANNER") {
        m_state.nextNode = "EXECUTOR";
    }
    else if (m_state.nextNode == "EXECUTOR") {
        m_state.nextNode = "VERIFIER";
    }
    else if (m_state.nextNode == "VERIFIER") {
        if (m_state.hasErrors && m_state.iteration < 3) {
            m_state.nextNode = "EXECUTOR"; // Loop back to fix errors
            qInfo() << "[AgentGraph] Verification failed. Routing back to EXECUTOR. Iteration:" << m_state.iteration;
        } else {
            m_state.nextNode = "END";
            qInfo() << "[AgentGraph] Verification successful (or max iterations reached). Finishing.";
        }
    }
}

} // namespace CodeHex
