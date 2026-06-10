# Container Module
add_library(WindTermContainer STATIC
    ContainerManager.h
    ContainerManager.cpp
)

target_link_libraries(WindTermContainer PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(WindTermContainer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
