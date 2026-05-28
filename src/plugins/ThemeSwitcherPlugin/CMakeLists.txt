cmake_minimum_required(VERSION 3.16)
project(ThemeSwitcherPlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

add_library(theme_switcher_plugin MODULE
    ThemeSwitcherPlugin.h
    ThemeSwitcherPlugin.cpp
)

target_include_directories(theme_switcher_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(theme_switcher_plugin PRIVATE Qt5::Core WindTermPluginSystem)

set_target_properties(theme_switcher_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
)
