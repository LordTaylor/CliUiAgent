#include <catch2/catch_test_macros.hpp>
#include "ResponseParser.h"
#include <QJsonObject>
#include <QUuid>

using namespace CodeHex;

TEST_CASE("Parser handles responses with trailing garbage or multiple completion markers", "[Parser][Advanced]") {
    // Example based on sessions where the model repeats "Task Completed" inside the tool call turn
    QString response = R"(
I will now run the script.
<name>Bash</name>
<input>
{
    "command": "python3 hello.py"
}
</input>
**Task Completed:** The script was executed.
**Task Finalized:** Everything is done.
)";

    auto result = ResponseParser::parse(response);
    auto calls = result.toolCalls;
    REQUIRE(calls.size() == 1);
    REQUIRE(calls.first().name == "Bash");
    REQUIRE(calls.first().input["command"].toString() == "python3 hello.py");
}

TEST_CASE("Parser handles extremely messy content with nested blocks (real session data)", "[Parser][Advanced]") {
    // Example from 18bf6e89-c5c4-4ed9-bcf2-557a5abe9705.json line 324
    QString response = R"(
The output "Hello, World!" indicates that the `hello_world.py` script has been executed successfully. ...```xml
<name>Bash</name>
<input>
{
    "command": "echo 'nested'"
}
</input>
```**Task Completed:**
- Successfully executed `hello_world.py`
)";

    auto result = ResponseParser::parse(response);
    auto calls = result.toolCalls;
    REQUIRE(calls.size() == 1);
    REQUIRE(calls.first().name == "Bash");
    REQUIRE(calls.first().input["command"].toString() == "echo 'nested'");
}

TEST_CASE("Parser ignores invalid XML tags if they don't match our specific pattern", "[Parser][Advanced]") {
    QString response = R"(
<thought>I should use a tool</thought>
<unknown_tag>Something</unknown_tag>
<name>TestTool</name>
<input>{"val": 1}</input>
)";

    auto result = ResponseParser::parse(response);
    auto calls = result.toolCalls;
    REQUIRE(calls.size() == 1);
    REQUIRE(calls.first().name == "TestTool");
}

TEST_CASE("Parser handles responses with multiple potential tool calls but pick only the first", "[Parser][Advanced]") {
    QString response = R"(
<name>FirstTool</name><input>{"id": 1}</input>
<name>SecondTool</name><input>{"id": 2}</input>
)";

    auto result = ResponseParser::parse(response);
    auto calls = result.toolCalls;
    REQUIRE(calls.size() == 2);
    REQUIRE(calls.first().name == "FirstTool");
}
