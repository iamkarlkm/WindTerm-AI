# Widget Module
add_library(WindTermWidget STATIC
    TerminalWidget.h
    TerminalWidget.cpp
    TerminalInputHandler.h
    TerminalInputHandler.cpp
    TerminalSearchEngine.h
    TerminalSearchEngine.cpp
    TerminalScrollBar.h
    TerminalScrollBar.cpp
    TerminalSelection.h
    TerminalSelection.cpp
    TerminalClipboard.h
    TerminalClipboard.cpp
    TerminalMultiplexer.h
    TerminalMultiplexer.cpp
    ShortcutManager.h
    ShortcutManager.cpp
    BatchRenderer.h
    BatchRenderer.cpp
    NotificationManager.h
    NotificationManager.cpp
    CommandPalette.h
    CommandPalette.cpp
    TabBar.h
    TabBar.cpp
    WorkspaceManager.h
    WorkspaceManager.cpp
    QuickConnectPanel.h
    QuickConnectPanel.cpp
    SynchronizedInput.h
    SynchronizedInput.cpp
)

target_link_libraries(WindTermWidget PUBLIC
    WindTermTerminal
    WindTermBuffer
    WindTermRenderer
    WindTermSsh
    WindTermBookmarks
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

target_include_directories(WindTermWidget PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
