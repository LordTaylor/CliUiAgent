#include "PromptManager.h"
#include "AppConfig.h"
#include "ModelProfileManager.h"
#include <QSysInfo>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QtGlobal>
#include <QCoreApplication>

namespace CodeHex {

PromptManager::PromptManager(AppConfig* config, QObject* parent) 
    : QObject(parent), m_config(config) {}

void PromptManager::ensureEnvCache() const {
    if (!m_envVersionCache.isEmpty()) return;

    QString info = "### INSTALLED TOOL VERSIONS:\n";
    
    auto getVersion = [](const QString& cmd, const QStringList& args) {
        QProcess proc;
        proc.start(cmd, args);
        if (proc.waitForFinished(1000)) {
            QString out = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
            if (out.isEmpty()) out = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();
            return out.isEmpty() ? "Detected" : out;
        }
        return QString("Not Found");
    };

    info += "- **Python**: " + getVersion("python3", {"--version"}) + "\n";
    info += "- **Node.js**: " + getVersion("node", {"--version"}) + "\n";
    info += "- **Git**: " + getVersion("git", {"--version"}) + "\n";
    info += "- **Qt**: " + QString(QT_VERSION_STR) + "\n";

    m_envVersionCache = info;
}

QString PromptManager::buildSystemPrompt(AgentRole role, 
                                         const QString& autoContext,
                                         const QStringList& activeTechniques,
                                         const QMap<QString, QString>& blackboard) const {
    ensureEnvCache();

    QString base = "### CURRENT HOST CONTEXT:\n"
                   "- **Operating System**: " + QSysInfo::prettyProductName() + "\n"
                   "- **CPU Architecture**: " + QSysInfo::currentCpuArchitecture() + "\n"
                   "- **Working Directory**: " + QDir(m_config->workingFolder()).absolutePath() + "\n"
                   "- **Current Role**: " + (role == AgentRole::Executor ? "Executor" : 
                                            (role == AgentRole::Reviewer ? "Reviewer" : 
                                            (role == AgentRole::Explorer ? "Explorer" : 
                                            (role == AgentRole::RAG ? "RAG" : 
                                            (role == AgentRole::Architect ? "Architect" :
                                            (role == AgentRole::Debugger ? "Debugger" :
                                            (role == AgentRole::SecurityAuditor ? "SecurityAuditor" : "Base"))))))) + "\n\n";

    base += m_envVersionCache + "\n\n";
    base += roleStrategy(role) + "\n\n";
    base += loadRolePrompt(role);
    
    // 🎭 ACTIVE TECHNIQUES (Behavior Injection)
    if (!activeTechniques.isEmpty()) {
        base += "\n\n### ACTIVE TECHNIQUES (MANDATORY BEHAVIORS):\n";
        for (const QString& tech : activeTechniques) {
            QString content = loadTechniquePrompt(tech);
            if (!content.isEmpty()) {
                base += content + "\n";
            }
        }
    }

    // 📋 SESSION BLACKBOARD (Short-Term Memory)
    if (!blackboard.isEmpty()) {
        base += "\n\n### SESSION BLACKBOARD (Short-Term Progress Notes):\n";
        for (auto it = blackboard.begin(); it != blackboard.end(); ++it) {
            base += QString("- **%1**: %2\n").arg(it.key(), it.value());
        }
    }

    // 🧠 AGENT BRAIN: Load persistent memory
    QString memoryPath = m_config->workingFolder() + "/.agent/memory.md";
    QFile memFile(memoryPath);
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        base += "\n\n### PROJECT MEMORY (Read to avoid repeating errors):\n" + 
                QString::fromUtf8(memFile.readAll());
    }

    // 📜 PROJECT RULES: Load mandatory constraints
    // 1. Global rules: walk up from executable until .agent/rules.md is found
    auto loadRulesFile = [](const QString& path) -> QString {
        QFile f(path);
        return (f.open(QIODevice::ReadOnly | QIODevice::Text))
            ? QString::fromUtf8(f.readAll()) : QString();
    };

