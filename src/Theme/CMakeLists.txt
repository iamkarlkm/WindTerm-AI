# Theme Module
add_library(WindTermTheme STATIC
    ThemeManager.h
    ThemeManager.cpp
    ThemeConfig.h
    ThemeConfig.cpp
    TerminalTheme.h
    TerminalTheme.cpp
    ColorSchemeEditor.h
    ColorSchemeEditor.cpp
)

target_link_libraries(WindTermTheme PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

target_include_directories(WindTermTheme PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
