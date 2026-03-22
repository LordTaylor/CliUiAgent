#include "ToolExecutor.h"
#include "ToolCall.h"
#include "tools/ReadFileTool.h"
#include "tools/WriteFileTool.h"
#include "tools/ListDirectoryTool.h"
#include "tools/BashTool.h"
#include "tools/SearchFilesTool.h"
#include "tools/GrepTool.h"
#include "tools/ReplaceTool.h"
#include "tools/GitTool.h"
#include "tools/MathLogicTool.h"
#include "tools/TakeScreenshotTool.h"
#include "tools/ImportModelProfileTool.h"
#include "tools/DownloadModelProfileTool.h"
#include <QtConcurrent>
#include <QMetaType>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QMutexLocker>
#include <QThread>

namespace CodeHex {

ToolExecutor::ToolExecutor(QObject* parent) : QObject(parent) {
    qRegisterMetaType<CodeHex::ToolResult>();

    // Dedicated Tool ThreadPool (P-5)
    m_toolPool.setMaxThreadCount(qMin(4, QThread::idealThreadCount()));

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
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Add));
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Commit));
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Checkout));
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Branches));
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Push));
    registerTool(std::make_shared<GitTool>(GitTool::Mode::Stash));
    registerTool(std::make_shared<MathLogicTool>());
    registerTool(std::make_shared<TakeScreenshotTool>());
    registerTool(std::make_shared<ImportModelProfileTool>());
    registerTool(std::make_shared<DownloadModelProfileTool>());

    // Register Aliases
    registerAlias("Read",  "ReadFile");
    registerAlias("Write", "WriteFile");
    registerAlias("LS",    "ListDirectory");
    registerAlias("Bash",  "Bash");
    registerAlias("RunCommand", "Bash"); // Back-compat
    registerAlias("Glob",  "SearchFiles");
    registerAlias("Grep",  "Search");
    registerAlias("Sed",   "Replace");
    registerAlias("Screenshot", "TakeScreenshot");
    registerAlias("ImportProfile",   "ImportModelProfile");
    registerAlias("DownloadProfile", "DownloadModelProfile");
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

    // Run on dedicated tool thread pool (P-5)
    QtConcurrent::run(&m_toolPool, [this, call, workDir]() {
        executeSync(call, workDir);
    });
}

ToolResult ToolExecutor::executeSync(const ToolCall& call, const QString& workDir) {
    ToolResult result;
    QString toolName = call.name;
    QString resolvedName = toolName;

    if (m_aliases.contains(resolvedName)) {
        resolvedName = m_aliases[resolvedName];
    }

    // --- In-Session Read Cache (P-2) ---
    bool isReadFile = (resolvedName == "ReadFile");
    bool isWriteTool = (resolvedName == "WriteFile" || resolvedName == "Replace"
                        || resolvedName == "SearchReplace");

    if (isReadFile && call.input.contains("path")) {
        QString filePath = call.input.value("path").toString();
        QFileInfo fi(QDir(workDir), filePath);
        QString absPath = fi.absoluteFilePath();
        QDateTime currentMod = QFileInfo(absPath).lastModified();

        QMutexLocker lock(&m_cacheMutex);
        if (m_readCache.contains(absPath) && m_readCache[absPath].fileModified == currentMod) {
            result.content = m_readCache[absPath].content;
            result.isError = false;
            result.toolUseId = call.id;
            lock.unlock();
            emit toolFinished(toolName, result);
            return result;
        }
    }

    // PreToolCall hook — scripts can inspect/modify input
    QJsonObject effectiveInput = call.input;
    if (m_hooks) {
        QVariantMap hookArgs;
        hookArgs["toolName"] = resolvedName;
        hookArgs["input"]    = QString(QJsonDocument(call.input).toJson());
        m_hooks->runHooks(HookRegistry::HookPoint::PreToolCall, hookArgs);
    }

    if (m_tools.contains(resolvedName)) {
        Tool* tool = m_tools[resolvedName].get();
        m_activeTool = tool;
        result = tool->execute(effectiveInput, workDir);
        m_activeTool = nullptr;
    } else {
        result = ToolResult{ {}, QString("Unknown tool: '%1'").arg(toolName), true };
    }

    // PostToolCall hook
    if (m_hooks) {
        QVariantMap hookArgs;
        hookArgs["toolName"] = resolvedName;
        hookArgs["result"]   = result.content;
        hookArgs["isError"]  = result.isError;
        m_hooks->runHooks(HookRegistry::HookPoint::PostToolCall, hookArgs);
    }

    // OnFileWrite hook — fire when WriteFile succeeds
    if (!result.isError && resolvedName == "WriteFile") {
        const QString writtenPath = call.input.contains("TargetFile")
            ? call.input["TargetFile"].toString()
            : call.input["path"].toString();
        if (m_hooks && !writtenPath.isEmpty()) {
            QVariantMap hookArgs;
            hookArgs["path"]    = writtenPath;
            hookArgs["content"] = call.input.contains("CodeContent")
                ? call.input["CodeContent"].toString()
                : call.input["content"].toString();
            m_hooks->runHooks(HookRegistry::HookPoint::OnFileWrite, hookArgs);
        }
    }

    // Cache successful ReadFile results
    if (isReadFile && !result.isError && call.input.contains("path")) {
        QString filePath = call.input.value("path").toString();
        QFileInfo fi(QDir(workDir), filePath);
        QString absPath = fi.absoluteFilePath();
        QMutexLocker lock(&m_cacheMutex);
        if (m_readCache.size() >= MAX_CACHE_ENTRIES) {
            m_readCache.erase(m_readCache.begin()); // evict oldest
        }
        m_readCache[absPath] = { QFileInfo(absPath).lastModified(), result.content };
    }

    // Invalidate cache on write operations
    if (isWriteTool && call.input.contains("path")) {
        QString filePath = call.input.value("path").toString();
        QFileInfo fi(QDir(workDir), filePath);
        clearCacheFor(fi.absoluteFilePath());
    }

    result.toolUseId = call.id;
    emit toolFinished(toolName, result);
    return result;
}

