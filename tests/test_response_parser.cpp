#include <catch2/catch_test_macros.hpp>
#include "../src/core/ResponseParser.h"
#include <QString>
#include <QJsonObject>

using namespace CodeHex;

TEST_CASE("ResponseParser: Basic XML Tool Call", "[ResponseParser]") {
    QString response = R"(
<thought>I need to write a file.</thought>
<name>WriteFile</name>
<input>
{
    "path": "test.txt",
    "content": "Hello World"
}
</input>
)";

    ResponseParser::ParseResult result = ResponseParser::parse(response);
    
    REQUIRE(result.thoughts.size() == 1);
    REQUIRE(result.thoughts.first().content == "I need to write a file.");
    
    REQUIRE(result.toolCalls.size() == 1);
    REQUIRE(result.toolCalls.first().name == "WriteFile");
    REQUIRE(result.toolCalls.first().input["path"].toString() == "test.txt");
    
    REQUIRE(result.cleanText.isEmpty());
}

TEST_CASE("ResponseParser: Multiple Thoughts and Text", "[ResponseParser]") {
    QString response = R"(
<thought>First thought.</thought>
Some intermediate text.
<thought>Second thought.</thought>
Final instructions.
)";

    ResponseParser::ParseResult result = ResponseParser::parse(response);
    
    REQUIRE(result.thoughts.size() == 2);
    REQUIRE(result.thoughts[0].content == "First thought.");
    REQUIRE(result.thoughts[1].content == "Second thought.");
    
    REQUIRE(result.cleanText == "Some intermediate text.\n\nFinal instructions.");
}

TEST_CASE("ResponseParser: Bash Fallback", "[ResponseParser]") {
    QString response = R"(
I will run this command:
```bash
ls -la
```
)";

    ResponseParser::ParseResult result = ResponseParser::parse(response);
    
    REQUIRE(result.toolCalls.size() == 1);
    REQUIRE(result.toolCalls.first().name == "Bash");
    REQUIRE(result.toolCalls.first().input["command"].toString() == "ls -la");
    
    REQUIRE(result.cleanText == "I will run this command:");
}

TEST_CASE("ResponseParser: Malformed JSON in Tool Call", "[ResponseParser]") {
    QString response = R"(
<name>WriteFile</name>
<input>
{
    "path": "test.txt",
    invalid json
}
</input>
)";

    ResponseParser::ParseResult result = ResponseParser::parse(response);
    
    REQUIRE(result.toolCalls.isEmpty()); // Should fail gracefully
}

TEST_CASE("ResponseParser: Cleaning Output", "[ResponseParser]") {
    QString response = R"(
<thought>Internal thought</thought>
Hello User!
<name>Tool</name><input>{}</input>
**Task Completed:** Success!
)";

    ResponseParser::ParseResult result = ResponseParser::parse(response);
    
    REQUIRE(result.cleanText == "Hello User!");
}
