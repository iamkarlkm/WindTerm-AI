# Cloud Integration Module
add_library(WindTermCloud STATIC
    AWSClient.h
    AWSClient.cpp
)

target_link_libraries(WindTermCloud PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(WindTermCloud PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
