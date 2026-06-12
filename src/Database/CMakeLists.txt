# Database Module
add_library(WindTermDatabase STATIC
    DatabaseClient.h
    DatabaseClient.cpp
)

target_link_libraries(WindTermDatabase PUBLIC
    Qt5::Core
    Qt5::Sql
)

target_include_directories(WindTermDatabase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
