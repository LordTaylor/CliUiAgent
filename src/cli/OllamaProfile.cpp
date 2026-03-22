#include "OllamaProfile.h"
#include <QRegularExpression>
#include "../data/Message.h"

namespace CodeHex {

static constexpr int kMaxHistoryMessages = 20;

QStringList OllamaProfile::buildArguments(const QString& prompt,
                                          const QString& /*workDir*/) const {
    // ollama run <model> "<prompt>" — non-interactive: prints response and exits.
    return {"run", model(), prompt};
}

QStringList OllamaProfile::buildArguments(const QString& prompt,
                                          const QString& workDir,
                                          const QList<Message>& history,
                                          const QString& systemPrompt) const {
    // Prepend prior exchanges as a text context block before the current prompt.
    const int histEnd   = history.size() - 1;   // skip last (== current prompt)
    const int histStart = qMax(0, histEnd - kMaxHistoryMessages);

    QString context;
    if (!systemPrompt.isEmpty()) {
        context += "System: " + systemPrompt + "\n\n";
    }

    QString lastAssContent;
    int repeatCount = 0;
    QString lastRole = (!systemPrompt.isEmpty() ? "System" : "");

    auto appendOrMerge = [&](const QString& role, const QString& content) {
        if (content.isEmpty()) return;
        if (role == lastRole) {
            context += "\n\n" + content;
        } else {
            context += role + ": " + content + "\n";
            lastRole = role;
        }
    };

    for (int i = histStart; i < histEnd; ++i) {
        const Message& msg = history.at(i);
        QString currentText = msg.textFromContentBlocks();

        if (msg.role == Message::Role::Assistant) {
            if (!lastAssContent.isEmpty() && currentText == lastAssContent) {
                repeatCount++;
                continue;
            }
            lastAssContent = currentText;
            appendOrMerge("Assistant", currentText);
        } else if (msg.role == Message::Role::User) {
            appendOrMerge("User", currentText);
        }
    }

    if (repeatCount >= 2) {
        context += "\nSystem: WARNING: You are appearing to repeat previous actions. Please finish the task or change your technical approach to avoid looping.\n";
        lastRole = "System";
    }

    const QString fullPrompt = (lastRole == "User")
        ? context + "\n\n" + prompt
        : (context.isEmpty() ? prompt : context + "\nUser: " + prompt);

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
