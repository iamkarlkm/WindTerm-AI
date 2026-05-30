#include "TerminalTheme.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>

TerminalTheme TerminalTheme::loadPreset(const QString& presetName) {
    TerminalTheme theme;
    
    if (presetName == "dark") {
        theme.name = "Dark";
        theme.description = "Classic dark theme";
    } else if (presetName == "light") {
        theme.name = "Light";
        theme.description = "Classic light theme";
        theme.colors.foreground = QColor(0, 0, 0);
        theme.colors.background = QColor(255, 255, 255);
        theme.colors.cursor = QColor(0, 0, 0);
    } else if (presetName == "monokai") {
        theme.name = "Monokai";
        theme.description = "Popular dark theme";
        theme.colors.foreground = QColor(248, 248, 242);
        theme.colors.background = QColor(39, 40, 34);
        theme.colors.cursor = QColor(248, 248, 242);
        theme.colors.ansiColors[0] = QColor(39, 40, 34);
        theme.colors.ansiColors[1] = QColor(249, 38, 114);
        theme.colors.ansiColors[2] = QColor(166, 226, 46);
        theme.colors.ansiColors[3] = QColor(230, 219, 116);
        theme.colors.ansiColors[4] = QColor(102, 217, 239);
        theme.colors.ansiColors[5] = QColor(174, 129, 255);
        theme.colors.ansiColors[6] = QColor(66, 194, 191);
    } else if (presetName == "solarized-dark") {
        theme.name = "Solarized Dark";
        theme.description = "Solarized dark variant";
        theme.colors.foreground = QColor(131, 148, 150);
        theme.colors.background = QColor(0, 43, 54);
        theme.colors.cursor = QColor(131, 148, 150);
    } else if (presetName == "github") {
        theme.name = "GitHub";
        theme.description = "GitHub dark theme";
        theme.colors.foreground = QColor(201, 209, 217);
        theme.colors.background = QColor(13, 17, 23);
        theme.colors.cursor = QColor(255, 255, 255);
    }
    
    return theme;
}

QVector<QString> TerminalTheme::availablePresets() {
    return {"dark", "light", "monokai", "solarized-dark", "github", "gruvbox", "one-dark", "nord"};
}

void TerminalTheme::saveToFile(const QString& filePath) const {
    QJsonObject json;
    json["name"] = name;
    json["description"] = description;
    json["foreground"] = colors.foreground.name();
    json["background"] = colors.background.name();
    json["cursor"] = colors.cursor.name();
    json["selection"] = colors.selection.name();
    json["opacity"] = opacity;
    json["smoothScroll"] = smoothScroll;
    json["cursorBlinkInterval"] = cursorBlinkInterval;
    
    QJsonArray ansiArray;
    for (const QColor& c : colors.ansiColors) {
        ansiArray.append(c.name());
    }
    json["ansiColors"] = ansiArray;
    
    QJsonObject fontObj;
    fontObj["family"] = font.family;
    fontObj["size"] = font.size;
    fontObj["bold"] = font.bold;
    fontObj["italic"] = font.italic;
    json["font"] = fontObj;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
    }
}

TerminalTheme TerminalTheme::loadFromFile(const QString& filePath) {
    TerminalTheme theme;
    
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject json = doc.object();
        
        theme.name = json["name"].toString();
        theme.description = json["description"].toString();
        theme.colors.foreground = QColor(json["foreground"].toString());
        theme.colors.background = QColor(json["background"].toString());
        theme.colors.cursor = QColor(json["cursor"].toString());
        theme.colors.selection = QColor(json["selection"].toString());
        theme.opacity = json["opacity"].toInt(100);
        theme.smoothScroll = json["smoothScroll"].toBool(true);
        theme.cursorBlinkInterval = json["cursorBlinkInterval"].toInt(530);
        
        QJsonArray ansiArray = json["ansiColors"].toArray();
        for (int i = 0; i < qMin(ansiArray.size(), 16); i++) {
            theme.colors.ansiColors[i] = QColor(ansiArray[i].toString());
        }
        
        QJsonObject fontObj = json["font"].toObject();
        theme.font.family = fontObj["family"].toString("monospace");
        theme.font.size = fontObj["size"].toInt(14);
        theme.font.bold = fontObj["bold"].toBool(false);
        theme.font.italic = fontObj["italic"].toBool(false);
    }
    
    return theme;
}

#include "TerminalTheme.moc"
