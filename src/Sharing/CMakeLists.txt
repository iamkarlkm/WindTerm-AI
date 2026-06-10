# Terminal Sharing Module
add_library(WindTermSharing STATIC
    TerminalSharingManager.h
    TerminalSharingManager.cpp
)

target_link_libraries(WindTermSharing PUBLIC
    Qt5::Core
)

target_include_directories(WindTermSharing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