    QString globalRules;
    {
        QDir dir(QCoreApplication::applicationDirPath());
        for (int i = 0; i < 8 && !dir.isRoot(); ++i) {
            QString candidate = dir.filePath(".agent/rules.md");
            globalRules = loadRulesFile(candidate);
            if (!globalRules.isEmpty()) {
                qDebug() << "[PromptManager] Global rules loaded from:" << candidate;
                break;
            }
            dir.cdUp();
        }
    }

    // 2. Project-specific rules (workingFolder — may override or extend global)
    QString projectRules;
    if (!m_config->workingFolder().isEmpty()) {
        projectRules = loadRulesFile(m_config->workingFolder() + "/.agent/rules.md");
    }

    QString allRules = globalRules;
    if (!projectRules.isEmpty() && projectRules != globalRules)
        allRules += (allRules.isEmpty() ? "" : "\n\n---\n\n") + projectRules;

    if (!allRules.isEmpty()) {
        base += "\n\n### MANDATORY PROJECT RULES & GUIDELINES (STRICT ADHERENCE REQUIRED):\n"
                + allRules;
    } else {
        static bool s_rulesWarned = false;
        if (!s_rulesWarned) {
            qWarning() << "[PromptManager] No rules.md found — agent running without explicit rules. (Reported once)";
            s_rulesWarned = true;
        }
    }

    base += "\n\n### INTERNAL TOOLS & SCRATCHPAD:\n"
            "- You have a dedicated directory `.agent/scratchpad/` for internal scripts.\n"
            "- If you encounter a complex task that standard tools can't solve easily, "
            "WRITE a custom script (Python/Bash) into that folder and EXECUTE it.\n"
            "- 🍏 **MacOS:** Use `osascript -e 'tell app \"Terminal\" to do script \"python3 %1\"'` to open a real window.\n"
            "- 🪟 **Windows:** Use `cmd /c start powershell -Command \"python %1; Read-Host 'Press Enter to exit'\"`.\n"
            "- 🐧 **Linux:** Use `x-terminal-emulator -e \"bash -c 'python3 %1; read -p \\\"Press Enter to exit\\\"'\"`.\n"
            "You CAN open windows on the user's computer.\n\n"
            "### HOST ENVIRONMENT:\n"
            "The user is currently running on: **" + QSysInfo::prettyProductName() + "**.\n"
            "ALWAYS use commands compatible with this operating system.\n\n"
            "### 🍎 MACOS TIPS & CAPABILITIES:\n"
            "- **File Operations:** Use `open <file>` to open documents or `open -a \"Application\"` for specific apps.\n"
            "- **Clipboard:** Use `pbcopy` and `pbpaste` to interact with the system clipboard.\n"
            "- **Search:** Use `mdfind` for fast Spotlight-based file searches.\n"
            "- **Packages:** `brew` is the preferred package manager for installing CLI tools.\n"
            "- **Shell:** Standard shell is **zsh**. Be mindful of quoting and escaping.\n\n"
            "### 🛠️ COMMAND CONSTRUCTION GUIDELINES:\n"
            "1. **STRICT XML FORMAT:** Your tool calls MUST use the `<tool_call><name>...</name><input>...</input></tool_call>` structure.\n"
            "2. **RAW JSON INPUT:** The `<input>` block must contain ONLY a single, valid JSON object.\n"
            "3. **NO MARKDOWN IN INPUT:** Never wrap the JSON inside `<input>` with ```json or other markdown tags. The parser expects raw JSON.\n"
            "4. **THINK BEFORE ACTING:** Always use a `<thought>` block to analyze the task and plan your commands before sending them.";

    if (!autoContext.isEmpty()) {
        base += "\n\n### DYNAMIC CONTEXT:\n" + autoContext;
    }

    return base;
}

