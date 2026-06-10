# Script Engine Module
add_library(WindTermScriptEngine STATIC
    ScriptEngine.h
    ScriptEngine.cpp
)

target_link_libraries(WindTermScriptEngine PUBLIC
    Qt5::Core
)

target_include_directories(WindTermScriptEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
