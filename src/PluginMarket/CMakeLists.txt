# Plugin Market Module
add_library(WindTermPluginMarket STATIC
    PluginMarketManager.h
    PluginMarketManager.cpp
)

target_link_libraries(WindTermPluginMarket PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(WindTermPluginMarket PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
