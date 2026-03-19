add_library(codehex_audio STATIC
    AudioRecorder.h AudioRecorder.cpp
    AudioPlayer.h   AudioPlayer.cpp
)

target_include_directories(codehex_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_audio PUBLIC Qt6::Core Qt6::Multimedia)