QString PromptManager::roleStrategy(AgentRole role) const {
    switch (role) {
        case AgentRole::Explorer:
            return "### ROLE STRATEGY: EXPLORER (Researcher)\n"
                   "You are in EXPLORATION mode. Your goal is to map the codebase and understand requirements.\n"
                   "- **Preferred Tools**: `SearchRepo`, `ListDir`, `ViewFile`, `GrepSearch`.\n"
                   "- **Guidelines**: Focus on understanding logic and architecture. Do NOT modify files or start implementation yet. "
                   "Summarize findings clearly for the next stage.";
        case AgentRole::Executor:
            return "### ROLE STRATEGY: EXECUTOR (Implementer)\n"
                   "You are in EXECUTION mode. Your goal is to implement the approved plan.\n"
                    "- **Preferred Tools**: `WriteFile`, `ReplaceFileContent`, `Bash` (for building/running).\n"
                    "- **Guidelines**: Focus on safety and code quality. Do NOT break existing features.";
        case AgentRole::Reviewer:
            return "### ROLE STRATEGY: REVIEWER (Quality Assurance)\n"
                   "You are in REVIEW mode. Your goal is to validate the work done.\n"
                   "- **Preferred Tools**: `ViewFile`, `GrepSearch`, `Bash` (for running tests).\n"
                   "- **Guidelines**: Look for edge cases, performance issues, and security flaws. Audit the diffs carefully. "
                   "Suggest improvements but do NOT modify the codebase yourself unless explicitly permitted.";
        case AgentRole::Architect:
            return "### ROLE STRATEGY: ARCHITECT (Designer)\n"
                   "You are in DESIGN mode. Your goal is high-level planning and modularity.\n"
                   "- **Preferred Tools**: `ReadFile`, `ListDirectory`, `GrepSearch`.\n"
                   "- **Guidelines**: Focus on design patterns, scalability, and loose coupling.";
        case AgentRole::Debugger:
            return "### ROLE STRATEGY: DEBUGGER (Troubleshooter)\n"
                   "You are in TROUBLESHOOTING mode. Identify root causes and fix bugs precisely.\n"
                   "- **Preferred Tools**: `Bash` (logging), `ReadFile`, `ReplaceFileContent`.\n"
                   "- **Guidelines**: Fix the cause, not just the symptom. Minimal regressions.";
        case AgentRole::SecurityAuditor:
            return "### ROLE STRATEGY: SECURITY AUDITOR (Guardian)\n"
                   "You are in AUDIT mode. Detect vulnerabilities and assess risks.\n"
                   "- **Preferred Tools**: `GrepSearch`, `ViewFile`, `Bash` (static analysis tools).\n"
                   "- **Guidelines**: Use OWASP standards. Rate risks as Low/Medium/High/Critical.";
        case AgentRole::RAG:
            return "### ROLE STRATEGY: RAG (Knowledge Retrieval)\n"
                   "You are in KNOWLEDGE RETRIEVAL mode. Use the provided context to answer questions about the entire project.\n"
                   "- **Preferred Tools**: None required (Context is automatically provided), but use `ViewFile` for extra detail if needed.\n"
                   "- **Guidelines**: Be precise. Cite specific files and line numbers from the provided context.";
        default:
            return "### ROLE STRATEGY: ASSISTANT\n"
                   "Provide general assistance, answer questions, and help with small tasks.";
    }
}

QString PromptManager::detectImplicitGoals(const QString& query) const {
    QString goals = "\n\n### IMPLICIT GOAL ANALYSIS:\n"
                    "Based on your query, the following underlying objectives have been identified:\n";
    
    bool detected = false;
    if (query.contains("fix", Qt::CaseInsensitive) || query.contains("bug", Qt::CaseInsensitive)) {
        goals += "- **Stability:** Ensure no regressions are introduced while fixing the issue.\n";
        goals += "- **Verification:** Provide a way to test the fix.\n";
        detected = true;
    }
    if (query.contains("add", Qt::CaseInsensitive) || query.contains("create", Qt::CaseInsensitive)) {
        goals += "- **Architecture Compatibility:** Ensure the new feature follows existing patterns.\n";
        goals += "- **Documentation:** Consider if help files need updating.\n";
        detected = true;
    }
    if (query.contains("optim", Qt::CaseInsensitive) || query.contains("fast", Qt::CaseInsensitive)) {
        goals += "- **Efficiency:** Prioritize token economy and resource-efficient algorithms.\n";
        goals += "- **Speed:** Focus on low-latency solutions and fast execution paths.\n";
        detected = true;
    }
    if (query.contains("review", Qt::CaseInsensitive) || query.contains("audit", Qt::CaseInsensitive)) {
        goals += "- **Security:** Identify potential vulnerabilities and security risks.\n";
        goals += "- **Quality:** Ensure compliance with best practices and project standards.\n";
        detected = true;
    }

    if (!detected) {
        goals += "- **Holistic Solution:** Address the immediate request while considering long-term maintainability.\n";
    }

    return goals;
}

