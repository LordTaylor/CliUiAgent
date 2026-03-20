#include <catch2/catch_test_macros.hpp>
#include <QString>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

// Use the same logic as AgentEngine to verify parsing of complex/malformed responses
namespace {
struct MockToolCall {
    QString id;
    QString name;
    QJsonObject input;
};

QList<MockToolCall> parseToolCalls(const QString& response) {
    QList<MockToolCall> parsedCalls;

    // XML parsing
    QRegularExpression re("<name>\\s*([^<\\s]+)\\s*</name>\\s*<input>\\s*(.*?)\\s*</input>", 
                          QRegularExpression::DotMatchesEverythingOption);
    
    QRegularExpressionMatchIterator i = re.globalMatch(response);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        MockToolCall call;
        call.id = QUuid::createUuid().toString();
        call.name = match.captured(1).trimmed();
        
        QString jsonStr = match.captured(2).trimmed();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (!doc.isNull() && doc.isObject()) {
            call.input = doc.object();
            parsedCalls.append(call);
            break; // Parse only the first valid one, as in AgentEngine
        }
    }

    // Fallback: Bash
    if (parsedCalls.isEmpty()) {
        QRegularExpression bashRe("```bash\\n(.*?)```", 
                              QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatchIterator bashIter = bashRe.globalMatch(response);
        while (bashIter.hasNext()) {
            QRegularExpressionMatch match = bashIter.next();
            MockToolCall call;
            call.id = QUuid::createUuid().toString();
            call.name = "Bash";
            QJsonObject input;
            input["command"] = match.captured(1).trimmed();
            call.input = input;
            parsedCalls.append(call);
            break; 
        }
    }

    return parsedCalls;
}
} // namespace

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

    auto calls = parseToolCalls(response);
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

    auto calls = parseToolCalls(response);
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

    auto calls = parseToolCalls(response);
    REQUIRE(calls.size() == 1);
    REQUIRE(calls.first().name == "TestTool");
}

TEST_CASE("Parser handles responses with multiple potential tool calls but pick only the first", "[Parser][Advanced]") {
    QString response = R"(
<name>FirstTool</name><input>{"id": 1}</input>
<name>SecondTool</name><input>{"id": 2}</input>
)";

    auto calls = parseToolCalls(response);
    REQUIRE(calls.size() == 1);
    REQUIRE(calls.first().name == "FirstTool");
}
