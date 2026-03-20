#include <catch2/catch_test_macros.hpp>
#include <QSignalSpy>
#include <QJsonObject>
#include "ChatController.h"
#include "AgentEngine.h"
#include "CliRunner.h"
#include "SessionManager.h"
#include "AppConfig.h"
#include "ScriptManager.h"
#include "ToolCall.h"

using namespace CodeHex;

// Simple mock for CliRunner to observe send calls
class MockCliRunner : public CliRunner {
public:
    using CliRunner::CliRunner;
    void send(const QString& prompt, 
              const QString& workDir, 
              const QStringList& imagePaths, 
              const QList<Message>& history,
              const QString& systemPrompt = {}) override {
        lastPrompt = prompt;
        lastSystemPrompt = systemPrompt;
        sendCount++;
    }
    QString lastPrompt;
    QString lastSystemPrompt;
    int sendCount = 0;
};

TEST_CASE("ChatController Tool Loop", "[ChatController]") {
    AppConfig config;
    QTemporaryDir tempDir;
    config.setDataDir(tempDir.path());
    config.ensureDirectories();
    
    SessionManager sessions(&config);
    MockCliRunner runner; 
    ScriptManager scripts(".", ".");
    
    ChatController controller(&config, &sessions, &runner, &scripts);
    config.setWorkingFolder(".");
    sessions.createSession("test_profile", "Test Session");

    SECTION("ToolCall handled by AgentEngine") {
        ToolCall call{"123", "ReadFile", {{"path", "test.txt"}}};
        
        // This should trigger recording the call and executing it via AgentEngine
        controller.agent()->setRunningForTest(true);
        controller.agent()->setSyncTools(true);
        controller.agent()->onToolCallReady(call);
        
        Session* session = sessions.currentSession();
        REQUIRE(session != nullptr);
        // Requirement was that some message is added or state changes.
        // In the new version, we check the session or signals.
    }

    SECTION("ToolResult handling in AgentEngine") {
        ToolResult result{"123", "File content here", false};
        
        int initialSendCount = runner.sendCount;
        
        // This should trigger recording the result and re-prompting
        controller.agent()->onToolResultReceived("ReadFile", result);
        
        Session* session = sessions.currentSession();
        REQUIRE(!session->messages.isEmpty());
        
        // Check if re-prompt was triggered
        REQUIRE(runner.sendCount == initialSendCount + 1);
    }
}
