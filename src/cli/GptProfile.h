#pragma once
#include "CliProfile.h"

namespace CodeHex {

// Wraps `sgpt` (shell-gpt) or `openai` CLI.
// Install: pip install shell-gpt
class GptProfile : public CliProfile {
public:
    QString name() const override { return "gpt"; }
    QString displayName() const override { return "OpenAI (sgpt)"; }
    QString executable() const override { return "sgpt"; }
    QString defaultModel() const override { return "gpt-4o"; }

    QStringList buildArguments(const QString& prompt, const QString& workDir) const override;
    QString parseStreamChunk(const QByteArray& raw) const override;
};

}  // namespace CodeHex
