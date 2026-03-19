#include <catch2/catch_test_macros.hpp>
#include <QTemporaryFile>
#include "../src/data/JsonSerializer.h"
#include "../src/data/Message.h"
#include "../src/data/Session.h"

using namespace CodeHex;

TEST_CASE("Message round-trips through JSON", "[data]") {
    Message msg;
    msg.id = QUuid::createUuid();
    msg.role = Message::Role::Assistant;
    msg.contentType = Message::ContentType::Text;
    msg.text = "Hello, world!";
    msg.timestamp = QDateTime::currentDateTimeUtc();
    msg.tokenCount = 42;

    const QJsonObject obj = msg.toJson();
    const Message restored = Message::fromJson(obj);

    CHECK(restored.id == msg.id);
    CHECK(restored.role == msg.role);
    CHECK(restored.text == msg.text);
    CHECK(restored.tokenCount == msg.tokenCount);
}

TEST_CASE("Session serializes and deserializes messages", "[data]") {
    Session s = Session::createNew("claude", "claude-sonnet-4-6");
    s.title = "Test session";

    Message m1;
    m1.id = QUuid::createUuid();
    m1.role = Message::Role::User;
    m1.text = "Question?";
    m1.timestamp = QDateTime::currentDateTimeUtc();

    s.appendMessage(m1);

    const QJsonObject obj = s.toJson();
    const Session restored = Session::fromJson(obj);

    CHECK(restored.title == s.title);
    CHECK(restored.messages.size() == 1);
    CHECK(restored.messages.first().text == "Question?");
}

TEST_CASE("JsonSerializer writes and reads file", "[data]") {
    QTemporaryFile tmp;
    REQUIRE(tmp.open());
    tmp.close();

    const QJsonObject orig = {{"key", "value"}, {"num", 42}};
    REQUIRE(JsonSerializer::writeFile(tmp.fileName(), orig));

    const QJsonObject read = JsonSerializer::readFile(tmp.fileName());
    CHECK(read["key"].toString() == "value");
    CHECK(read["num"].toInt() == 42);
}
