# Unit Tests for WindTerm-Extensions

find_package(Qt5 COMPONENTS Test Core Widgets REQUIRED)

# Existing tests (in tests/ root)
add_executable(circular_buffer_test
    CircularTextBufferTest.cpp
)
target_link_libraries(circular_buffer_test PRIVATE
    Qt5::Test
    Qt5::Core
    WindTermBuffer
)

add_executable(ansi_parser_test
    AnsiParserTest.cpp
)
target_link_libraries(ansi_parser_test PRIVATE
    Qt5::Test
    Qt5::Core
    WindTermTerminal
)

add_executable(terminal_state_test
    TerminalStateTest.cpp
)
target_link_libraries(terminal_state_test PRIVATE
    Qt5::Test
    Qt5::Core
    WindTermTerminal
)

add_executable(url_detector_test
    UrlDetectorTest.cpp
)
target_link_libraries(url_detector_test PRIVATE
    Qt5::Test
    Qt5::Core
    WindTermWidget
)

# New tests (in tests/src/)
add_executable(credential_manager_test
    src/credential_manager_test.cpp
)
target_link_libraries(credential_manager_test PRIVATE
    Qt5::Test
    Qt5::Core
)
target_include_directories(credential_manager_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(script_engine_test
    src/script_engine_test.cpp
)
target_link_libraries(script_engine_test PRIVATE
    Qt5::Test
    Qt5::Core
)
target_include_directories(script_engine_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(session_history_test
    src/session_history_test.cpp
)
target_link_libraries(session_history_test PRIVATE
    Qt5::Test
    Qt5::Core
)
target_include_directories(session_history_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Register tests
enable_testing()
add_test(NAME CircularBufferTest COMMAND circular_buffer_test)
add_test(NAME AnsiParserTest COMMAND ansi_parser_test)
add_test(NAME TerminalStateTest COMMAND terminal_state_test)
add_test(NAME UrlDetectorTest COMMAND url_detector_test)
add_test(NAME CredentialManagerTest COMMAND credential_manager_test)
add_test(NAME ScriptEngineTest COMMAND script_engine_test)
add_test(NAME SessionHistoryTest COMMAND session_history_test)
