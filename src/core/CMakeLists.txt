add_library(codehex_core STATIC
    AppConfig.h    AppConfig.cpp
    TokenCounter.h TokenCounter.cpp
    SessionManager.h SessionManager.cpp
    ChatController.h ChatController.cpp
)

target_include_directories(codehex_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_core PUBLIC
    codehex_data
    Qt6::Core
)
# ChatController depends on CLI and Scripting — use INTERFACE to avoid circular deps
# Actual linking happens in the executable target
target_include_directories(codehex_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_SOURCE_DIR}/src/scripting
)
