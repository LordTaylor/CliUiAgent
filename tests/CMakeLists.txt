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
