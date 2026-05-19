cmake_minimum_required(VERSION 3.16)

project(WindTermExtensions
    VERSION 0.2.0
    DESCRIPTION "WindTerm Extensions - GPU Accelerated Rendering Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 构建选项
option(BUILD_RENDERER "Build GPU Renderer Module" ON)
option(BUILD_BUFFER "Build Circular Text Buffer" ON)
option(BUILD_TESTS "Build Unit Tests" OFF)
option(WITH_METAL "Enable Metal Backend (macOS)" ON)
option(WITH_OPENGL "Enable OpenGL Backend" ON)
option(WITH_DIRECTX "Enable DirectX12 Backend (Windows)" ON)

# 查找依赖
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

if(BUILD_RENDERER)
    message(STATUS "Building GPU Renderer Module")
    
    # 渲染器源文件
    set(RENDERER_SOURCES
        src/Renderer/GPURenderer.cpp
        src/Renderer/OpenGLRenderer.cpp
        src/Renderer/PlatformDetector.cpp
        src/Renderer/RendererFactory.cpp
    )
    
    set(RENDERER_HEADERS
        src/Renderer/GPURenderer.h
        src/Renderer/OpenGLRenderer.h
        src/Renderer/PlatformDetector.h
        src/Renderer/RendererFactory.h
    )
    
    # 平台特定源文件
    if(APPLE AND WITH_METAL)
        enable_language(OBJCXX)
        list(APPEND RENDERER_SOURCES src/Renderer/MetalRenderer.mm)
        list(APPEND RENDERER_HEADERS src/Renderer/MetalRenderer.h)
        
        find_library(METAL_FRAMEWORK Metal)
        find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)
    endif()
    
    if(WIN32 AND WITH_DIRECTX)
        list(APPEND RENDERER_SOURCES src/Renderer/DirectX12Renderer.cpp)
    endif()
    
    # 创建渲染器库
    add_library(WindTermRenderer STATIC ${RENDERER_SOURCES} ${RENDERER_HEADERS})
    
    target_include_directories(WindTermRenderer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermRenderer PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::OpenGL
    )
    
    # 链接平台框架
    if(APPLE AND WITH_METAL)
        target_link_libraries(WindTermRenderer PRIVATE
            ${METAL_FRAMEWORK}
            ${QUARTZ_CORE_FRAMEWORK}
        )
        target_compile_definitions(WindTermRenderer PRIVATE Q_OS_MACOS)
    endif()
    
    if(WIN32 AND WITH_DIRECTX)
        target_link_libraries(WindTermRenderer PRIVATE d3d12 dxgi)
        target_compile_definitions(WindTermRenderer PRIVATE Q_OS_WIN)
    endif()
    
    target_compile_definitions(WindTermRenderer PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_BUFFER)
    message(STATUS "Building Circular Text Buffer")
    
    add_library(WindTermBuffer STATIC
        src/Buffer/CircularTextBuffer.cpp
        src/Buffer/CircularTextBuffer.h
    )
    
    target_include_directories(WindTermBuffer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermBuffer PUBLIC Qt5::Core)
    
    target_compile_definitions(WindTermBuffer PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装规则
if(BUILD_RENDERER)
    install(TARGETS WindTermRenderer
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    install(DIRECTORY src/Renderer/
        DESTINATION include/WindTermExtensions/Renderer
        FILES_MATCHING PATTERN "*.h"
    )
endif()

if(BUILD_BUFFER)
    install(TARGETS WindTermBuffer
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    install(FILES src/Buffer/CircularTextBuffer.h
        DESTINATION include/WindTermExtensions/Buffer
    )
endif()

# 配置摘要
message(STATUS "")
message(STATUS "WindTerm Extensions Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Renderer: ${BUILD_RENDERER}")
message(STATUS "  Build Buffer: ${BUILD_BUFFER}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Metal Backend: ${WITH_METAL}")
message(STATUS "  OpenGL Backend: ${WITH_OPENGL}")
message(STATUS "  DirectX12 Backend: ${WITH_DIRECTX}")
message(STATUS "  Qt Version: ${Qt5Core_VERSION}")
message(STATUS "")
