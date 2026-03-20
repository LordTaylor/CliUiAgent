#include "EnsembleManager.h"
#include "AgentEngine.h"
#include "AppConfig.h"
#include "../cli/CliRunner.h"
#include "../cli/ConfigurableProfile.h"
#include "../data/LlmProvider.h"
#include <QDebug>
#include <QTimer>
#include <QString>
#include <QStringList>

namespace CodeHex {

EnsembleManager::EnsembleManager(AppConfig* config, QObject* parent)
    : QObject(parent), m_config(config)
{
}

void EnsembleManager::runEnsemble(const QString& prompt, 
                                 const QStringList& modelIds, 
                                 const QString& workDir,
                                 const QList<Message>& history,
                                 const QString& systemPrompt)
{
    m_responses.clear();
    m_pendingModels = modelIds.size();
    m_workDir = workDir;
    m_history = history;
    m_originalPrompt = prompt;
    m_systemPrompt = systemPrompt;
    
    if (m_pendingModels == 0) {
        emit ensembleError("No models selected for ensemble.");
        return;
    }

    for (const QString& id : modelIds) {
        auto* runner = new CliRunner(this);
        auto provider = findProviderById(id);
        if (provider.id.isEmpty()) {
            qWarning() << "[EnsembleManager] Provider not found:" << id;
            m_pendingModels--;
            delete runner;
            continue;
        }

        auto profile = ConfigurableProfile::fromProvider(provider);
        runner->setProfile(std::move(profile));

        connect(runner, &CliRunner::outputChunk, this, [this, id](const QString& chunk) {
            m_responses[id] += chunk;
        });

        connect(runner, &CliRunner::finished, this, [this, id, runner](int exitCode) {
            Q_UNUSED(exitCode);
            m_pendingModels--;
            if (m_pendingModels == 0) {
                synthesize();
            }
            runner->deleteLater();
        });
        
        runner->send(prompt, workDir, {}, history, systemPrompt);
    }
}

void EnsembleManager::synthesize()
{
    QString synthesisPrompt = "You are a master reasoning engine. I have queried multiple LLMs with the following prompt:\n\n";
    synthesisPrompt += "### ORIGINAL PROMPT:\n" + m_originalPrompt + "\n\n";
    synthesisPrompt += "### INDIVIDUAL MODEL RESPONSES:\n";

    for (auto it = m_responses.begin(); it != m_responses.end(); ++it) {
        synthesisPrompt += QString("\n--- Model: %1 ---\n%2\n").arg(it.key()).arg(it.value());
    }

    synthesisPrompt += "\n### TASK:\nSynthesize these responses into a single, highly accurate, and coherent answer. "
                       "If there are contradictions, resolve them using logic and best practices. "
                       "If tool calls were suggested, ensure the final response includes the correct tool calls in the canonical XML format.";

    // Call "Master" model (using current active provider as master for now)
    auto* masterRunner = new CliRunner(this);
    auto masterProvider = m_config->activeProvider();
    auto profile = ConfigurableProfile::fromProvider(masterProvider);
    masterRunner->setProfile(std::move(profile));

    connect(masterRunner, &CliRunner::outputChunk, this, [this](const QString& chunk) {
        m_synthesized += chunk;
    });

    connect(masterRunner, &CliRunner::finished, this, [this, masterRunner](int exitCode) {
        Q_UNUSED(exitCode);
        emit responseReady(m_synthesized);
        masterRunner->deleteLater();
    });

    masterRunner->send(synthesisPrompt, m_workDir, {}, m_history, m_systemPrompt);
}

LlmProvider EnsembleManager::findProviderById(const QString& id) const {
    for (const auto& p : m_config->providers()) {
        if (p.id == id) return p;
    }
    return {};
}

} // namespace CodeHex
