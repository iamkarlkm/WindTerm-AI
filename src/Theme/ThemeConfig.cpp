#include "ThemeConfig.h"

ThemeConfig::ThemeConfig()
    : foreground(200, 200, 200)
    , background(30, 30, 30)
    , cursor(200, 200, 200)
    , selection(100, 100, 150, 128)
    , cursorText(30, 30, 30)
    , black(0, 0, 0)
    , red(204, 0, 0)
    , green(0, 204, 0)
    , yellow(204, 204, 0)
    , blue(0, 0, 204)
    , magenta(204, 0, 204)
    , cyan(0, 204, 204)
    , white(200, 200, 200)
    , brightBlack(128, 128, 128)
    , brightRed(255, 0, 0)
    , brightGreen(0, 255, 0)
    , brightYellow(255, 255, 0)
    , brightBlue(0, 0, 255)
    , brightMagenta(255, 0, 255)
    , brightCyan(0, 255, 255)
    , brightWhite(255, 255, 255)
    , fontFamily("Monaco")
    , fontSize(14)
    , backgroundOpacity(0.0)
{
}

QColor ThemeConfig::ansiColor(int index) const {
    static const QColor ThemeConfig::*colors[] = {
        &ThemeConfig::black, &ThemeConfig::red, &ThemeConfig::green, &ThemeConfig::yellow,
        &ThemeConfig::blue, &ThemeConfig::magenta, &ThemeConfig::cyan, &ThemeConfig::white,
        &ThemeConfig::brightBlack, &ThemeConfig::brightRed, &ThemeConfig::brightGreen, &ThemeConfig::brightYellow,
        &ThemeConfig::brightBlue, &ThemeConfig::brightMagenta, &ThemeConfig::brightCyan, &ThemeConfig::brightWhite
    };
    
    if (index >= 0 && index < 16) {
        return this->*colors[index];
    }
    return foreground;
}

QJsonObject ThemeConfig::toJson() const {
    QJsonObject json;
    json["name"] = name;
    json["foreground"] = foreground.name(QColor::HexArgb);
    json["background"] = background.name(QColor::HexArgb);
    json["cursor"] = cursor.name(QColor::HexArgb);
    json["selection"] = selection.name(QColor::HexArgb);
    json["cursorText"] = cursorText.name(QColor::HexArgb);
    json["black"] = black.name();
    json["red"] = red.name();
    json["green"] = green.name();
    json["yellow"] = yellow.name();
    json["blue"] = blue.name();
    json["magenta"] = magenta.name();
    json["cyan"] = cyan.name();
    json["white"] = white.name();
    json["brightBlack"] = brightBlack.name();
    json["brightRed"] = brightRed.name();
    json["brightGreen"] = brightGreen.name();
    json["brightYellow"] = brightYellow.name();
    json["brightBlue"] = brightBlue.name();
    json["brightMagenta"] = brightMagenta.name();
    json["brightCyan"] = brightCyan.name();
    json["brightWhite"] = brightWhite.name();
    json["fontFamily"] = fontFamily;
    json["fontSize"] = fontSize;
    json["backgroundImage"] = backgroundImage;
    json["backgroundOpacity"] = backgroundOpacity;
    return json;
}

ThemeConfig ThemeConfig::fromJson(const QJsonObject& json) {
    ThemeConfig config;
    config.name = json["name"].toString();
    config.foreground = QColor(json["foreground"].toString());
    config.background = QColor(json["background"].toString());
    config.cursor = QColor(json["cursor"].toString());
    config.selection = QColor(json["selection"].toString());
    config.cursorText = QColor(json["cursorText"].toString());
    config.black = QColor(json["black"].toString());
    config.red = QColor(json["red"].toString());
    config.green = QColor(json["green"].toString());
    config.yellow = QColor(json["yellow"].toString());
    config.blue = QColor(json["blue"].toString());
    config.magenta = QColor(json["magenta"].toString());
    config.cyan = QColor(json["cyan"].toString());
    config.white = QColor(json["white"].toString());
    config.brightBlack = QColor(json["brightBlack"].toString());
    config.brightRed = QColor(json["brightRed"].toString());
    config.brightGreen = QColor(json["brightGreen"].toString());
    config.brightYellow = QColor(json["brightYellow"].toString());
    config.brightBlue = QColor(json["brightBlue"].toString());
    config.brightMagenta = QColor(json["brightMagenta"].toString());
    config.brightCyan = QColor(json["brightCyan"].toString());
    config.brightWhite = QColor(json["brightWhite"].toString());
    config.fontFamily = json["fontFamily"].toString("Monaco");
    config.fontSize = json["fontSize"].toInt(14);
    config.backgroundImage = json["backgroundImage"].toString();
    config.backgroundOpacity = json["backgroundOpacity"].toDouble(0.0);
    return config;
}

