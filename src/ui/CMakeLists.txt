add_library(codehex_ui STATIC
    MainWindow.h MainWindow.cpp
    chat/ChatControlBanner.h chat/ChatControlBanner.cpp
    chat/ChatView.h         chat/ChatView.cpp
    chat/MessageModel.h     chat/MessageModel.cpp
    chat/MessageDelegate.h  chat/MessageDelegate.cpp
    chat/ToolApprovalDialog.h chat/ToolApprovalDialog.cpp
    input/InputPanel.h      input/InputPanel.cpp
    input/AttachmentButton.h input/AttachmentButton.cpp
    input/VoiceButton.h     input/VoiceButton.cpp
    widgets/TerminalWidget.h widgets/TerminalWidget.cpp
    widgets/TerminalPanel.h  widgets/TerminalPanel.cpp
    widgets/ThemeManager.h   widgets/ThemeManager.cpp
    workfolder/WorkFolderSelector.h workfolder/WorkFolderSelector.cpp
    workfolder/WorkFolderPanel.h    workfolder/WorkFolderPanel.cpp
    settings/SettingsDialog.h       settings/SettingsDialog.cpp
    skills/SkillsDialog.h           skills/SkillsDialog.cpp
    plugins/PluginsDialog.h         plugins/PluginsDialog.cpp
    session/SessionPanel.h  session/SessionPanel.cpp
    help/HelpDialog.h       help/HelpDialog.cpp
)

target_include_directories(codehex_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_ui PUBLIC
    codehex_data
    codehex_core
    codehex_cli
    codehex_audio
    Qt6::Core Qt6::Widgets
)
