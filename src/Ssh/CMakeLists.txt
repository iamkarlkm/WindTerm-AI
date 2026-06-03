# SSH Module
add_library(WindTermSsh STATIC
    SshConnection.h
    SshConnection.cpp
    SshConnectionPool.h
    SshConnectionPool.cpp
    SshTunnelManager.h
    SshTunnelManager.cpp
)

target_link_libraries(WindTermSsh PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(WindTermSsh PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
