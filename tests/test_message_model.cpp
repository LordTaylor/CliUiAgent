#include <catch2/catch_test_macros.hpp>
#include "../src/data/Session.h"
#include "../src/ui/chat/MessageModel.h"

using namespace CodeHex;

static Message makeMsg(const QString& text, Message::Role role = Message::Role::User) {
    Message m;
    m.id = QUuid::createUuid();
    m.role = role;
    m.text = text;
    m.timestamp = QDateTime::currentDateTimeUtc();
    return m;
}

TEST_CASE("MessageModel loads last 10 messages", "[ui]") {
    Session s = Session::createNew("claude", "test");
    for (int i = 0; i < 25; ++i) {
        s.appendMessage(makeMsg(QString("msg %1").arg(i)));
    }

    MessageModel model;
    model.setSession(&s);

    CHECK(model.rowCount() == 10);
}

TEST_CASE("MessageModel canLoadMore returns true when >10 messages", "[ui]") {
    Session s = Session::createNew("claude", "test");
    for (int i = 0; i < 15; ++i) {
        s.appendMessage(makeMsg(QString("msg %1").arg(i)));
    }

    MessageModel model;
    model.setSession(&s);

    CHECK(model.canLoadMore());

    model.loadMoreMessages();
    CHECK(model.rowCount() == 15);
    CHECK_FALSE(model.canLoadMore());
}

TEST_CASE("MessageModel appendMessage increases row count", "[ui]") {
    Session s = Session::createNew("claude", "test");
    MessageModel model;
    model.setSession(&s);
    CHECK(model.rowCount() == 0);

    s.appendMessage(makeMsg("hello"));
    model.appendMessage(s.messages.last());
    CHECK(model.rowCount() == 1);
}
