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
)

add_test(NAME PerformanceBenchmark COMMAND performance_benchmark)
