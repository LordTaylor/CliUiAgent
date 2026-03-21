#include <catch2/catch_test_macros.hpp>
#include "../src/core/tools/ToolUtils.h"
#include <QString>

using namespace CodeHex;

TEST_CASE("ToolUtils: isIgnored", "[ToolUtils]") {
    REQUIRE(ToolUtils::isIgnored("node_modules/test.js") == true);
    REQUIRE(ToolUtils::isIgnored("src/node_modules/test.js") == true);
    REQUIRE(ToolUtils::isIgnored("build/main.o") == true);
    REQUIRE(ToolUtils::isIgnored("src/build/main.o") == true);
    REQUIRE(ToolUtils::isIgnored(".git/config") == true);
    REQUIRE(ToolUtils::isIgnored(".agent/task.md") == true);
    REQUIRE(ToolUtils::isIgnored(".DS_Store") == true);
    REQUIRE(ToolUtils::isIgnored("src/main.pyc") == true);
    REQUIRE(ToolUtils::isIgnored("src/__pycache__/test.py") == true);
    REQUIRE(ToolUtils::isIgnored("bin/test.exe") == true);
    REQUIRE(ToolUtils::isIgnored("obj/test.o") == true);
    
    REQUIRE(ToolUtils::isIgnored("src/main.cpp") == false);
    REQUIRE(ToolUtils::isIgnored("docs/readme.md") == false);
}
