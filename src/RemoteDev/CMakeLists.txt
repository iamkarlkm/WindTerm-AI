# Remote Development Module
add_library(WindTermRemoteDev STATIC
    RemoteDevelopmentManager.h
    RemoteDevelopmentManager.cpp
)

target_link_libraries(WindTermRemoteDev PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(WindTermRemoteDev PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
