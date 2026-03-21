find_package(Catch2 REQUIRED)

add_executable(codehex_tests
    test_main.cpp
    test_json_serializer.cpp
    test_message_model.cpp
    test_session_manager.cpp
    test_cli_runner.cpp
    test_tool_executor.cpp
    test_chat_controller.cpp
    test_agent_engine_parser.cpp
    test_response_parser.cpp
    test_tool_utils.cpp
    test_agent_engine_parser_advanced.cpp
    test_response_filter_streaming.cpp
    test_context_manager.cpp
    test_lmstudio_integration.cpp
    test_java_codegen.cpp
)

qt_add_resources(codehex_tests "resources"
    PREFIX "/"
    FILES
        "../resources/prompts/base.txt"
        "../resources/prompts/explorer.txt"
        "../resources/prompts/executor.txt"
        "../resources/prompts/reviewer.txt"
)

target_link_libraries(codehex_tests PRIVATE
    codehex_data
    codehex_core
    codehex_cli
    codehex_ui
    codehex_scripting
    codehex_audio
    Catch2::Catch2WithMain
    Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Test
    lua::lua sol2::sol2 pybind11::embed
)

include(CTest)
include(Catch)
catch_discover_tests(codehex_tests)

add_executable(prompt_tests PromptTesting.cpp)
target_link_libraries(prompt_tests PRIVATE
    codehex_data
    codehex_core
    Qt6::Core Qt6::Test
)
add_test(NAME prompt_tests COMMAND prompt_tests)
