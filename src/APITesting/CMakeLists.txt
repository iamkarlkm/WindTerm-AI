# API Testing Module
add_library(WindTermAPITesting STATIC
    APITestClient.h
    APITestClient.cpp
)

target_link_libraries(WindTermAPITesting PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(WindTermAPITesting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
