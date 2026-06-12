# Local AI Model Module
add_library(WindTermAIModel STATIC
    LocalAIModel.h
    LocalAIModel.cpp
)

target_link_libraries(WindTermAIModel PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(WindTermAIModel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
