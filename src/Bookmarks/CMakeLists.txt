# Bookmarks Module
add_library(WindTermBookmarks STATIC
    BookmarkManager.h
    BookmarkManager.cpp
)

target_link_libraries(WindTermBookmarks PUBLIC
    Qt5::Core
    Qt5::Widgets
)

target_include_directories(WindTermBookmarks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
