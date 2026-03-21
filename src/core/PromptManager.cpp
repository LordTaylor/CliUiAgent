#include "PromptManager.h"
#include "AppConfig.h"
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
        qWarning() << "[PromptManager] No rules.md found — agent running without explicit rules.";
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
    // (In a real app, this would come from a model registry/config)
    int modelLimit = 120000; 
    QString model = m_config->activeProvider().selectedModel.toLower();
    if (model.contains("sonnet") || model.contains("opus")) {
        modelLimit = 120000;
    } else if (model.contains("haiku") || model.contains("gpt-4o-mini")) {
        modelLimit = 128000;
    }
    
    // We reserve some room for system prompt and tools (roughly 20% or 10k)
    pruneOptions.maxTokens = modelLimit - 10000; 
    
    QList<Message> prunedHistory = ContextManager::prune(fullHistory, pruneOptions, statsOut);

    QJsonArray messages;
    for (const auto& msg : prunedHistory) {
        QJsonObject msgObj;
        msgObj["role"] = (msg.role == Message::Role::Assistant) ? "assistant" : "user";
        
        QJsonArray content;
        QJsonObject textBlock;
        textBlock["type"] = "text";
        
        QString textContent = msg.rawContent;
        if (textContent.isEmpty()) {
            textContent = msg.textFromContentBlocks();
        }
        
        // Anthropic refuses completely empty text blocks. 
        // If an assistant message contains absolutely no text/meaningful raw content, specify a fallback or skip.
        // But for CodeHex, tool results are in User role (msg.textFromContentBlocks) and tool calls are Assistant role (msg.rawContent).
        textBlock["text"] = textContent.isEmpty() ? "..." : textContent;
        
        content.append(textBlock);
        
        msgObj["content"] = content;
        messages.append(msgObj);
    }
    request["messages"] = messages;

    // 4. Thinking Configuration
    if (thinkingBudget > 0) {
        QJsonObject thinking;
        thinking["type"] = "enabled";
        thinking["budget_tokens"] = thinkingBudget;
        request["thinking"] = thinking;
    }

    // 5. Output Configuration
    QJsonObject outputConfig;
    outputConfig["effort"] = "medium";
    request["output_config"] = outputConfig;

    return request;
}

} // namespace CodeHex
