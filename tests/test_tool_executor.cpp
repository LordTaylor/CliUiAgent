#include <catch2/catch_test_macros.hpp>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "../src/core/ToolExecutor.h"
#include "../src/data/ToolCall.h"

using namespace CodeHex;

TEST_CASE("ToolExecutor ReadFile", "[ToolExecutor]") {
    QTemporaryDir dir;
    QFile file(dir.path() + "/test.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write("Hello World");
    file.close();

    ToolExecutor executor;
    ToolCall call{"1", "ReadFile", {{"path", "test.txt"}}};
    ToolResult result = executor.executeSync(call, dir.path());

    REQUIRE(result.isError == false);
    REQUIRE(result.content == "Hello World");
}

TEST_CASE("ToolExecutor WriteFile", "[ToolExecutor]") {
    QTemporaryDir dir;
    ToolExecutor executor;
    ToolCall call{"2", "WriteFile", {{"path", "new.txt"}, {"content", "New Content"}}};
    ToolResult result = executor.executeSync(call, dir.path());

    REQUIRE(result.isError == false);
    QFile file(dir.path() + "/new.txt");
    REQUIRE(file.exists());
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    REQUIRE(QString(file.readAll()) == "New Content");
}

TEST_CASE("ToolExecutor RunCommand", "[ToolExecutor]") {
    ToolExecutor executor;
    ToolCall call{"3", "RunCommand", {{"command", "echo 'hello'"}}};
    ToolResult result = executor.executeSync(call, "");

    REQUIRE(result.isError == false);
    REQUIRE(result.content.trimmed() == "hello");
}

TEST_CASE("ToolExecutor ListDirectory", "[ToolExecutor]") {
    QTemporaryDir dir;
    QDir(dir.path()).mkdir("subdir");
    QFile file(dir.path() + "/subdir/file.txt");
    file.open(QIODevice::WriteOnly);
    file.close();

    ToolExecutor executor;
    ToolCall call{"4", "ListDirectory", {{"path", "."}}};
    ToolResult result = executor.executeSync(call, dir.path());

    REQUIRE(result.isError == false);
    REQUIRE(result.content.contains("subdir"));
    REQUIRE(result.content.contains("file.txt"));
    REQUIRE(result.content.contains("KB")); // Check for size detail
}

TEST_CASE("ToolExecutor Search", "[ToolExecutor]") {
    QTemporaryDir dir;
    QFile file(dir.path() + "/search_test.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write("Line 1: apple\nLine 2: banana\nLine 3: cherry apple");
    file.close();

    ToolExecutor executor;
    ToolCall call{"5", "Search", {{"query", "apple"}}};
    ToolResult result = executor.executeSync(call, dir.path());

    REQUIRE(result.isError == false);
    REQUIRE(result.content.contains("Line 1"));
    REQUIRE(result.content.contains("Line 3"));
    REQUIRE(!result.content.contains("Line 2"));
}

TEST_CASE("ToolExecutor Replace", "[ToolExecutor]") {
    QTemporaryDir dir;
    QFile file(dir.path() + "/replace_test.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write("Hello World");
    file.close();

    ToolExecutor executor;
    ToolCall call{"6", "Replace", {{"path", "replace_test.txt"}, {"pattern", "World"}, {"replacement", "CodeHex"}}};
    ToolResult result = executor.executeSync(call, dir.path());

    REQUIRE(result.isError == false);
    
    QFile checkFile(dir.path() + "/replace_test.txt");
    checkFile.open(QIODevice::ReadOnly | QIODevice::Text);
    REQUIRE(QString(checkFile.readAll()) == "Hello CodeHex");
}