QString ToolExecutor::getToolDefinitions() const {
    QString defs = "## System Tools (Tool Use)\n\n";
    defs += "To perform an action, you MUST use the following XML-JSON format in your response:\n\n";
    defs += "```xml\n";
    defs += "<tool_call>\n";
    defs += "<name>ToolName</name>\n";
    defs += "<input>\n";
    defs += "{\"parameter\": \"value\"}\n";
    defs += "</input>\n";
    defs += "</tool_call>\n";
    defs += "```\n\n";
    
    defs += "### RULES:\n";
    defs += "1. **Multi-tool Parallelism.** You can send multiple `<tool_call>` tags in a single response — "
            "they will be executed PARALLELIZED. Use this for independent operations (e.g., reading multiple files).\n";
    defs += "2. **Valid JSON.** The content of `<input>` MUST be a valid JSON object.\n";
    defs += "3. **No Markdown Inside.** Do NOT use ` ```json ` or ` ``` ` tags inside the `<input>` block. This is a critical error to avoid.\n";
    defs += "   *Negative Example (WRONG):* `<input> ```json {\"path\": \"file.txt\"} ``` </input>`\n";
    defs += "   *Positive Example (CORRECT):* `<input>{\"path\": \"file.txt\"}</input>`\n";
    defs += "4. **Chain-of-Thought.** Always use `<thought>` tags to plan your multi-step process before sending the XML.\n\n";
    
    defs += "### AVAILABLE TOOLS:\n\n";
    for (auto it = m_tools.begin(); it != m_tools.end(); ++it) {
        auto tool = it.value();
        defs += QString("#### %1\n").arg(tool->name());
        defs += tool->description() + "\n";
        defs += "Parameters (JSON schema):\n";
        QJsonDocument doc(tool->parameters());
        defs += "```json\n" + doc.toJson(QJsonDocument::Indented) + "```\n\n";
    }
    return defs;
}

QJsonArray ToolExecutor::getToolDefinitionsJson() const {
    QJsonArray tools;
    for (auto it = m_tools.begin(); it != m_tools.end(); ++it) {
        auto tool = it.value();
        QJsonObject toolObj;
        toolObj["name"] = tool->name();
        toolObj["description"] = tool->description();
        toolObj["input_schema"] = tool->parameters(); // parameters() returns the JSON schema object
        tools.append(toolObj);
    }
    return tools;
}

void ToolExecutor::stop() {
    Tool* active = m_activeTool.load();
    if (active) {
        active->abort();
    }
}

QJsonArray ToolExecutor::getToolDefinitionsForRole(AgentRole role,
                                                    const QStringList& allowedTools) const {
    Q_UNUSED(role)
    if (allowedTools.isEmpty())
        return getToolDefinitionsJson(); // All tools

    QJsonArray tools;
    for (auto it = m_tools.begin(); it != m_tools.end(); ++it) {
        if (allowedTools.contains(it.key())) {
            auto tool = it.value();
            QJsonObject obj;
            obj["name"]         = tool->name();
            obj["description"]  = tool->description();
            obj["input_schema"] = tool->parameters();
            tools.append(obj);
        }
    }
    return tools;
}

void ToolExecutor::clearCacheFor(const QString& path) {
    QMutexLocker lock(&m_cacheMutex);
    m_readCache.remove(path);
}

}  // namespace CodeHex