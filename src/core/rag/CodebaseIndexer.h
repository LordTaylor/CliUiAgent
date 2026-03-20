#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <vector>
#include <QMutex>
#include <atomic>

namespace CodeHex {

class EmbeddingManager;

struct IndexChunk {
    QString filePath;
    int startLine;
    int endLine;
    QString content;
    std::vector<float> embedding;
};

class CodebaseIndexer : public QObject {
    Q_OBJECT
public:
    explicit CodebaseIndexer(EmbeddingManager* embeddings, QObject* parent = nullptr);

    /**
     * @brief Walk the directory and build the index.
     * Only re-indexes files if they changed.
     */
    void indexDirectory(const QString& path);

    /**
     * @brief Search for the most relevant chunks.
     */
    QList<IndexChunk> search(const QString& query, int limit = 5);

    bool isIndexing() const { return m_isIndexing; }

signals:
    void indexingStarted();
    void indexingFinished();
    void progressChanged(int current, int total);

private:
    float cosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2);
    QList<QString> findFiles(const QString& root);
    QList<IndexChunk> chunkFile(const QString& path);

    EmbeddingManager* m_embeddings;
    QList<IndexChunk> m_index;
    QMap<QString, QDateTime> m_fileState; // filePath -> lastModified
    std::atomic<bool> m_isIndexing{false};
    mutable QMutex m_mutex;
};

} // namespace CodeHex
