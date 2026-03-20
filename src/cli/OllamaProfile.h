#pragma once
#include "CliProfile.h"

namespace CodeHex {

// Wraps `ollama run <model>` CLI.
class OllamaProfile : public CliProfile {
public:
    explicit OllamaProfile(const QString& model = "llama3.2") : m_model(model) {}

    QString name() const override { return "ollama"; }
    QString displayName() const override { return "Ollama"; }
    QString executable() const override { return "ollama"; }
    QString defaultModel() const override { return m_model; }
    
    void setModel(const QString& model) override { m_model = model; }
    QString model() const override { return m_model; }

    QStringList buildArguments(const QString& prompt,
                               const QString& workDir) const override;
    QStringList buildArguments(const QString& prompt,
                               const QString& workDir,
                               const QList<Message>& history,
                               const QString& systemPrompt = {}) const override;
    QString parseStreamChunk(const QByteArray& raw) const override;

private:
    QString m_model;
};

}  // namespace CodeHex
