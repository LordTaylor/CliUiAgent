#pragma once
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QMap>
#include <memory>
#include <QtConcurrent>
#include "../data/ToolCall.h"
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

signals:
    void toolStarted(const QString& toolName, const QJsonObject& input);
    void toolFinished(const QString& toolName, const CodeHex::ToolResult& result);

private:
    void registerTool(std::shared_ptr<Tool> tool);
    void registerAlias(const QString& alias, const QString& originalName);

    QMap<QString, std::shared_ptr<Tool>> m_tools;
    QMap<QString, QString> m_aliases;
};

}  // namespace CodeHex
