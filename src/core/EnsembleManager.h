#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include "../data/LlmProvider.h"
#include "../data/Message.h"
#include "AppConfig.h"

namespace CodeHex {

class CliRunner;

class EnsembleManager : public QObject {
    Q_OBJECT
public:
    explicit EnsembleManager(AppConfig* config, QObject* parent = nullptr);

    void runEnsemble(const QString& prompt, 
                    const QStringList& modelIds, 
                    const QString& workDir,
                    const QList<Message>& history,
                    const QString& systemPrompt);

signals:
    void responseReady(const QString& synthesizedResponse);
    void ensembleError(const QString& error);

private:
    AppConfig* m_config;
    QMap<QString, CliRunner*> m_runners;
    QMap<QString, QString> m_responses;
    int m_pendingModels = 0;
    QString m_workDir;
    QList<Message> m_history;
    QString m_originalPrompt;
    QString m_systemPrompt;
    QString m_synthesized;

    void synthesize();
    LlmProvider findProviderById(const QString& id) const;
};

} // namespace CodeHex
