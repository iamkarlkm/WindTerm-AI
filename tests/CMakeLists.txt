cmake_minimum_required(VERSION 3.16)

project(WindTermExtensionsTests)

# 渲染器测试应用
if(BUILD_RENDERER)
    add_executable(renderer_test
        main.cpp
        RendererTestWindow.cpp
        RendererTestWindow.h
    )
    
    target_include_directories(renderer_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(renderer_test PRIVATE
        WindTermRenderer
        WindTermBuffer
        Qt5::Widgets
        Qt5::OpenGL
    )
    
    if(APPLE)
        find_library(METAL_FRAMEWORK Metal)
        find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)
        target_link_libraries(renderer_test PRIVATE
            ${METAL_FRAMEWORK}
            ${QUARTZ_CORE_FRAMEWORK}
        )
    endif()
endif()

# 文本缓冲单元测试
if(BUILD_BUFFER)
    find_package(Qt5Test QUIET)
    if(Qt5Test_FOUND)
        add_executable(buffer_test
            CircularTextBufferTest.cpp
        )
        
        target_include_directories(buffer_test PRIVATE
            ${CMAKE_SOURCE_DIR}/src
        )
        
        target_link_libraries(buffer_test PRIVATE
            WindTermBuffer
            Qt5::Core
            Qt5::Test
        )
        
        add_test(NAME CircularTextBufferTest COMMAND buffer_test)
    else()
        message(STATUS "Qt5Test not found, skipping buffer unit tests")
    endif()
endif()

# ANSI 解析器单元测试
find_package(Qt5Test QUIET)
if(Qt5Test_FOUND AND BUILD_RENDERER)
    add_executable(ansi_parser_test
        AnsiParserTest.cpp
    )
    
    target_include_directories(ansi_parser_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(ansi_parser_test PRIVATE
        WindTermTerminal
        Qt5::Core
        Qt5::Test
    )
    
    add_test(NAME AnsiParserTest COMMAND ansi_parser_test)
    
    # TerminalState 单元测试
    add_executable(terminal_state_test
        TerminalStateTest.cpp
    )
    
    target_include_directories(terminal_state_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(terminal_state_test PRIVATE
        WindTermTerminal
        Qt5::Core
        Qt5::Test
    )
    
    add_test(NAME TerminalStateTest COMMAND terminal_state_test)
    
    # UrlDetector 单元测试
    add_executable(url_detector_test
        UrlDetectorTest.cpp
    )
    
    target_include_directories(url_detector_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(url_detector_test PRIVATE
        WindTermWidget
        Qt5::Core
        Qt5::Test
    )
    
    add_test(NAME UrlDetectorTest COMMAND url_detector_test)
    
    # UnicodeUtil 单元测试
    add_executable(unicode_util_test
        UnicodeUtilTest.cpp
    )
    
    target_include_directories(unicode_util_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(unicode_util_test PRIVATE
        WindTermRenderer
        Qt5::Core
        Qt5::Test
    )
    
    add_test(NAME UnicodeUtilTest COMMAND unicode_util_test)
endif()

# 性能基准测试
add_executable(performance_benchmark
    PerformanceBenchmark.cpp
    PerformanceBenchmark.h
    BenchmarkMain.cpp
)

target_include_directories(performance_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(performance_benchmark PRIVATE
    Qt5::Core
    Qt5::Gui
    WindTermRenderer
    WindTermTerminal
)

add_test(NAME PerformanceBenchmark COMMAND performance_benchmark)
