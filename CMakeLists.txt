cmake_minimum_required(VERSION 3.25)

project(CodeHex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# --- Qt6 ---
find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Multimedia MultimediaWidgets Network Concurrent Test Svg
)
qt_standard_project_setup()

# --- Conan-generated find modules (only if using conan preset) ---
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

find_package(nlohmann_json REQUIRED)
find_package(lua REQUIRED)
find_package(sol2 REQUIRED)
find_package(pybind11 REQUIRED)

# --- Sub-libraries ---
add_subdirectory(src/data)
add_subdirectory(src/core)
add_subdirectory(src/cli)
add_subdirectory(src/audio)
add_subdirectory(src/scripting)
add_subdirectory(src/ui)
add_subdirectory(src/app)

# --- Tests ---
enable_testing()
add_subdirectory(tests)

# --- Main executable ---
set(MACOS_ICONS resources/icons/app.icns)
set_source_files_properties(${MACOS_ICONS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

qt_add_executable(CodeHex src/main.cpp)
target_sources(CodeHex PRIVATE ${MACOS_ICONS})

qt_add_resources(CodeHex "resources"
    PREFIX "/"
    FILES
        resources/stylesheets/dark.qss
        resources/stylesheets/theme_template.qss
        resources/icons/send.svg
        resources/icons/stop.svg
        resources/icons/attach.svg
        resources/icons/mic.svg
        resources/icons/folder.svg
        resources/icons/settings.svg
        resources/icons/skills.svg
        resources/icons/plugins.svg
        resources/icons/privacy.svg
        resources/icons/power.svg
        resources/icons/bug.svg
        resources/icons/app.png
        resources/icons/app.icns
        resources/prompts/base.txt
        resources/prompts/explorer.txt
        resources/prompts/executor.txt
        resources/prompts/reviewer.txt
        resources/prompts/rag.txt
        resources/prompts/refactor.txt
        resources/prompts/architect.txt
        resources/prompts/debugger.txt
        resources/prompts/security_auditor.txt
        resources/prompts/techniques/tdd.txt
        resources/prompts/techniques/clean_code.txt
        resources/prompts/techniques/performance.txt
)

qt_add_resources(CodeHex "help"
    PREFIX "/help/en"
    BASE "docs/help/en"
    FILES
        docs/help/en/index.md
        docs/help/en/getting-started.md
        docs/help/en/ui-guide.md
        docs/help/en/autonomous-agent.md
        docs/help/en/lm-studio.md
        docs/help/en/sessions.md
        docs/help/en/cli-profiles.md
        docs/help/en/scripting.md
        docs/help/en/voice-and-attachments.md
        docs/help/en/keyboard-shortcuts.md
        docs/help/en/ci-cd.md
)
qt_add_resources(CodeHex "help_pl"
    PREFIX "/help/pl"
    BASE "docs/help"
    FILES
        docs/help/index.md
        docs/help/ui-guide.md
        docs/help/ci-cd.md
)

target_link_libraries(CodeHex PRIVATE
    codehex_app
    codehex_ui
    codehex_core
    codehex_cli
    codehex_data
    codehex_audio
    codehex_scripting
    Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Network Qt6::Concurrent Qt6::Svg
    nlohmann_json::nlohmann_json
    lua::lua
    sol2::sol2
    pybind11::embed
)

set_target_properties(CodeHex PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "CodeHex"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.codehex.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_ICON_FILE "app.icns"
)
