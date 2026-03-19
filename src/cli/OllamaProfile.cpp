#include "OllamaProfile.h"
#include <QRegularExpression>
#include "../data/Message.h"

namespace CodeHex {

static constexpr int kMaxHistoryMessages = 20;

QStringList OllamaProfile::buildArguments(const QString& prompt,
                                          const QString& /*workDir*/) const {
    // ollama run <model> "<prompt>" — non-interactive: prints response and exits.
    return {"run", m_model, prompt};
}

QStringList OllamaProfile::buildArguments(const QString& prompt,
                                          const QString& workDir,
                                          const QList<Message>& history) const {
    // Prepend prior exchanges as a text context block before the current prompt.
    const int histEnd   = history.size() - 1;   // skip last (== current prompt)
    const int histStart = qMax(0, histEnd - kMaxHistoryMessages);

    QString context;
    for (int i = histStart; i < histEnd; ++i) {
        const Message& msg = history.at(i);
        if (msg.role == Message::Role::User)
            context += "User: " + msg.textFromContentBlocks() + "\n";
        else if (msg.role == Message::Role::Assistant)
            context += "Assistant: " + msg.textFromContentBlocks() + "\n";
    }

    const QString fullPrompt = context.isEmpty()
        ? prompt
        : context + "\nUser: " + prompt;

    return buildArguments(fullPrompt, workDir);
}

QString OllamaProfile::parseStreamChunk(const QByteArray& raw) const {
    // `ollama run` streams plain text to stdout (no JSON wrapper).
    // Strip ANSI escape sequences that appear in some terminal environments.
    QString text = QString::fromUtf8(raw);
    static const QRegularExpression ansi(R"(\x1B\[[0-9;]*[A-Za-z])");
    text.remove(ansi);
    return text;
}

}  // namespace CodeHex
