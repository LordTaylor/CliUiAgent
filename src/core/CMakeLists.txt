add_library(codehex_core STATIC
    AppConfig.h      AppConfig.cpp
    TokenCounter.h   TokenCounter.cpp
    SessionManager.h SessionManager.cpp
    ChatController.h ChatController.cpp
    ToolExecutor.h   ToolExecutor.cpp
    AgentEngine.h    AgentEngine.cpp
    Tool.h
    tools/ToolUtils.h
    tools/ReadFileTool.h
    tools/WriteFileTool.h
    tools/ListDirectoryTool.h
    tools/BashTool.h
    tools/SearchFilesTool.h
    tools/GrepTool.h
    tools/ReplaceTool.h
    tools/GitTool.h
)

target_include_directories(codehex_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_core PUBLIC
    codehex_data
    Qt6::Core
    Qt6::Concurrent
)
# ChatController depends on CLI and Scripting — use INTERFACE to avoid circular deps
# Actual linking happens in the executable target
target_include_directories(codehex_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_SOURCE_DIR}/src/scripting
)
