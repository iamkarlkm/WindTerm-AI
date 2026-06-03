# Utils Module
add_library(WindTermUtils STATIC
    PerformanceMonitor.h
    PerformanceMonitor.cpp
    UnicodeUtil.h
    UnicodeUtil.cpp
    TerminalMacro.h
    TerminalMacro.cpp
)

target_link_libraries(WindTermUtils PUBLIC
    Qt5::Core
    Qt5::Widgets
)

target_include_directories(WindTermUtils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
