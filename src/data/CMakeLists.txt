add_library(codehex_data STATIC
    Attachment.h Attachment.cpp
    Message.h    Message.cpp
    Session.h    Session.cpp
    JsonSerializer.h JsonSerializer.cpp
)

target_include_directories(codehex_data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_data PUBLIC Qt6::Core nlohmann_json::nlohmann_json)
