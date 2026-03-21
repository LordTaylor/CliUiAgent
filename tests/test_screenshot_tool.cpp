#include <catch2/catch_test_macros.hpp>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include "../src/core/ToolExecutor.h"
#include "../src/data/ToolCall.h"

using namespace CodeHex;

TEST_CASE("ToolExecutor TakeScreenshot", "[ToolExecutor]") {
    ToolExecutor executor;
    // The alias "Screenshot" was registered in ToolExecutor.cpp
    ToolCall call{"screenshot_test_1", "Screenshot", {}};
    
    // Note: This test requires a GUI environment to actually grab a window.
    // In CI/Server environments without a display, this might fail or return an error.
    // We'll check if it at least doesn't crash and returns a path if successful.
    
    ToolResult result = executor.executeSync(call, "");

    // If we're in an environment without a screen, it might return an error.
    // But if it succeeds, we verify the output.
    if (!result.isError) {
        REQUIRE(!result.content.isEmpty());
        QFile file(result.content);
        REQUIRE(file.exists());
        
        // Verify it's a valid image
        QImageReader reader(result.content);
        REQUIRE(reader.canRead());
        
        // Cleanup
        file.remove();
    } else {
        // If it failed, check the error message
        REQUIRE((result.content.contains("Failed") || result.content.contains("No screen")));
    }
}
