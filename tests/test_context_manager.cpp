#include <catch2/catch_test_macros.hpp>
#include "ContextManager.h"
#include "TokenCounter.h"
#include <QList>

using namespace CodeHex;

TEST_CASE("ContextManager prunes history correctly", "[ContextManager]") {
    auto createMsg = [](Message::Role role, const QString& text) {
        Message msg;
        msg.role = role;
        msg.addText(text);
        return msg;
    };

    QList<Message> history;
    history.append(createMsg(Message::Role::System, "System prompt context")); // ~4 tokens
    history.append(createMsg(Message::Role::User, "Old message 1")); // ~4 tokens
    history.append(createMsg(Message::Role::Assistant, "Old response 1")); // ~4 tokens
    history.append(createMsg(Message::Role::User, "Old message 2")); // ~4 tokens
    history.append(createMsg(Message::Role::Assistant, "Old response 2")); // ~4 tokens
    history.append(createMsg(Message::Role::User, "Newest message")); // ~4 tokens
    
    // Total approx: 6 messages * 4 tokens + 6*4 overhead = ~48 tokens
    
    ContextManager::PruningOptions options;
    options.maxTokens = 30; // Force pruning
    options.keepRatio = 0.5f; // Keep approx 15 tokens of new stuff + system
    
    SECTION("Basic pruning keeps system prompt and newest messages") {
        QList<Message> pruned = ContextManager::prune(history, options);
        
        REQUIRE(!pruned.isEmpty());
        REQUIRE(pruned.first().role == Message::Role::System);
        REQUIRE(pruned.last().textFromContentBlocks() == "Newest message");
        
        // Should have removed oldest user/assistant pairs that didn't fit
        REQUIRE(pruned.size() < history.size());
        
        int totalTokens = TokenCounter::countMessages(pruned);
        REQUIRE(totalTokens <= options.maxTokens);
    }
    
    SECTION("Does not prune if under limit") {
        options.maxTokens = 1000;
        QList<Message> pruned = ContextManager::prune(history, options);
        REQUIRE(pruned.size() == history.size());
    }
}

TEST_CASE("TokenCounter precision test", "[TokenCounter]") {
    // English
    REQUIRE(TokenCounter::count("Hello world") == 2);
    // Contractions
    REQUIRE(TokenCounter::count("don't stop 're") >= 3);
    // Code-like
    REQUIRE(TokenCounter::count("int main() { return 0; }") > 5);
}