ThemeConfig ThemeConfig::defaultTheme() {
    ThemeConfig theme;
    theme.name = "Default";
    return theme;
}

ThemeConfig ThemeConfig::draculaTheme() {
    ThemeConfig theme;
    theme.name = "Dracula";
    theme.foreground = QColor(248, 248, 242);
    theme.background = QColor(40, 42, 54);
    theme.cursor = QColor(248, 248, 242);
    theme.selection = QColor(68, 71, 90, 128);
    theme.cursorText = QColor(40, 42, 54);
    theme.black = QColor(0, 0, 0);
    theme.red = QColor(255, 85, 85);
    theme.green = QColor(80, 250, 123);
    theme.yellow = QColor(241, 250, 140);
    theme.blue = QColor(98, 114, 164);
    theme.magenta = QColor(255, 121, 198);
    theme.cyan = QColor(139, 233, 253);
    theme.white = QColor(186, 194, 222);
    theme.brightBlack = QColor(85, 85, 85);
    theme.brightRed = QColor(255, 85, 85);
    theme.brightGreen = QColor(80, 250, 123);
    theme.brightYellow = QColor(241, 250, 140);
    theme.brightBlue = QColor(98, 114, 164);
    theme.brightMagenta = QColor(255, 121, 198);
    theme.brightCyan = QColor(139, 233, 253);
    theme.brightWhite = QColor(255, 255, 255);
    theme.fontFamily = "JetBrains Mono";
    return theme;
}

ThemeConfig ThemeConfig::monokaiTheme() {
    ThemeConfig theme;
    theme.name = "Monokai";
    theme.foreground = QColor(248, 248, 242);
    theme.background = QColor(39, 40, 34);
    theme.cursor = QColor(248, 248, 242);
    theme.selection = QColor(73, 72, 62, 128);
    theme.cursorText = QColor(39, 40, 34);
    theme.black = QColor(39, 40, 34);
    theme.red = QColor(249, 38, 114);
    theme.green = QColor(166, 226, 46);
    theme.yellow = QColor(246, 209, 60);
    theme.blue = QColor(102, 217, 239);
    theme.magenta = QColor(174, 129, 255);
    theme.cyan = QColor(102, 217, 239);
    theme.white = QColor(248, 248, 242);
    theme.brightBlack = QColor(88, 88, 91);
    theme.brightRed = QColor(249, 38, 114);
    theme.brightGreen = QColor(166, 226, 46);
    theme.brightYellow = QColor(246, 209, 60);
    theme.brightBlue = QColor(102, 217, 239);
    theme.brightMagenta = QColor(174, 129, 255);
    theme.brightCyan = QColor(102, 217, 239);
    theme.brightWhite = QColor(255, 255, 255);
    return theme;
}

ThemeConfig ThemeConfig::solarizedDarkTheme() {
    ThemeConfig theme;
    theme.name = "Solarized Dark";
    theme.foreground = QColor(131, 148, 150);
    theme.background = QColor(0, 43, 54);
    theme.cursor = QColor(131, 148, 150);
    theme.selection = QColor(0, 43, 54, 128);
    theme.cursorText = QColor(0, 43, 54);
    theme.black = QColor(7, 54, 66);
    theme.red = QColor(220, 50, 47);
    theme.green = QColor(133, 153, 0);
    theme.yellow = QColor(181, 137, 0);
    theme.blue = QColor(38, 139, 210);
    theme.magenta = QColor(211, 54, 130);
    theme.cyan = QColor(42, 161, 152);
    theme.white = QColor(238, 232, 213);
    theme.brightBlack = QColor(0, 43, 54);
    theme.brightRed = QColor(203, 75, 22);
    theme.brightGreen = QColor(88, 110, 117);
    theme.brightYellow = QColor(101, 123, 131);
    theme.brightBlue = QColor(131, 148, 150);
    theme.brightMagenta = QColor(108, 113, 196);
    theme.brightCyan = QColor(147, 161, 161);
    theme.brightWhite = QColor(253, 246, 227);
    return theme;
}
