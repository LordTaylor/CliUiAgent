add_library(codehex_ui STATIC
    MainWindow.h MainWindow.cpp
    chat/ChatView.h         chat/ChatView.cpp
    chat/MessageModel.h     chat/MessageModel.cpp
    chat/MessageDelegate.h  chat/MessageDelegate.cpp
    input/InputPanel.h      input/InputPanel.cpp
    input/AttachmentButton.h input/AttachmentButton.cpp
    input/VoiceButton.h     input/VoiceButton.cpp
    console/ConsoleWidget.h console/ConsoleWidget.cpp
    workfolder/WorkFolderSelector.h workfolder/WorkFolderSelector.cpp
    session/SessionPanel.h  session/SessionPanel.cpp
)

target_include_directories(codehex_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_ui PUBLIC
    codehex_data
    codehex_core
    codehex_cli
    codehex_audio
    Qt6::Core Qt6::Widgets
)
