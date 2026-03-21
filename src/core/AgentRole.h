#pragma once

namespace CodeHex {

/**
 * @brief Categorizes the current state or specialized personality of the agent.
 */
enum class AgentRole {
    Base,      // General-purpose
    Explorer,  // Research, search, information gathering
    Executor,  // Code implementation, tool usage
    Reviewer,  // Verification, auditing, logic checking
    RAG,       // Knowledge retrieval from codebase
    REFACTOR,   // Code simplification & optimization
    Architect, // High-level design and planning
    Debugger,  // Root cause analysis and bug fixing
    SecurityAuditor // Security auditing and risk assessment
};

} // namespace CodeHex
