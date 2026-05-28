cmake_minimum_required(VERSION 3.16)
project(TerminalInfoPlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

add_library(terminal_info_plugin MODULE
    TerminalInfoPlugin.h
    TerminalInfoPlugin.cpp
)

target_include_directories(terminal_info_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(terminal_info_plugin PRIVATE Qt5::Core WindTermPluginSystem)

set_target_properties(terminal_info_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
)
