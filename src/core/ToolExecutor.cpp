#include "ToolExecutor.h"
#include "tools/ReadFileTool.h"
#include "tools/WriteFileTool.h"
#include "tools/ListDirectoryTool.h"
#include "tools/BashTool.h"
#include "tools/SearchFilesTool.h"
#include "tools/GrepTool.h"
#include "tools/ReplaceTool.h"
#include "tools/GitTool.h"
#include <QtConcurrent>
#include <QMetaType>

namespace CodeHex {

ToolExecutor::ToolExecutor(QObject* parent) : QObject(parent) {
    qRegisterMetaType<CodeHex::ToolResult>();
    
    // Register Core Tools
    registerTool(std::make_shared<ReadFileTool>());
    registerTool(std::make_shared<WriteFileTool>());
    registerTool(std::make_shared<ListDirectoryTool>());
    registerTool(std::make_shared<BashTool>());
    registerTool(std::make_shared<SearchFilesTool>());
    registerTool(std::make_shared<GrepTool>());
    registerTool(std::make_shared<ReplaceTool>());

    // Register Git Tools
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Status));
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Diff));
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Log));

    // Register Aliases
    registerAlias("Read",  "ReadFile");
    registerAlias("Write", "WriteFile");
    registerAlias("LS",    "ListDirectory");
    registerAlias("Bash",  "Bash");
    registerAlias("RunCommand", "Bash"); // Back-compat
    registerAlias("Glob",  "SearchFiles");
    registerAlias("Grep",  "Search");
    registerAlias("Sed",   "Replace");
}

void ToolExecutor::registerTool(std::shared_ptr<Tool> tool) {
    if (tool) {
        m_tools[tool->name()] = tool;
    }
}

void ToolExecutor::registerAlias(const QString& alias, const QString& originalName) {
    m_aliases[alias] = originalName;
}

void ToolExecutor::execute(const ToolCall& call, const QString& workDir) {
    emit toolStarted(call.name, call.input);

    // Capture everything needed by value to ensure thread safety
    QString toolName = call.name;
    QJsonObject input = call.input;
    QString toolUseId = call.id;

    QtConcurrent::run([this, toolName, input, workDir, toolUseId]() {
        ToolResult result = executeSync(ToolCall{toolUseId, toolName, input}, workDir);
        // executeSync already emitted toolFinished? No, we should emit it here for async
        // Wait, executeSync should NOT emit toolFinished if we want to control it from here.
        // Actually, let's keep executeSync pure and handle signals in public methods.
    });
}

ToolResult ToolExecutor::executeSync(const ToolCall& call, const QString& workDir) {
    // Note: toolStarted is emitted ONLY in the async version to avoid double emission
    // if called from execute(). But for tests, maybe we want it?
    // Let's stick to the rule: execute() handles signals for the UI loop.

    ToolResult result;
    QString toolName = call.name;
    QString resolvedName = toolName;

    if (m_aliases.contains(resolvedName)) {
        resolvedName = m_aliases[resolvedName];
    }

    if (m_tools.contains(resolvedName)) {
        result = m_tools[resolvedName]->execute(call.input, workDir);
    } else {
        result = ToolResult{ {}, QString("Unknown tool: '%1'").arg(toolName), true };
    }

    result.toolUseId = call.id;
    
    // If called from a background thread (via execute()), we emit toolFinished here.
    // If called from a test (main thread), it still works.
    emit toolFinished(toolName, result);
    
    return result;
}

}  // namespace CodeHex