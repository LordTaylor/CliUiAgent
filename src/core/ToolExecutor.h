#pragma once
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QMap>
#include <memory>
#include <QtConcurrent>
#include <atomic>
#include "Tool.h"
#include "ToolCall.h"

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

    /**
     * @brief Aborts the currently running tool.
     */
    void stop();

signals:
    void toolStarted(const QString& toolName, const QJsonObject& input);
    void toolFinished(const QString& toolName, const CodeHex::ToolResult& result);

private:
    QMap<QString, std::shared_ptr<Tool>> m_tools;
    QMap<QString, QString> m_aliases;
    std::atomic<Tool*> m_activeTool{nullptr};
};

}  // namespace CodeHex