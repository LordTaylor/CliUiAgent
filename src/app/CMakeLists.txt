add_library(codehex_app STATIC
    Application.h Application.cpp
)

target_include_directories(codehex_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_app PUBLIC
    codehex_ui
    codehex_core
    codehex_cli
    codehex_audio
    codehex_scripting
    Qt6::Core Qt6::Widgets
)
