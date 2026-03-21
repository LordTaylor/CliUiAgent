#include <QObject>
#include <QString>
#include <QMap>
#include <QHash>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QThreadPool>
#include <QMutex>
#include <memory>
#include <atomic>

#include "ToolCall.h"
#include "Tool.h"

namespace CodeHex {

class ToolExecutor : public QObject {
    Q_OBJECT
public:
    explicit ToolExecutor(QObject* parent = nullptr);

    /**
     * @brief Executes the tool call asynchronously in a background thread.
     * Emits toolStarted() immediately and toolFinished() when done.
     */
    void execute(const ToolCall& call, const QString& workDir);

    /**
     * @brief Executes the tool call synchronously in the current thread.
     * Use primarily for testing or when already in a worker thread.
     */
    ToolResult executeSync(const ToolCall& call, const QString& workDir);
    void registerTool(std::shared_ptr<Tool> tool);
    void registerAlias(const QString& alias, const QString& originalName);
    
    /**
     * @brief Returns a formatted string describing all registered tools.
     */
    QString getToolDefinitions() const;
    QJsonArray getToolDefinitionsJson() const;

    /**
     * @brief Aborts the currently running tool.
     */
    void stop();

    /** @brief Removes cached content for the given file path. */
    void clearCacheFor(const QString& path);

signals:
    void toolStarted(const QString& toolName, const QJsonObject& input);
    void toolFinished(const QString& toolName, const CodeHex::ToolResult& result);

    /** @brief Invalidates read cache for the given path (call after writes). */
    void invalidateCache(const QString& path);

private:
    QMap<QString, std::shared_ptr<Tool>> m_tools;
    QMap<QString, QString> m_aliases;
    std::atomic<Tool*> m_activeTool{nullptr};

    // --- Dedicated Tool ThreadPool (P-5) ---
    QThreadPool m_toolPool;

    // --- In-Session Read Cache (P-2) ---
    struct CacheEntry {
        QDateTime fileModified;
        QString   content;
    };
    QHash<QString, CacheEntry> m_readCache;
    QMutex m_cacheMutex;
    static constexpr int MAX_CACHE_ENTRIES = 50;
};

}  // namespace CodeHex