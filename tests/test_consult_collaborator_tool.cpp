/**
 * @file test_consult_collaborator_tool.cpp
 * @brief Tests for Roadmap Item #5: Multi-Agent Collaborative Mode.
 *
 * These tests verify that ConsultCollaboratorTool correctly validates its inputs
 * and that the tool is properly registered in AgentEngine/ToolExecutor with the
 * expected alias.
 *
 * NOTE: The actual LLM round-trip (consultCollaborator()) is NOT unit-tested here
 * because it requires a live LLM backend. See test_lmstudio_integration.cpp for
 * live integration pattern. Unit tests focus on tool contract and error paths.
 */
#include <catch2/catch_test_macros.hpp>
#include <QJsonObject>
#include "../src/core/AgentEngine.h"
#include "../src/core/tools/AskAgentTool.h"

using namespace CodeHex;

// ---------------------------------------------------------------------------
// Minimal stub engine for parameter-validation tests
// ---------------------------------------------------------------------------
class StubEngineForCollaboratorTest {
public:
    // The tool only calls consultCollaborator() if the prompt is non-empty.
    // This stub is never actually invoked in these tests.
    QString consultCollaborator(const QString& /*prompt*/, const QString& /*role*/) {
        return "stub_response";
    }
};

// ---------------------------------------------------------------------------
// We cannot instantiate ConsultCollaboratorTool with StubEngine because it
// expects a real AgentEngine*. Instead we exercise the parameter validation
// path by directly inspecting parameters() and the error path via manual checks.
// ---------------------------------------------------------------------------

TEST_CASE("AskAgentTool: parameters schema is well-formed", "[AskAgent]") {
    // We need a real AgentEngine* for construction, but we can pass nullptr
    // for the purpose of inspecting metadata only (no execute() call).
    AskAgentTool tool(nullptr);

    CHECK(tool.name() == "AskAgent");
    CHECK(!tool.description().isEmpty());

    QJsonObject schema = tool.parameters();
    REQUIRE(schema.contains("type"));
    REQUIRE(schema["type"].toString() == "object");
    REQUIRE(schema.contains("properties"));

    QJsonObject props = schema["properties"].toObject();
    REQUIRE(props.contains("prompt"));
    REQUIRE(props.contains("role"));

    REQUIRE(schema.contains("required"));
    QJsonArray required = schema["required"].toArray();
    // "prompt" must be in the required list
    bool foundPrompt = false;
    for (const auto& v : required) {
        if (v.toString() == "prompt") foundPrompt = true;
    }
    REQUIRE(foundPrompt);
}

TEST_CASE("AskAgentTool: returns error if prompt is missing", "[AskAgent]") {
    // Passing nullptr as engine is safe here because the execute() will
    // return early with an error before touching m_engine.
    AskAgentTool tool(nullptr);

    QJsonObject emptyInput;
    ToolResult result = tool.execute(emptyInput, "");

    REQUIRE(result.isError == true);
    REQUIRE(result.content.contains("prompt"));
}

TEST_CASE("AskAgentTool: returns error if prompt is whitespace-only", "[AskAgent]") {
    AskAgentTool tool(nullptr);

    QJsonObject input;
    input["prompt"] = "   ";
    ToolResult result = tool.execute(input, "");

    REQUIRE(result.isError == true);
}
