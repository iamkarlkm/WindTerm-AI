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
option(BUILD_THEME "Build Theme Module" ON)
option(BUILD_WIDGET "Build Terminal Widget" ON)
option(BUILD_APP "Build Terminal Application" ON)
option(BUILD_TESTS "Build Unit Tests" ON)
option(BUILD_PLUGINS "Build Example Plugins" ON)
option(WITH_METAL "Enable Metal Backend (macOS)" ON)
option(WITH_OPENGL "Enable OpenGL Backend" ON)
option(WITH_DIRECTX "Enable DirectX12 Backend (Windows)" ON)
option(WITH_VULKAN "Enable Vulkan Backend" OFF)

# 查找依赖
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL Sql Network)

if(BUILD_RENDERER)
    message(STATUS "Building GPU Renderer Module")
    
    # 渲染器源文件
    set(RENDERER_SOURCES
        src/Renderer/GPURenderer.cpp
        src/Renderer/SoftwareRenderer.cpp
        src/Renderer/OpenGLRenderer.cpp
        src/Renderer/PlatformDetector.cpp
        src/Renderer/RendererFactory.cpp
        src/Renderer/SDFGlyphGenerator.cpp
        src/Renderer/DirectX12Renderer.cpp
        src/Renderer/VulkanRenderer.cpp
        src/Utils/UnicodeUtil.cpp
    )
    
    set(RENDERER_HEADERS
        src/Renderer/GPURenderer.h
        src/Renderer/SoftwareRenderer.h
        src/Renderer/OpenGLRenderer.h
        src/Renderer/PlatformDetector.h
        src/Renderer/RendererFactory.h
        src/Renderer/SDFGlyphGenerator.h
        src/Renderer/SDFShader.h
        src/Renderer/DirectX12Renderer.h
        src/Renderer/VulkanRenderer.h
        src/Renderer/d3dx12_simple.h
        src/Utils/UnicodeUtil.h
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

if(BUILD_RENDERER)
    message(STATUS "Building Terminal Module")
    
    add_library(WindTermTerminal STATIC
        src/Terminal/AnsiParser.cpp
        src/Terminal/AnsiParser.h
        src/Terminal/TerminalState.cpp
        src/Terminal/TerminalState.h
        src/Terminal/PtyManager.cpp
        src/Terminal/PtyManager.h
        src/Terminal/TerminalSession.cpp
        src/Terminal/TerminalSession.h
    )
    
    target_include_directories(WindTermTerminal PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermTerminal PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
    )
    
    target_compile_definitions(WindTermTerminal PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_BUFFER)
    message(STATUS "Building Memory Fragment Module")
    
    add_library(WindTermMemoryFragment STATIC
        src/MemoryFragment/MemoryFragment.h
        src/MemoryFragment/MemoryFragmentContext.cpp
        src/MemoryFragment/MemoryFragmentStore.cpp
        src/MemoryFragment/MemoryFragmentStore.h
    )
    
    target_include_directories(WindTermMemoryFragment PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermMemoryFragment PUBLIC
        Qt5::Core
        Qt5::Sql
    )
    
    target_compile_definitions(WindTermMemoryFragment PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_RENDERER)
    message(STATUS "Building SSH Module")
    
    add_library(WindTermSsh STATIC
        src/Ssh/SshConfig.h
        src/Ssh/SshChannelSession.cpp
        src/Ssh/SshChannelSession.h
        src/Ssh/ConnectionProfile.h
        src/Ssh/ConnectionManager.cpp
        src/Ssh/ConnectionManager.h
        src/Ssh/ScpClient.h
        src/Ssh/ScpClient.cpp
    )
    
    target_include_directories(WindTermSsh PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermSsh PUBLIC
        Qt5::Core
        Qt5::Sql
        ssh
    )
    
    target_compile_definitions(WindTermSsh PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_RENDERER)
    message(STATUS "Building Command History Module")
    
    add_library(WindTermCommandHistory STATIC
        src/CommandHistory/CommandHistoryStore.h
        src/CommandHistory/CommandHistoryStore.cpp
    )
    
    target_include_directories(WindTermCommandHistory PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermCommandHistory PUBLIC
        Qt5::Core
        Qt5::Sql
    )
    
    target_compile_definitions(WindTermCommandHistory PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_RENDERER)
    message(STATUS "Building Bookmarks Module")
    
    add_library(WindTermBookmarks STATIC
        src/Bookmarks/BookmarksStore.h
        src/Bookmarks/BookmarksStore.cpp
    )
    
    target_include_directories(WindTermBookmarks PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermBookmarks PUBLIC
        Qt5::Core
        Qt5::Sql
    )
    
    target_compile_definitions(WindTermBookmarks PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_RENDERER)
    message(STATUS "Building Settings Module")
    
    add_library(WindTermSettings STATIC
        src/Settings/SettingsManager.h
        src/Settings/SettingsManager.cpp
    )
    
    target_include_directories(WindTermSettings PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermSettings PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Sql
    )
    
    target_compile_definitions(WindTermSettings PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_THEME)
    message(STATUS "Building Theme Module")
    
    add_library(WindTermTheme STATIC
        src/Theme/ThemeConfig.h
        src/Theme/ThemeConfig.cpp
        src/Theme/ThemeManager.h
        src/Theme/ThemeManager.cpp
    )
    
    target_include_directories(WindTermTheme PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermTheme PUBLIC
        Qt5::Core
        Qt5::Gui
    )
    
    target_compile_definitions(WindTermTheme PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_RENDERER)
    message(STATUS "Building AI Integration Module")
    
    add_library(WindTermAiIntegration STATIC
        src/AiIntegration/AiConfig.h
        src/AiIntegration/AiClient.h
        src/AiIntegration/AiClient.cpp
    )
    
    target_include_directories(WindTermAiIntegration PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermAiIntegration PUBLIC
        Qt5::Core
        Qt5::Network
    )
    
    target_compile_definitions(WindTermAiIntegration PRIVATE
        QT_DEPRECATED_WARNINGS
    )
endif()

if(BUILD_RENDERER)
    message(STATUS "Building Plugin System Module")
    
    add_library(WindTermPluginSystem STATIC
        src/plugins/PluginInterface.h
        src/plugins/PluginContext.h
        src/plugins/PluginLoader.h
        src/plugins/PluginManager.h
    )
    
    target_include_directories(WindTermPluginSystem PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermPluginSystem PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
    )
    
    target_compile_definitions(WindTermPluginSystem PRIVATE
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
        src/Widget/TerminalPane.cpp
        src/Widget/TerminalPane.h
        src/Widget/SplitterContainer.cpp
        src/Widget/SplitterContainer.h
        src/Widget/MemoryViewerDialog.cpp
        src/Widget/MemoryViewerDialog.h
        src/Widget/MemoryEditorDialog.cpp
        src/Widget/MemoryEditorDialog.h
        src/Widget/ConnectionDialog.cpp
        src/Widget/ConnectionDialog.h
        src/Widget/ThemeDialog.cpp
        src/Widget/ThemeDialog.h
        src/Widget/CommandSearchDialog.cpp
        src/Widget/CommandSearchDialog.h
        src/Widget/BookmarksDialog.cpp
        src/Widget/BookmarksDialog.h
        src/Widget/ImportExportDialog.cpp
        src/Widget/ImportExportDialog.h
        src/Widget/TabWidget.cpp
        src/Widget/TabWidget.h
        src/Widget/PluginManagerDialog.cpp
        src/Widget/PluginManagerDialog.h
        src/Widget/AiAssistantDialog.cpp
        src/Widget/AiAssistantDialog.h
        src/Widget/FileTransferDialog.cpp
        src/Widget/FileTransferDialog.h
        src/Widget/RecordingDialog.cpp
        src/Widget/RecordingDialog.h
        src/Widget/TerminalSearchDialog.cpp
        src/Widget/TerminalSearchDialog.h
        src/Widget/UrlDetector.cpp
        src/Widget/UrlDetector.h
        src/Widget/TerminalRecording.cpp
        src/Widget/TerminalRecording.h
    )
    
    target_include_directories(WindTermWidget PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(WindTermWidget PUBLIC
        WindTermRenderer
        WindTermBuffer
        WindTermTerminal
        WindTermMemoryFragment
        WindTermSsh
        WindTermTheme
        WindTermCommandHistory
        WindTermBookmarks
        WindTermSettings
        WindTermPluginSystem
        WindTermAiIntegration
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
        Qt5::Sql
        Qt5::Network
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

if(BUILD_PLUGINS)
    message(STATUS "Building Example Plugins")
    
    add_subdirectory(src/plugins/TerminalInfoPlugin)
    add_subdirectory(src/plugins/ThemeSwitcherPlugin)
    
    install(DIRECTORY src/plugins/
        DESTINATION plugins
        FILES_MATCHING PATTERN "*.json"
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
message(STATUS "  Build Plugins: ${BUILD_PLUGINS}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Metal Backend: ${WITH_METAL}")
message(STATUS "  OpenGL Backend: ${WITH_OPENGL}")
message(STATUS "  DirectX12 Backend: ${WITH_DIRECTX}")
message(STATUS "  Vulkan Backend: ${WITH_VULKAN}")
message(STATUS "  Qt Version: ${Qt5Core_VERSION}")
message(STATUS "")