QString PromptManager::loadRolePrompt(AgentRole role) const {
    QString fileName;
    switch (role) {
        case AgentRole::Base:     fileName = "base.txt"; break;
        case AgentRole::Explorer: fileName = "explorer.txt"; break;
        case AgentRole::Executor: fileName = "executor.txt"; break;
        case AgentRole::Reviewer: fileName = "reviewer.txt"; break;
        case AgentRole::RAG:      fileName = "rag.txt"; break;
        case AgentRole::Refactor: fileName = "refactor.txt"; break;
        case AgentRole::Architect: fileName = "architect.txt"; break;
        case AgentRole::Debugger:  fileName = "debugger.txt"; break;
        case AgentRole::SecurityAuditor: fileName = "security_auditor.txt"; break;
    }
    
    QFile file(":/resources/prompts/" + fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    return QString();
}

QString PromptManager::loadTechniquePrompt(const QString& name) const {
    // 1. Try embedded resources first
    QFile resFile(":/resources/prompts/techniques/" + name + ".txt");
    if (resFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(resFile.readAll());
    }
    
    // 2. Try file system if not in resources
    QFile fsFile(m_config->workingFolder() + "/.agent/techniques/" + name + ".txt");
    if (fsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(fsFile.readAll());
    }
    
    return QString();
}

QJsonObject PromptManager::buildRequestJson(AgentRole role, 
                                          const QString& userInput, 
                                          const QList<Message>& history, 
                                          const QJsonArray& tools,
                                          const QStringList& activeTechniques,
                                          const QMap<QString, QString>& blackboard,
                                          int thinkingBudget,
                                          bool useCache,
                                          const QString& ragContext,
                                          ContextManager::ContextStats* statsOut) const {
    QJsonObject request;

    // 1. System Prompt (Array of blocks)
    QJsonArray system;
    QJsonObject systemBlock;
    systemBlock["type"] = "text";
    
    // Note: We need to pass techniques and blackboard from AgentEngine/Session
    systemBlock["text"] = buildSystemPrompt(role, ragContext, activeTechniques, blackboard);
    
    if (useCache) {
        QJsonObject cacheControl;
        cacheControl["type"] = "ephemeral";
        cacheControl["ttl"] = "1h";
        systemBlock["cache_control"] = cacheControl;
    }
    system.append(systemBlock);
    request["system"] = system;

    // 2. Tools
    request["tools"] = tools;

    // 3. Messages (with Pruning)
    QList<Message> fullHistory = history;
    if (!userInput.isEmpty()) {
        Message userMsg;
        userMsg.role = Message::Role::User;
        userMsg.addText(userInput);
        fullHistory.append(userMsg);
    }

    ContextManager::PruningOptions pruneOptions;
    // Dynamic token threshold based on model capability
    int modelLimit = m_config->activeProvider().contextWindow;
    if (modelLimit <= 0) modelLimit = 32768; // Fallback
    
    // We reserve some room for system prompt and tools (roughly 10%)
    pruneOptions.maxTokens = modelLimit - (modelLimit / 10); 
    
    QList<Message> prunedHistory = ContextManager::prune(fullHistory, pruneOptions, statsOut);

    QJsonArray messages;
    auto appendOrMerge = [&](const QString& role, const QString& content) {
        if (content.isEmpty()) return;
        if (!messages.isEmpty() && messages.last().toObject()["role"].toString() == role) {
            QJsonObject last = messages.last().toObject();
            QJsonArray msgContent = last["content"].toArray();
            if (!msgContent.isEmpty()) {
                QJsonObject textBlock = msgContent.last().toObject();
                QString existing = textBlock["text"].toString();
                textBlock["text"] = existing + "\n\n" + content;
                msgContent[msgContent.size() - 1] = textBlock;
                last["content"] = msgContent;
                messages[messages.size() - 1] = last;
            }
        } else {
            QJsonObject msgObj;
            msgObj["role"] = role;
            QJsonArray msgContent;
            QJsonObject textBlock;
            textBlock["type"] = "text";
            textBlock["text"] = content;
            msgContent.append(textBlock);
            msgObj["content"] = msgContent;
            messages.append(msgObj);
        }
    };

    // Resolve model profile (hot-reloadable, no recompilation needed)
    const bool isAnthropic = useCache;
    const QString modelName = m_config->activeProvider().selectedModel;
    const ModelProfile profile = isAnthropic
        ? ModelProfile::defaultProfile()
        : m_config->profileManager()->findProfile(modelName);
    const QString toolRespFmt = profile.toolResponseFormat;

    for (const auto& msg : prunedHistory) {
        QString role = (msg.role == Message::Role::Assistant) ? "assistant" : "user";
        QString textContent = msg.rawContent;
        if (textContent.isEmpty()) {
            textContent = msg.textFromContentBlocks();
        }

        // Non-Anthropic models: format tool results according to profile
        if (!isAnthropic && !msg.toolResults.isEmpty()) {
            for (const auto& tr : msg.toolResults) {
                QJsonObject toolMsg;
                QString wrappedContent;

                if (toolRespFmt == "deepseek-output") {
                    wrappedContent = QString("<\uFF5Ctool\u2581outputs\u2581begin\uFF5C>"
                                            "<\uFF5Ctool\u2581output\u2581begin\uFF5C>"
                                            "%1"
                                            "<\uFF5Ctool\u2581output\u2581end\uFF5C>"
                                            "<\uFF5Ctool\u2581outputs\u2581end\uFF5C>").arg(tr.content);
                    toolMsg["role"] = "user";
                } else if (toolRespFmt == "mistral-results") {
                    QJsonObject resultObj;
                    resultObj["call_id"] = tr.toolUseId;
                    resultObj["content"] = tr.content;
                    QJsonArray resultArr;
                    resultArr.append(resultObj);
                    wrappedContent = QString("[TOOL_RESULTS]%1[/TOOL_RESULTS]")
                        .arg(QString::fromUtf8(QJsonDocument(resultArr).toJson(QJsonDocument::Compact)));
                    toolMsg["role"] = "user";
                } else {
                    // "qwen-tool-role" and any future formats default to this
                    wrappedContent = QString("<tool_response>\n%1\n</tool_response>").arg(tr.content);
                    toolMsg["role"] = "tool";
                }

                QJsonArray toolContent;
                QJsonObject block;
                block["type"] = "text";
                block["text"] = wrappedContent;
                toolContent.append(block);
                toolMsg["content"] = toolContent;
                messages.append(toolMsg);
            }
            continue;
        }

        // Anthropic refuses completely empty text blocks.
        if (textContent.isEmpty()) textContent = "...";

        appendOrMerge(role, textContent);
    }
    request["messages"] = messages;

    // 4. Provider-specific generation parameters (profile-driven)
    if (isAnthropic) {
        if (thinkingBudget > 0) {
            QJsonObject thinking;
            thinking["type"] = "enabled";
            thinking["budget_tokens"] = thinkingBudget;
            request["thinking"] = thinking;
        }
        QJsonObject outputConfig;
        outputConfig["effort"] = "medium";
        request["output_config"] = outputConfig;
    } else if (profile.hasGenerationParams) {
        request["temperature"] = profile.temperature;
        request["top_p"]       = profile.topP;
        if (profile.topK >= 0)
            request["top_k"] = profile.topK;
        if (!profile.stopSequences.isEmpty()) {
            QJsonArray stops;
            for (const auto& s : profile.stopSequences) stops.append(s);
            request["stop"] = stops;
        }
    }

    return request;
}

} // namespace CodeHex
