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
option(BUILD_WIDGET "Build Terminal Widget" ON)
option(BUILD_APP "Build Terminal Application" ON)
option(BUILD_TESTS "Build Unit Tests" OFF)
option(WITH_METAL "Enable Metal Backend (macOS)" ON)
option(WITH_OPENGL "Enable OpenGL Backend" ON)
option(WITH_DIRECTX "Enable DirectX12 Backend (Windows)" ON)
option(WITH_VULKAN "Enable Vulkan Backend" OFF)

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
        src/Renderer/SDFGlyphGenerator.cpp
        src/Renderer/DirectX12Renderer.cpp
        src/Renderer/VulkanRenderer.cpp
    )
    
    set(RENDERER_HEADERS
        src/Renderer/GPURenderer.h
        src/Renderer/OpenGLRenderer.h
        src/Renderer/PlatformDetector.h
        src/Renderer/RendererFactory.h
        src/Renderer/SDFGlyphGenerator.h
        src/Renderer/SDFShader.h
        src/Renderer/DirectX12Renderer.h
        src/Renderer/VulkanRenderer.h
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

if(BUILD_WIDGET)
    message(STATUS "Building Terminal Widget")
    
    add_library(WindTermWidget STATIC
        src/Widget/TerminalWidget.cpp
        src/Widget/TerminalWidget.h
        src/Widget/TerminalMainWindow.cpp
        src/Widget/TerminalMainWindow.h
    )
    
    target_include_directories(WindTermWidget PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermWidget PUBLIC
        WindTermRenderer
        WindTermBuffer
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
    )
    
    target_compile_definitions(WindTermWidget PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_APP)
    message(STATUS "Building Terminal Application")
    
    add_executable(windterm-terminal
        src/Widget/main.cpp
    )
    
    target_link_libraries(windterm-terminal PRIVATE
        WindTermWidget
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
    )
    
    if(APPLE AND WITH_METAL)
        find_library(METAL_FRAMEWORK Metal)
        find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)
        target_link_libraries(windterm-terminal PRIVATE
            ${METAL_FRAMEWORK}
            ${QUARTZ_CORE_FRAMEWORK}
        )
    endif()
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

if(BUILD_WIDGET)
    install(TARGETS WindTermWidget
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    install(DIRECTORY src/Widget/
        DESTINATION include/WindTermExtensions/Widget
        FILES_MATCHING PATTERN "*.h"
    )
endif()

if(BUILD_APP)
    install(TARGETS windterm-terminal
        RUNTIME DESTINATION bin
    )
endif()

# 配置摘要
message(STATUS "")
message(STATUS "WindTerm Extensions Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Renderer: ${BUILD_RENDERER}")
message(STATUS "  Build Buffer: ${BUILD_BUFFER}")
message(STATUS "  Build Widget: ${BUILD_WIDGET}")
message(STATUS "  Build Application: ${BUILD_APP}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Metal Backend: ${WITH_METAL}")
message(STATUS "  OpenGL Backend: ${WITH_OPENGL}")
message(STATUS "  DirectX12 Backend: ${WITH_DIRECTX}")
message(STATUS "  Vulkan Backend: ${WITH_VULKAN}")
message(STATUS "  Qt Version: ${Qt5Core_VERSION}")
message(STATUS "")
