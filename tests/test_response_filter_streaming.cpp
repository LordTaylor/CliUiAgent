#include <catch2/catch_test_macros.hpp>
#include "ResponseFilter.h"
#include <QString>

using namespace CodeHex;

TEST_CASE("ResponseFilter buffers partial tags correctly", "[ResponseFilter][Streaming]") {
    ResponseFilter filter;
    
    SECTION("Tag split across two chunks") {
        // Chunk 1: "Hello <n"
        QString out1 = filter.processChunk("Hello <n");
        REQUIRE(out1 == "Hello ");
        
        // Chunk 2: "ame>cat</name>World"
        QString out2 = filter.processChunk("ame>cat</name>World");
        REQUIRE(out2 == "World");
    }
    
    SECTION("Tag split across three chunks") {
        // Chunk 1: "Value: <in"
        QString out1 = filter.processChunk("Value: <in");
        REQUIRE(out1 == "Value: ");
        
        // Chunk 2: "p"
        QString out2 = filter.processChunk("p");
        REQUIRE(out2 == "");
        
        // Chunk 3: "ut>42</input>Done"
        QString out3 = filter.processChunk("ut>42</input>Done");
        // Note: Currently my implementation suppresses the tag and what's inside if it recurses.
        // Let's see how it behaves with "ut>42</input>Done"
        // it finds "</input>" in "ut>42</input>Done". suppresses prefix "ut>42". return processChunk("Done").
        REQUIRE(out3 == "Done");
    }

    SECTION("Thinking tags (both versions)") {
        filter.reset();
        
        // Chunk 1: "I am <thou"
        REQUIRE(filter.processChunk("I am <thou") == "I am ");
        
        // Chunk 2: "ght>thinking...</thought>Result"
        QString out2 = filter.processChunk("ght>thinking...</thought>Result");
        REQUIRE(out2 == "Result");
        REQUIRE(filter.thoughtBuffer().contains("thinking..."));
    }

    SECTION("Stray backticks and markdown blocks") {
        filter.reset();
        
        // Chunk 1: "Here is code: ``"
        REQUIRE(filter.processChunk("Here is code: ``") == "Here is code: ");
        
        // Chunk 2: "`bash\necho 1\n```Next"
        REQUIRE(filter.processChunk("`bash\necho 1\n```Next") == "Next");
    }
}
