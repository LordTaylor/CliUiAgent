#include "ModelRouter.h"
#include "AppConfig.h"
#include <QRegularExpression>

namespace CodeHex {

ModelRouter::ModelRouter(AppConfig* config, QObject* parent)
    : QObject(parent), m_config(config) {
    loadDefaults();
}

QString ModelRouter::getProfileIdForRole(AgentRole role) const {
    return m_roleMap.value(role, m_config->activeProviderId());
}

void ModelRouter::setProfileIdForRole(AgentRole role, const QString& profileId) {
    m_roleMap[role] = profileId;
}

void ModelRouter::loadDefaults() {
    // In a real app, this could be loaded from a config file.
    // For now, we use the active provider as a default for all roles.
    QString active = m_config->activeProviderId();
    m_roleMap[AgentRole::Base]     = active;
    m_roleMap[AgentRole::Explorer] = active;
    m_roleMap[AgentRole::Executor] = active;
    m_roleMap[AgentRole::Reviewer] = active;
}

AgentRole ModelRouter::detectRoleFromPrompt(const QString& userInput) const {
    const QString lower = userInput.toLower();

    // Scoring: each keyword match adds to a role's score
    struct { AgentRole role; QStringList keywords; } rules[] = {
        { AgentRole::Explorer,  {"znajdź", "szukaj", "pokaż", "wylistuj", "search", "find", "list", "show", "where is", "locate"} },
        { AgentRole::Executor,  {"napisz", "dodaj", "utwórz", "zaimplementuj", "write", "add", "create", "implement", "build", "make"} },
        { AgentRole::Reviewer,  {"sprawdź", "zweryfikuj", "review", "check", "verify", "audit", "validate", "test"} },
        { AgentRole::Debugger,  {"napraw", "debug", "fix", "bug", "error", "crash", "stacktrace", "dlaczego nie działa"} },
        { AgentRole::Refactor,  {"refaktor", "uprość", "refactor", "simplify", "clean up", "optimize", "DRY"} },
        { AgentRole::Architect, {"zaprojektuj", "architektura", "design", "architect", "plan", "structure", "diagram"} },
        { AgentRole::SecurityAuditor, {"bezpieczeństwo", "security", "vulnerability", "CVE", "injection", "XSS"} },
        { AgentRole::RAG,       {"kontekst", "codebase", "repozytorium", "context", "knowledge"} },
    };

    AgentRole bestRole = AgentRole::Base;
    int bestScore = 0;

    for (const auto& rule : rules) {
        int score = 0;
        for (const auto& kw : rule.keywords) {
            if (lower.contains(kw)) ++score;
        }
        if (score > bestScore) {
            bestScore = score;
            bestRole = rule.role;
        }
    }

    return bestRole;
}

QMap<AgentRole, int> ModelRouter::scoreRolesFromPrompt(const QString& userInput) const {
    const QString lower = userInput.toLower();

    struct { AgentRole role; QStringList keywords; } rules[] = {
        { AgentRole::Explorer,  {"znajdź", "szukaj", "pokaż", "wylistuj", "search", "find", "list", "show", "where is", "locate"} },
        { AgentRole::Executor,  {"napisz", "dodaj", "utwórz", "zaimplementuj", "write", "add", "create", "implement", "build", "make"} },
        { AgentRole::Reviewer,  {"sprawdź", "zweryfikuj", "review", "check", "verify", "audit", "validate", "test"} },
        { AgentRole::Debugger,  {"napraw", "debug", "fix", "bug", "error", "crash", "stacktrace", "dlaczego nie działa"} },
        { AgentRole::Refactor,  {"refaktor", "uprość", "refactor", "simplify", "clean up", "optimize", "DRY"} },
        { AgentRole::Architect, {"zaprojektuj", "architektura", "design", "architect", "plan", "structure", "diagram"} },
        { AgentRole::SecurityAuditor, {"bezpieczeństwo", "security", "vulnerability", "CVE", "injection", "XSS"} },
        { AgentRole::RAG,       {"kontekst", "codebase", "repozytorium", "context", "knowledge"} },
    };

    QMap<AgentRole, int> scores;
    for (const auto& rule : rules) {
        int score = 0;
        for (const auto& kw : rule.keywords)
            if (lower.contains(kw)) ++score;
        scores[rule.role] = score;
    }
    return scores;
}

QStringList ModelRouter::allowedToolsForRole(AgentRole role) const {
    // Empty = all tools allowed. Non-empty = whitelist.
    switch (role) {
    case AgentRole::Explorer:
        return {"SearchFiles", "Search", "ListDirectory", "ReadFile",
                "GitStatus", "GitLog", "GitDiff"};
    case AgentRole::Reviewer:
        return {"ReadFile", "Search", "SearchFiles", "ListDirectory",
                "GitStatus", "GitLog", "GitDiff"};
    case AgentRole::Architect:
        return {"ReadFile", "Search", "SearchFiles", "ListDirectory",
                "VisualizeCodebase", "ExportKnowledgeGraph"};
    case AgentRole::SecurityAuditor:
        return {"ReadFile", "Search", "SearchFiles", "ListDirectory",
                "Bash", "GitLog"};
    case AgentRole::Debugger:
        return {"ReadFile", "Search", "SearchFiles", "Bash", "Replace",
                "WriteFile", "ReadStacktrace", "AnalyzePerformance", "TakeScreenshot"};
    case AgentRole::Refactor:
        return {"ReadFile", "WriteFile", "Replace", "Search", "SearchFiles", "ListDirectory"};
    case AgentRole::RAG:
        return {"ReadFile", "Search", "SearchFiles", "ListDirectory"};
    case AgentRole::Executor:
    case AgentRole::Base:
    default:
        return {}; // All tools
    }
}

QStringList ModelRouter::defaultTechniquesForRole(AgentRole role) const {
    switch (role) {
    case AgentRole::Executor:   return {"clean_code"};
    case AgentRole::Reviewer:   return {"tdd"};
    case AgentRole::Debugger:   return {"clean_code"};
    case AgentRole::Refactor:   return {"clean_code", "performance"};
    case AgentRole::SecurityAuditor: return {};
    default:                    return {};
    }
}

} // namespace CodeHex
