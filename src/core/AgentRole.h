#pragma once

namespace CodeHex {

/**
 * @brief Categorizes the current state or specialized personality of the agent.
 */
enum class AgentRole {
    Base,      // General-purpose
    Explorer,  // Research, search, information gathering
    Executor,  // Code implementation, tool usage
    Reviewer   // Verification, auditing, logic checking
};

} // namespace CodeHex
