add_library(codehex_cli STATIC
    CliProfile.h
    ProfileEntry.h
    CliRunner.h            CliRunner.cpp
    OllamaProfile.h        OllamaProfile.cpp
    ConfigurableProfile.h  ConfigurableProfile.cpp
)

target_include_directories(codehex_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_cli PUBLIC Qt6::Core)
