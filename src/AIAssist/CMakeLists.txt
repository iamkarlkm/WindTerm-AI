# AI Assist Module
add_library(WindTermAIAssist STATIC
    CommandCompletion.h
    CommandCompletion.cpp
)

target_link_libraries(WindTermAIAssist PUBLIC
    Qt5::Core
)

target_include_directories(WindTermAIAssist PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
