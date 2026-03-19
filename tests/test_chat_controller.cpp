#include <catch2/catch_test_macros.hpp>
#include <QSignalSpy>
#include <QJsonObject>
#include "ChatController.h"
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
    void send(const QString& prompt, const QString& workDir, const QStringList& imagePaths, const QList<Message>& history) override {
        lastPrompt = prompt;
        sendCount++;
    }
    QString lastPrompt;
    int sendCount = 0;
};

TEST_CASE("ChatController Tool Loop", "[ChatController]") {
    AppConfig config;
    SessionManager sessions(&config);
    MockCliRunner runner; // CliRunner takes QObject* parent
    ScriptManager scripts(".", ".");
    
    ChatController controller(&config, &sessions, &runner, &scripts);
    sessions.createSession("test_profile", "Test Session");

    SECTION("ToolCall recording") {
        ToolCall call{"123", "ReadFile", {{"path", "test.txt"}}};
        
        // This should trigger recording the call and executing it
        controller.onToolCallReady(call);
        
        Session* session = sessions.currentSession();
        REQUIRE(session != nullptr);
        REQUIRE(!session->messages.isEmpty());
        
        const Message& lastMsg = session->messages.last();
        REQUIRE(lastMsg.role == Message::Role::Assistant);
        REQUIRE(!lastMsg.contentBlocks.isEmpty());
        REQUIRE(lastMsg.contentBlocks.first().type == BlockType::ToolCall);
        REQUIRE(lastMsg.contentBlocks.first().content.contains("ReadFile"));
    }

    SECTION("ToolResult handling and re-prompt") {
        ToolResult result{"123", "File content here", false};
        
        int initialSendCount = runner.sendCount;
        
        // This should trigger recording the result and re-prompting
        controller.onToolResultReceived("ReadFile", result);
        
        Session* session = sessions.currentSession();
        REQUIRE(!session->messages.isEmpty());
        
        const Message& lastMsg = session->messages.last();
        REQUIRE(lastMsg.role == Message::Role::Assistant);
        REQUIRE(lastMsg.contentBlocks.first().type == BlockType::Output);
        REQUIRE(lastMsg.contentBlocks.first().content == "File content here");
        
        // Check if re-prompt was triggered
        REQUIRE(runner.sendCount == initialSendCount + 1);
        REQUIRE(runner.lastPrompt.isEmpty()); // Re-prompt uses history, prompt is empty string usually
    }
}
