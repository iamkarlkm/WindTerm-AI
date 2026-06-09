# History Module
add_library(WindTermHistory STATIC
    SessionHistory.h
    SessionHistory.cpp
)

target_link_libraries(WindTermHistory PUBLIC
    Qt5::Core
)

target_include_directories(WindTermHistory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
