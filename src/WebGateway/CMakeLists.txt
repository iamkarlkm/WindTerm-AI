# Web Gateway Module
add_library(WindTermWebGateway STATIC
    WebTerminalGateway.h
    WebTerminalGateway.cpp
    TerminalSession.h
    TerminalSession.cpp
)

target_link_libraries(WindTermWebGateway PUBLIC
    Qt5::Core
    Qt5::Network
    Qt5::WebSockets
)

target_include_directories(WindTermWebGateway PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
