#include "CodebaseIndexer.h"
#include "EmbeddingManager.h"
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <cmath>
#include <algorithm>

namespace CodeHex {

CodebaseIndexer::CodebaseIndexer(EmbeddingManager* embeddings, QObject* parent)
    : QObject(parent), m_embeddings(embeddings) {}

void CodebaseIndexer::indexDirectory(const QString& root) {
    if (m_isIndexing.exchange(true)) return;
    emit indexingStarted();

    const QStringList files = findFiles(root);
    int total = files.size();
    int current = 0;

    for (const QString& path : files) {
        QFileInfo info(path);
        QDateTime lastMod = info.lastModified();

        if (!m_fileState.contains(path) || m_fileState[path] < lastMod) {
            // Remove old chunks for this file
            {
                QMutexLocker locker(&m_mutex);
                m_index.erase(std::remove_if(m_index.begin(), m_index.end(),
                              [&](const IndexChunk& c) { return c.filePath == path; }),
                              m_index.end());
            }

            // Add new chunks
            QList<IndexChunk> chunks = chunkFile(path);
            
            // Generate embeddings in batch for this file
            QStringList texts;
            for (const auto& c : chunks) texts << c.content;
            
            auto embeds = m_embeddings->getEmbeddings(texts);
            for (int i = 0; i < chunks.size() && i < embeds.size(); ++i) {
                chunks[i].embedding = embeds[i];
                QMutexLocker locker(&m_mutex);
                m_index.append(chunks[i]);
            }

            QMutexLocker locker(&m_mutex);
            m_fileState[path] = lastMod;
        }

        current++;
        emit progressChanged(current, total);
    }

    m_isIndexing = false;
    emit indexingFinished();
}

QList<IndexChunk> CodebaseIndexer::search(const QString& query, int limit) {
    if (m_index.isEmpty()) return {};

    std::vector<float> queryEmbed = m_embeddings->getEmbedding(query);
    if (queryEmbed.empty()) return {};

    struct ScoredChunk {
        IndexChunk chunk;
        float score;
    };

    QList<ScoredChunk> scored;
    {
        QMutexLocker locker(&m_mutex);
        for (const auto& chunk : m_index) {
            float s = cosineSimilarity(queryEmbed, chunk.embedding);
            scored.append({chunk, s});
        }
    }

    // Sort by score descending
    std::sort(scored.begin(), scored.end(), [](const ScoredChunk& a, const ScoredChunk& b) {
        return a.score > b.score;
    });

    QList<IndexChunk> results;
    for (int i = 0; i < std::min(limit, (int)scored.size()); ++i) {
        results.append(scored[i].chunk);
    }
    return results;
}

float CodebaseIndexer::cosineSimilarity(const std::vector<float>& v1, const std::vector<float>& v2) {
    if (v1.size() != v2.size() || v1.empty()) return 0.0f;
    double dot = 0.0, n1 = 0.0, n2 = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        dot += v1[i] * v2[i];
        n1 += v1[i] * v1[i];
        n2 += v2[i] * v2[i];
    }
    if (n1 <= 0.0 || n2 <= 0.0) return 0.0f;
    return static_cast<float>(dot / (std::sqrt(n1) * std::sqrt(n2)));
}

QList<QString> CodebaseIndexer::findFiles(const QString& root) {
    QList<QString> result;
    QDirIterator it(root, { "*.cpp", "*.h", "*.txt", "*.md", "*.py", "*.lua", "*.qss" },
                    QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        // Ignore build and git folders
        if (!path.contains("/build/") && !path.contains("/.git/")) {
            result << path;
        }
    }
    return result;
}

QList<IndexChunk> CodebaseIndexer::chunkFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QList<IndexChunk> chunks;
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // Simple line-based chunking for MVP
    QStringList lines = content.split('\n');
    const int chunkSize = 50; // lines
    const int overlap = 10;

    for (int i = 0; i < lines.size(); i += (chunkSize - overlap)) {
        int end = std::min(i + chunkSize, (int)lines.size());
        QStringList chunkLines = lines.mid(i, end - i);
        
        IndexChunk c;
        c.filePath = path;
        c.startLine = i + 1;
        c.endLine = end;
        c.content = chunkLines.join('\n');
        chunks.append(c);
        
        if (end == lines.size()) break;
    }

    return chunks;
}

} // namespace CodeHex
