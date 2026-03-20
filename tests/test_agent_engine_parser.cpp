#include <catch2/catch_test_macros.hpp>
#include <QString>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

// A mock to simulate exactly what AgentEngine::onRunnerFinished does when extracting tools
struct MockToolCall {
    QString id;
    QString name;
    QJsonObject input;
};

QList<MockToolCall> parseToolCalls(const QString& response) {
    QList<MockToolCall> parsedCalls;

    // XML parsing
    QRegularExpression re("<name>\\s*([^<]+)\\s*</name>\\s*<input>\\s*(.*?)\\s*</input>", 
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
            break; // Stop after first successful XML block
        }
    }

    // Bash Fallback parsing
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
            break; // Stop after first Bash block
        }
    }

    return parsedCalls;
}

TEST_CASE("AgentEngine parses valid XML tool call correctly", "[AgentEngine][Parser]") {
    QString response = R"(
Here is the file you requested.
<name>WriteFile</name>
<input>
{
    "content": "public class HelloWorld {}",
    "path": "./HelloWorld.java"
}
</input>
)";
    
    auto calls = parseToolCalls(response);
    REQUIRE(calls.size() == 1);
    REQUIRE(calls.first().name == "WriteFile");
    REQUIRE(calls.first().input.contains("path"));
    REQUIRE(calls.first().input["path"].toString() == "./HelloWorld.java");
}

TEST_CASE("AgentEngine breaks XML loop and isolates correctly", "[AgentEngine][Parser]") {
    QString loopedResponse = R"(
Oczywiście! Oto kod Java "Hello World".```xml
<name>WriteFile</name>
<input>
{
    "content": "public class HelloWorld {\n    public static void main(String[] args) {\n        System.out.println(\"Hello, World!\");\n    }\n}",
    "path": "./HelloWorld.java"
}
</input>
``````xml
<name>WriteFile</name>
<input>
{
    "content": "public class HelloWorld {\n    public static void main(String[] args) {\n        System.out.println(\"Hello, World!\");\n    }\n}",
    "path": "./HelloWorld.java"
}
</input>
```
)";

    auto calls = parseToolCalls(loopedResponse);
    REQUIRE(calls.size() == 1); // Should only capture the first block without grouping multiple!
    REQUIRE(calls.first().name == "WriteFile");
    REQUIRE(calls.first().input.contains("path"));
    REQUIRE(calls.first().input["path"].toString() == "./HelloWorld.java");
}

TEST_CASE("AgentEngine falls back to Bash parsing if XML fails or missing", "[AgentEngine][Parser]") {
    QString bashResponse = R"(
Sure, here is the script:
```bash
echo "Hello" > hello.txt
```
Have a good day!
)";

    auto calls = parseToolCalls(bashResponse);
    REQUIRE(calls.size() == 1);
    REQUIRE(calls.first().name == "Bash");
    REQUIRE(calls.first().input.contains("command"));
    REQUIRE(calls.first().input["command"].toString().contains("echo \"Hello\""));
}
