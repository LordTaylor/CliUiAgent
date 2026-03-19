#pragma once
#include "CliProfile.h"

namespace CodeHex {

// Wraps the official `claude` CLI (Anthropic).
// Invocation: claude --print --output-format stream-json -p "<prompt>"
class ClaudeProfile : public CliProfile {
public:
    QString name() const override { return "claude"; }
    QString displayName() const override { return "Claude CLI"; }
    QString executable() const override { return "claude"; }
    QString defaultModel() const override { return "claude-sonnet-4-6"; }

    QStringList buildArguments(const QString& prompt, const QString& workDir) const override;
    QString parseStreamChunk(const QByteArray& raw) const override;
};

}  // namespace CodeHex
