#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QObject>

namespace CodeHex {

/**
 * @brief Handles LLM output filtering, tag suppression, and thinking states.
 */
class ResponseFilter : public QObject {
    Q_OBJECT
public:
    explicit ResponseFilter(QObject* parent = nullptr);

    void reset();
    
    /**
     * @brief Processes a new chunk of output.
     * @return The filtered text to be displayed to the user.
     */
    QString processChunk(const QString& chunk);

    /**
     * @brief Cleans all tool tags from a finalized response.
     */
    QString cleanToolTags(const QString& text) const;

    QString currentResponse() const { return m_currentResponse; }
    QString thoughtBuffer() const { return m_thoughtBuffer; }
    bool isThinking() const { return m_isThinkingStream; }

signals:
    void thinkingStarted();
    void thinkingFinished();

private:
    QString m_currentResponse;
    QString m_thoughtBuffer;
    QString m_tailBuffer;
    bool m_isThinkingStream = false;
    bool m_isSuppressed = false;
    QString m_suppressClosingTag;
};

} // namespace CodeHex
