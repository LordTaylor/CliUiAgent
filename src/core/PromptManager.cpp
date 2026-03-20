#include "PromptManager.h"
#include "AppConfig.h"
#include <QSysInfo>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QOperatingSystemVersion>

namespace CodeHex {

PromptManager::PromptManager(AppConfig* config, QObject* parent) 
    : QObject(parent), m_config(config) {}

QString PromptManager::buildSystemPrompt(AgentEngine::Role role, const QString& autoContext) const {
    QString base = "### CURRENT HOST CONTEXT:\n"
                   "- **Operating System**: " + QSysInfo::prettyProductName() + "\n"
                   "- **CPU Architecture**: " + QSysInfo::currentCpuArchitecture() + "\n"
                   "- **Working Directory**: " + QDir(m_config->workingFolder()).absolutePath() + "\n"
                   "- **Current Role**: " + (role == AgentEngine::Role::Executor ? "Executor" : 
                                            (role == AgentEngine::Role::Reviewer ? "Reviewer" : 
                                            (role == AgentEngine::Role::Explorer ? "Explorer" : "Base"))) + "\n\n";

    base += roleStrategy(role) + "\n\n";
    base += loadRolePrompt(role);
    
    // 🧠 AGENT BRAIN: Load persistent memory
    QString memoryPath = m_config->workingFolder() + "/.agent/memory.md";
    QFile memFile(memoryPath);
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        base += "\n\n### PROJECT MEMORY (Read to avoid repeating errors):\n" + 
                QString::fromUtf8(memFile.readAll());
    }

    // 📜 PROJECT RULES: Load mandatory constraints
    QString rulesPath = m_config->workingFolder() + "/.agent/rules.md";
    QFile rulesFile(rulesPath);
    if (rulesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        base += "\n\n### MANDATORY PROJECT RULES & GUIDELINES (STRICT ADHERENCE REQUIRED):\n" + 
                QString::fromUtf8(rulesFile.readAll());
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
            "ALWAYS use commands compatible with this operating system.";

    if (!autoContext.isEmpty()) {
        base += "\n\n### DYNAMIC CONTEXT:\n" + autoContext;
    }

    return base;
}

QString PromptManager::roleStrategy(AgentEngine::Role role) const {
    switch (role) {
        case AgentEngine::Role::Explorer:
            return "### ROLE STRATEGY: EXPLORER (Researcher)\n"
                   "You are in EXPLORATION mode. Your goal is to map the codebase and understand requirements.\n"
                   "- **Preferred Tools**: `SearchRepo`, `ListDir`, `ViewFile`, `GrepSearch`.\n"
                   "- **Guidelines**: Focus on understanding logic and architecture. Do NOT modify files or start implementation yet. "
                   "Summarize findings clearly for the next stage.";
        case AgentEngine::Role::Executor:
            return "### ROLE STRATEGY: EXECUTOR (Implementer)\n"
                   "You are in EXECUTION mode. Your goal is to implement the approved plan.\n"
                   "- **Preferred Tools**: `WriteFile`, `ReplaceFileContent`, `Bash` (for building/running).\n"
                   "- **Guidelines**: Follow the user's implementation plan strictly. Verify every change with a build or test. "
                   "If you hit unexpected complexity, stop and ask for a plan update.";
        case AgentEngine::Role::Reviewer:
            return "### ROLE STRATEGY: REVIEWER (Quality Assurance)\n"
                   "You are in REVIEW mode. Your goal is to validate the work done.\n"
                   "- **Preferred Tools**: `ViewFile`, `GrepSearch`, `Bash` (for running tests).\n"
                   "- **Guidelines**: Look for edge cases, performance issues, and security flaws. Audit the diffs carefully. "
                   "Suggest improvements but do NOT modify the codebase yourself unless explicitly permitted.";
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
    if (query.contains("optim", Qt::CaseInsensitive)) {
        goals += "- **Readability vs Performance:** Maintain code clarity while improving speed/memory.\n";
        detected = true;
    }

    if (!detected) {
        goals += "- **Holistic Solution:** Address the immediate request while considering long-term maintainability.\n";
    }

    return goals;
}

QString PromptManager::loadRolePrompt(AgentEngine::Role role) const {
    QString fileName;
    switch (role) {
        case AgentEngine::Role::Base:     fileName = "base.txt"; break;
        case AgentEngine::Role::Explorer: fileName = "explorer.txt"; break;
        case AgentEngine::Role::Executor: fileName = "executor.txt"; break;
        case AgentEngine::Role::Reviewer: fileName = "reviewer.txt"; break;
    }
    
    QFile file(":/resources/prompts/" + fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    return QString();
}

QJsonObject PromptManager::buildRequestJson(AgentEngine::Role role, 
                                          const QString& userInput, 
                                          const QList<Message>& history, 
                                          const QJsonArray& tools,
                                          int thinkingBudget,
                                          bool useCache) const {
    QJsonObject request;

    // 1. System Prompt (Array of blocks)
    QJsonArray system;
    QJsonObject systemBlock;
    systemBlock["type"] = "text";
    systemBlock["text"] = buildSystemPrompt(role, QString());
    
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
    pruneOptions.maxTokens = 32000; // Safe default for most modern models
    QList<Message> prunedHistory = ContextManager::prune(fullHistory, pruneOptions);

    QJsonArray messages;
    for (const auto& msg : prunedHistory) {
        QJsonObject msgObj;
        msgObj["role"] = (msg.role == Message::Role::Assistant) ? "assistant" : "user";
        
        QJsonArray content;
        QJsonObject textBlock;
        textBlock["type"] = "text";
        textBlock["text"] = msg.textFromContentBlocks();
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
