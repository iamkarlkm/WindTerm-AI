#ifndef ANSI_ESCAPE_CODES_H
#define ANSI_ESCAPE_CODES_H

#include <cstdint>

namespace AnsiCodes {

// C0 Control codes
constexpr char BEL = 0x07;
constexpr char BS  = 0x08;
constexpr char TAB = 0x09;
constexpr char LF  = 0x0A;
constexpr char CR  = 0x0D;

// Escape sequences
constexpr const char* CSI_SEQ = "\x1B[";
constexpr const char* OSC_SEQ  = "\x1B]";
constexpr const char* DCS_SEQ  = "\x1BP";
constexpr const char* APC_SEQ  = "\x1B_";
constexpr const char* PM_SEQ   = "\x1B^";

// SGR (Select Graphic Rendition) codes
namespace SGR {
    constexpr int RESET = 0;
    constexpr int BOLD = 1;
    constexpr int DIM = 2;
    constexpr int ITALIC = 3;
    constexpr int UNDERLINE = 4;
    constexpr int BLINK_SLOW = 5;
    constexpr int BLINK_FAST = 6;
    constexpr int REVERSE = 7;
    constexpr int HIDDEN = 8;
    constexpr int STRIKETHROUGH = 9;
    
    constexpr int FOREGROUND_BLACK = 30;
    constexpr int FOREGROUND_RED = 31;
    constexpr int FOREGROUND_GREEN = 32;
    constexpr int FOREGROUND_YELLOW = 33;
    constexpr int FOREGROUND_BLUE = 34;
    constexpr int FOREGROUND_MAGENTA = 35;
    constexpr int FOREGROUND_CYAN = 36;
    constexpr int FOREGROUND_WHITE = 37;
    constexpr int FOREGROUND_DEFAULT = 39;
    
    constexpr int BACKGROUND_BLACK = 40;
    constexpr int BACKGROUND_RED = 41;
    constexpr int BACKGROUND_GREEN = 42;
    constexpr int BACKGROUND_YELLOW = 43;
    constexpr int BACKGROUND_BLUE = 44;
    constexpr int BACKGROUND_MAGENTA = 45;
    constexpr int BACKGROUND_CYAN = 46;
    constexpr int BACKGROUND_WHITE = 47;
    constexpr int BACKGROUND_DEFAULT = 49;
    
    // 256 color mode
    constexpr int FOREGROUND_256 = 38;
    constexpr int BACKGROUND_256 = 48;
    constexpr int COLOR_256_SUBCODE = 5;
    
    // True color (24-bit)
    constexpr int FOREGROUND_TRUECOLOR = 38;
    constexpr int BACKGROUND_TRUECOLOR = 48;
    constexpr int COLOR_TRUECOLOR_SUBCODE = 2;
}

// Cursor movement
namespace Cursor {
    constexpr const char* UP = "A";
    constexpr const char* DOWN = "B";
    constexpr const char* RIGHT = "C";
    constexpr const char* LEFT = "D";
    constexpr const char* NEXT_LINE = "E";
    constexpr const char* PREV_LINE = "F";
    constexpr const char* COLUMN = "G";
    constexpr const char* POSITION = "H";
    constexpr const char* SAVE = "s";
    constexpr const char* RESTORE = "u";
    constexpr const char* HIDE = "?25l";
    constexpr const char* SHOW = "?25h";
}

// Erase commands
namespace Erase {
    constexpr const char* RIGHT = "K";
    constexpr const char* LEFT = "1K";
    constexpr const char* LINE = "2K";
    constexpr const char* DOWN = "J";
    constexpr const char* UP = "1J";
    constexpr const char* SCREEN = "2J";
    constexpr const char* SAVED_LINES = "3J";
}

// Scroll commands
namespace Scroll {
    constexpr const char* UP = "S";
    constexpr const char* DOWN = "T";
}

// DEC private modes
namespace DEC {
    constexpr const char* RESET = "c";
    constexpr const char* SAVE_CURSOR = "?1049h";
    constexpr const char* RESTORE_CURSOR = "?1049l";
    constexpr const char* ALT_SCREEN = "?1049";
    constexpr const char* ORIGIN_MODE = "?6";
    constexpr const char* AUTO_WRAP = "?7";
    constexpr const char* REVERSE_VIDEO = "?5";
    constexpr const char* MOUSE_TRACKING = "?1000";
    constexpr const char* MOUSE_EXTENDED = "?1006";
    constexpr const char* BRACKETED_PASTE = "?2004";
}

// OSC (Operating System Command)
namespace OSC {
    constexpr const char* SET_WINDOW_TITLE = "0;";
    constexpr const char* SET_ICON_NAME = "1;";
    constexpr const char* SET_TITLE = "2;";
    constexpr const char* SET_COLOR = "4;";
    constexpr const char* RESET_COLOR = "104;";
    constexpr const char* HYPERLINK = "8;";
    constexpr const char* NOTIFY = "777;notify";
}

// DCS (Device Control String)
namespace DCS {
    constexpr const char* TERMINAL_CAPABILITIES = "+q";
}

}

#endif
