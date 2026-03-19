add_library(codehex_cli STATIC
    CliProfile.h
    CliRunner.h     CliRunner.cpp
    ClaudeProfile.h ClaudeProfile.cpp
    OllamaProfile.h OllamaProfile.cpp
    GptProfile.h    GptProfile.cpp
)

target_include_directories(codehex_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_cli PUBLIC Qt6::Core)
