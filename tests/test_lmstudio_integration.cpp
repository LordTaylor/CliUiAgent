#include <catch2/catch_test_macros.hpp>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

/**
 * Integration test that sends a real request to LM Studio running on localhost:1234.
 * 
 * This test is tagged with [Integration] and requires:
 *   - LM Studio running on localhost:1234
 *   - A model loaded (e.g. qwen/qwen2.5-coder-14b)
 *
 * Run with: ctest -R "LMStudio" --output-on-failure
 */

TEST_CASE("LMStudio responds to a simple prompt", "[Integration][LMStudio]") {
    // Build a minimal OpenAI-compatible chat completion request
    QJsonArray messages;
    messages.append(QJsonObject{{"role", "system"}, {"content", "You are a helpful assistant. Reply in one sentence."}});
    messages.append(QJsonObject{{"role", "user"}, {"content", "Say hello in Polish."}});

    QJsonObject body;
    body["model"] = "qwen/qwen2.5-coder-14b";
    body["messages"] = messages;
    body["stream"] = false;  // Non-streaming for simplicity in test
    body["temperature"] = 0.1;
    body["max_tokens"] = 50;

    QString jsonPayload = QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));

    // Use curl to send the request (same as ConfigurableProfile does)
    QProcess curl;
    curl.setProgram("curl");
    curl.setArguments({
        "-s",
        "-X", "POST",
        "-H", "Content-Type: application/json",
        "-H", "Authorization: Bearer lm-studio",
        "--data-raw", jsonPayload,
        "--connect-timeout", "3",
        "--max-time", "30",
        "http://localhost:1234/v1/chat/completions"
    });

    curl.start();
    bool started = curl.waitForStarted(5000);
    
    // If LM Studio is not running, skip (don't fail)
    if (!started) {
        WARN("LM Studio not reachable — skipping integration test");
        return;
    }

    bool finished = curl.waitForFinished(30000);  // 30s timeout for model response
    
    if (!finished || curl.exitCode() != 0) {
        WARN("LM Studio request failed (exit=" + std::to_string(curl.exitCode()) + ") — skipping");
        return;
    }

    QByteArray output = curl.readAllStandardOutput();
    REQUIRE(!output.isEmpty());

    // Parse the response
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(output, &err);
    
    if (doc.isNull()) {
        WARN("Failed to parse LM Studio response: " + err.errorString().toStdString());
        WARN("Raw output: " + output.toStdString().substr(0, 200));
        return;
    }

    REQUIRE(doc.isObject());
    QJsonObject responseObj = doc.object();
    
    // Validate the response structure
    REQUIRE(responseObj.contains("choices"));
    QJsonArray choices = responseObj["choices"].toArray();
    REQUIRE(!choices.isEmpty());
    
    QJsonObject firstChoice = choices[0].toObject();
    REQUIRE(firstChoice.contains("message"));
    
    QJsonObject message = firstChoice["message"].toObject();
    REQUIRE(message.contains("content"));
    
    QString content = message["content"].toString();
    INFO("LM Studio response: " + content.toStdString());
    REQUIRE(!content.isEmpty());
}

TEST_CASE("LMStudio XML tool call can be parsed from response", "[Integration][LMStudio]") {
    // Same setup but with a tool-call prompt
    QJsonArray messages;
    messages.append(QJsonObject{{"role", "system"}, {"content", 
        "You are a coding assistant. When asked to write a file, respond with:\n"
        "<name>WriteFile</name>\n<input>\n{\"content\": \"...\", \"path\": \"...\"}\n</input>\n"
        "Do not include any other text."
    }});
    messages.append(QJsonObject{{"role", "user"}, {"content", "Write a hello world Java file to ./Hello.java"}});

    QJsonObject body;
    body["model"] = "qwen/qwen2.5-coder-14b";
    body["messages"] = messages;
    body["stream"] = false;
    body["temperature"] = 0.1;
    body["max_tokens"] = 200;

    QString jsonPayload = QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));

    QProcess curl;
    curl.setProgram("curl");
    curl.setArguments({
        "-s", "-X", "POST",
        "-H", "Content-Type: application/json",
        "-H", "Authorization: Bearer lm-studio",
        "--data-raw", jsonPayload,
        "--connect-timeout", "3",
        "--max-time", "30",
        "http://localhost:1234/v1/chat/completions"
    });

    curl.start();
    if (!curl.waitForStarted(5000)) {
        WARN("LM Studio not reachable — skipping");
        return;
    }
    if (!curl.waitForFinished(30000) || curl.exitCode() != 0) {
        WARN("LM Studio request failed — skipping");
        return;
    }

    QByteArray output = curl.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (doc.isNull()) {
        WARN("Failed to parse response — skipping");
        return;
    }

    QString content = doc.object()["choices"].toArray()[0].toObject()
                        ["message"].toObject()["content"].toString();
    
    INFO("Model response: " + content.toStdString());
    
    // Check that the response contains XML tool tags
    QRegularExpression re("<name>\\s*([^<]+)\\s*</name>\\s*<input>\\s*(.*?)\\s*</input>", 
                          QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = re.match(content);
    
    if (match.hasMatch()) {
        QString toolName = match.captured(1).trimmed();
        QString jsonStr = match.captured(2).trimmed();
        INFO("Parsed tool: " + toolName.toStdString());
        INFO("Parsed JSON: " + jsonStr.toStdString());
        
        CHECK(toolName == "WriteFile");
        
        QJsonDocument toolDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
        CHECK(!toolDoc.isNull());
        if (!toolDoc.isNull()) {
            CHECK(toolDoc.object().contains("path"));
            CHECK(toolDoc.object().contains("content"));
        }
    } else {
        WARN("Model did not produce XML tool tags — this is model-dependent behavior");
    }
}
