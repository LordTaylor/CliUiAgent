#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>

#ifdef slots
#undef slots
#endif
#include <pybind11/embed.h>
#define slots Q_SLOTS

namespace CodeHex {

class EmbeddingManager : public QObject {
    Q_OBJECT
public:
    explicit EmbeddingManager(QObject* parent = nullptr);
    ~EmbeddingManager();

    /**
     * @brief Generate embedding for a single text chunk.
     */
    std::vector<float> getEmbedding(const QString& text);

    /**
     * @brief Generate embeddings for multiple text chunks in batch.
     */
    QList<std::vector<float>> getEmbeddings(const QStringList& texts);

private:
    void initializePython();

    bool m_initialized = false;
    pybind11::object m_embeddingFunc;
};

} // namespace CodeHex
