#include "AgentPipeline.h"
#include "AgentEngine.h"
#include <QDebug>
#include <QDateTime>
#include <QRegularExpression>

namespace CodeHex {

AgentPipeline::AgentPipeline(AgentEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {}

QList<AgentPipeline::Stage> AgentPipeline::buildPipeline(const QString& taskDescription) const {
    QList<Stage> stages;
    const QString lower = taskDescription.toLower();

    // Heuristic: detect complexity keywords to decide pipeline shape
    bool needsResearch = lower.contains("find") || lower.contains("search") ||
                         lower.contains("znajdź") || lower.contains("szukaj") ||
                         lower.contains("where") || lower.contains("analyze");
    bool needsImpl     = lower.contains("implement") || lower.contains("write") ||
                         lower.contains("add") || lower.contains("create") ||
                         lower.contains("napisz") || lower.contains("dodaj") ||
                         lower.contains("zaimplementuj");
    bool needsReview   = lower.contains("review") || lower.contains("check") ||
                         lower.contains("verify") || lower.contains("sprawdź") ||
                         lower.contains("test");
    bool needsDesign   = lower.contains("design") || lower.contains("architect") ||
                         lower.contains("plan") || lower.contains("zaprojektuj");

    // Complex task: 3+ matchCount → full pipeline
    int matchCount = (needsResearch ? 1 : 0) + (needsImpl ? 1 : 0) +
                  (needsReview ? 1 : 0) + (needsDesign ? 1 : 0);

    if (matchCount >= 2 || needsDesign) {
        // Full pipeline: Architect → Explorer → Executor → Reviewer
        if (needsDesign || matchCount >= 3) {
            stages.append({AgentRole::Architect,
                "Analyze the following task and create a step-by-step implementation plan. "
                "Identify key files, components, and dependencies.\n\nTask: " + taskDescription,
                {}, false});
        }

        if (needsResearch || matchCount >= 3) {
            stages.append({AgentRole::Explorer,
                "Based on the plan below, gather all necessary context from the codebase. "
                "Read relevant files, search for patterns, and identify existing code to modify.\n\n"
                "{{PREV_OUTPUT}}\n\nOriginal task: " + taskDescription,
                {}, false});
        }

        if (needsImpl) {
            stages.append({AgentRole::Executor,
                "Implement the changes described below. Use the gathered context to make "
                "accurate modifications.\n\n{{PREV_OUTPUT}}\n\nOriginal task: " + taskDescription,
                {}, false});
        }

        if (needsReview || matchCount >= 3) {
            stages.append({AgentRole::Reviewer,
                "Review the changes made below. Check for correctness, edge cases, and "
                "potential issues. Verify the implementation matches the original plan.\n\n"
                "{{PREV_OUTPUT}}\n\nOriginal task: " + taskDescription,
                {}, false});
        }
    }

    // Simple task or no clear matchCount — single stage with auto-detected role
    if (stages.isEmpty()) {
        stages.append({AgentRole::Base, taskDescription, {}, false});
    }

    return stages;
}

void AgentPipeline::execute(const QList<Stage>& stages) {
    if (stages.isEmpty()) return;

    m_stages = stages;
    m_currentStage = -1;
    m_running = true;

    qInfo() << "[AgentPipeline] Starting pipeline with" << m_stages.size() << "stages";
    runNextStage();
}

void AgentPipeline::runNextStage() {
    ++m_currentStage;

    if (m_currentStage >= m_stages.size()) {
        // Pipeline complete
        m_running = false;
        QString finalOutput = m_stages.last().output;
        qInfo() << "[AgentPipeline] Pipeline complete.";
        emit pipelineComplete(finalOutput);
        return;
    }

    auto& stage = m_stages[m_currentStage];

    // Substitute {{PREV_OUTPUT}} with previous stage's output
    if (m_currentStage > 0 && !m_stages[m_currentStage - 1].output.isEmpty()) {
        stage.prompt.replace("{{PREV_OUTPUT}}",
            "### Previous Stage Output:\n" + m_stages[m_currentStage - 1].output);
    } else {
        stage.prompt.replace("{{PREV_OUTPUT}}", "(no previous output)");
    }

    qInfo() << "[AgentPipeline] Stage" << m_currentStage + 1 << "/" << m_stages.size()
            << "role:" << static_cast<int>(stage.role);

    emit stageStarted(m_currentStage, stage.role, stage.prompt);

    // Set the engine role and process
    m_engine->setRole(stage.role);
    m_engine->process(stage.prompt);
}

void AgentPipeline::onStageComplete(const QString& output) {
    if (!m_running || m_currentStage < 0 || m_currentStage >= m_stages.size()) return;

    m_stages[m_currentStage].output = output;
    m_stages[m_currentStage].completed = true;

    emit stageFinished(m_currentStage, output);

    // Continue to next stage
    runNextStage();
}

void AgentPipeline::abort() {
    m_running = false;
    m_engine->stop();
    emit pipelineAborted();
}

} // namespace CodeHex
