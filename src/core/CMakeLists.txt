add_library(codehex_core STATIC
    AppConfig.h      AppConfig.cpp
    CrashHandler.h   CrashHandler.cpp
    TokenCounter.h   TokenCounter.cpp
    SessionManager.h SessionManager.cpp
    ChatController.h ChatController.cpp
    ToolExecutor.h   ToolExecutor.cpp
    AgentEngine.h    AgentEngine.cpp
                     AgentEngine_Loop.cpp
                     AgentEngine_Runner.cpp
                     AgentEngine_Tools.cpp
                     AgentEngine_Collab.cpp
    ResponseFilter.h ResponseFilter.cpp
    PromptManager.h  PromptManager.cpp
    ResponseParser.h ResponseParser.cpp
    ContextManager.h ContextManager.cpp
    LlmDiscoveryService.h LlmDiscoveryService.cpp
    ThemeManager.h   ThemeManager.cpp
    Tool.h
    tools/ToolUtils.h
    tools/ReadFileTool.h
    tools/WriteFileTool.h
    tools/ListDirectoryTool.h
    tools/BashTool.h
    tools/SearchFilesTool.h
    tools/GrepTool.h
    CodebaseVisualizer.h CodebaseVisualizer.cpp
    tools/VisualizeCodebaseTool.h
     tools/CompleteTaskTool.h
    tools/WebSearchTool.h
    tools/TakeScreenshotTool.h
    FileHotnessProvider.h
    FileHotnessProvider.cpp
    DiffUtils.cpp
    tools/ReplaceTool.h
    tools/SearchReplaceTool.h
    tools/MathLogicTool.h tools/MathLogicTool.cpp
    ProjectAuditor.h ProjectAuditor.cpp
    EnsembleManager.h EnsembleManager.cpp
    AgentRole.h
    ModelProfileManager.h ModelProfileManager.cpp
    ModelRouter.h    ModelRouter.cpp
    AgentPipeline.h  AgentPipeline.cpp
    AgentGraph.h     AgentGraph.cpp
    tools/GitTool.h
    tools/SearchRepoTool.h
    tools/ImportModelProfileTool.h
    tools/DownloadModelProfileTool.h
    HuggingFaceImporter.h HuggingFaceImporter.cpp
    rag/EmbeddingManager.h rag/EmbeddingManager.cpp
    rag/CodebaseIndexer.h  rag/CodebaseIndexer.cpp
)

target_include_directories(codehex_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(codehex_core PRIVATE ${CMAKE_SOURCE_DIR}/src/data)
target_link_libraries(codehex_core PUBLIC
    codehex_data
    codehex_cli
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    pybind11::embed
)
# ChatController depends on CLI and Scripting — use INTERFACE to avoid circular deps
# Actual linking happens in the executable target
target_include_directories(codehex_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_SOURCE_DIR}/src/scripting
)
