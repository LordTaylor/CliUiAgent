#pragma once
#include <optional>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include "../data/Message.h"
#include "../data/ToolCall.h"

namespace CodeHex {

// ── StreamResult ─────────────────────────────────────────────────────────────
// Returned by CliProfile::parseLine() for each complete stdout line.
// At most one of textToken / toolCall is populated per call.
struct StreamResult {
    QString                 textToken;  // non-empty → streaming text token
    std::optional<ToolCall> toolCall;   // set → a tool call was fully parsed
};

// ── CliProfile ────────────────────────────────────────────────────────────────
// Abstract interface for a CLI-backed model profile.
// Concrete implementations: ClaudeProfile, OllamaProfile, ConfigurableProfile.
class CliProfile {
public:
    virtual ~CliProfile() = default;

    virtual QString name()         const = 0;
    virtual QString displayName()  const = 0;
    virtual QString executable()   const = 0;
    virtual QString defaultModel() const = 0;

    // ── Argument builders ─────────────────────────────────────────────────────
    virtual QStringList buildArguments(const QString& prompt,
                                       const QString& workDir) const {
        return buildArguments(prompt, workDir, {}, {});
    }

    // Multi-turn overload — default delegates to 2-arg (ignores history).
    virtual QStringList buildArguments(const QString& prompt,
                                       const QString& workDir,
                                       const QList<Message>& /*history*/,
                                       const QString& /*systemPrompt*/ = {}) const {
        return buildArguments(prompt, workDir);
    }

    // ── Stream parsing ────────────────────────────────────────────────────────
    // Legacy single-value parser — returns a text token or empty string.
    // Kept for back-compat; new code should override parseLine() instead.
    virtual QString parseStreamChunk(const QByteArray& raw) const = 0;

    // Stateful line parser called by CliRunner for every complete stdout line.
    // Profiles that support tool-call streaming (ClaudeProfile) override this to
    // track multi-line tool_use blocks and emit a ToolCall when complete.
    // Default: wraps parseStreamChunk() — no tool-call support.
    virtual StreamResult parseLine(const QByteArray& line) const {
        return StreamResult{ parseStreamChunk(line), std::nullopt };
    }

    // Called by CliRunner::send() to reset per-request parser state
    // (tool-block accumulators, etc.) before starting a new request.
    virtual void reset() {}

    // ── Misc ─────────────────────────────────────────────────────────────────
    virtual QMap<QString, QString> extraEnvironment() const { return {}; }

    // Extra CLI arguments for image attachments (e.g. --image path).
    // Inserted before the -p prompt flag so ordering is correct.
    virtual QStringList imageArguments(const QStringList& /*imagePaths*/) const { return {}; }
};

}  // namespace CodeHex
