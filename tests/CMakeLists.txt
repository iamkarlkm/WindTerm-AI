# Unit Tests
find_package(Qt5 COMPONENTS Test REQUIRED)

# CircularTextBuffer Test
add_executable(circular_buffer_test CircularTextBufferTest.cpp)
target_link_libraries(circular_buffer_test PRIVATE
    WindTermBuffer
    Qt5::Core
    Qt5::Widgets
    Qt5::Test
)
add_test(NAME CircularTextBufferTest COMMAND circular_buffer_test)

# AnsiParser Test
add_executable(ansi_parser_test AnsiParserTest.cpp)
target_link_libraries(ansi_parser_test PRIVATE
    WindTermTerminal
    Qt5::Core
    Qt5::Widgets
    Qt5::Test
)
add_test(NAME AnsiParserTest COMMAND ansi_parser_test)

# TerminalState Test
add_executable(terminal_state_test TerminalStateTest.cpp)
target_link_libraries(terminal_state_test PRIVATE
    WindTermTerminal
    Qt5::Core
    Qt5::Widgets
    Qt5::Test
)
add_test(NAME TerminalStateTest COMMAND terminal_state_test)

# UrlDetector Test
add_executable(url_detector_test UrlDetectorTest.cpp)
target_link_libraries(url_detector_test PRIVATE
    WindTermWidget
    Qt5::Core
    Qt5::Widgets
    Qt5::Test
)
add_test(NAME UrlDetectorTest COMMAND url_detector_test)
