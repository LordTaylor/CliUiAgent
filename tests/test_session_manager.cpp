#include <catch2/catch_test_macros.hpp>
#include <QTemporaryDir>
#include "../src/core/AppConfig.h"
#include "../src/core/SessionManager.h"
#include "../src/core/TokenCounter.h"

using namespace CodeHex;

TEST_CASE("SessionManager creates and lists sessions", "[SessionManager]") {
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    AppConfig config;
    config.setDataDir(tempDir.path());
    SessionManager mgr(&config);
    mgr.loadAll();
    CHECK(mgr.allSessions().isEmpty());
}

TEST_CASE("TokenCounter estimates are non-zero for non-empty text", "[core]") {
    CHECK(TokenCounter::estimate("Hello world") > 0);
    CHECK(TokenCounter::estimate("") == 0);
}
