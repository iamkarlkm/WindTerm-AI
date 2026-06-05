#include "ColorSchemeEditor.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QDir>
#include <QDebug>

ColorSchemeManager* ColorSchemeManager::s_instance = nullptr;

ColorSchemeManager::ColorSchemeManager(QObject* parent)
    : QObject(parent) {
    
    m_schemesFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/color_schemes.json";
    
    // 加载内置配色
    loadBuiltinSchemes();
    
    // 加载用户配色
    QFile file(m_schemesFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonArray schemesJson = QJsonDocument::fromJson(file.readAll()).array();
        
        for (const QJsonValue& value : schemesJson) {
            QJsonObject json = value.toObject();
            
            ColorScheme scheme;
            scheme.id = json["id"].toString();
            scheme.name = json["name"].toString();
            scheme.description = json["description"].toString();
            scheme.author = json["author"].toString();
            scheme.isBuiltin = false;
            scheme.createdAt = json["createdAt"].toVariant().toLongLong();
            scheme.modifiedAt = json["modifiedAt"].toVariant().toLongLong();
            
            // 解析颜色
            scheme.foreground = QColor(json["foreground"].toString());
            scheme.background = QColor(json["background"].toString());
            scheme.cursor = QColor(json["cursor"].toString());
            scheme.cursorText = QColor(json["cursorText"].toString());
            scheme.selection = QColor(json["selection"].toString());
            scheme.selectionText = QColor(json["selectionText"].toString());
            
            QJsonArray colorsJson = json["colors"].toArray();
            for (int i = 0; i < 8 && i < colorsJson.size(); ++i) {
                scheme.colors[i] = QColor(colorsJson[i].toString());
            }
            
            QJsonArray brightJson = json["brightColors"].toArray();
            for (int i = 0; i < 8 && i < brightJson.size(); ++i) {
                scheme.brightColors[i] = QColor(brightJson[i].toString());
            }
            
            m_schemes[scheme.id] = scheme;
        }
        
        qDebug() << "[ColorScheme] Loaded" << m_schemes.size() << "schemes";
    }
}

ColorSchemeManager* ColorSchemeManager::instance() {
    if (!s_instance) {
        s_instance = new ColorSchemeManager();
    }
    return s_instance;
}

void ColorSchemeManager::loadBuiltinSchemes() {
    // Monokai
    ColorScheme monokai = createMonokaiScheme();
    monokai.id = "builtin-monokai";
    monokai.isBuiltin = true;
    m_schemes[monokai.id] = monokai;
    
    // Solarized Dark
    ColorScheme solarizedDark = createSolarizedDarkScheme();
    solarizedDark.id = "builtin-solarized-dark";
    solarizedDark.isBuiltin = true;
    m_schemes[solarizedDark.id] = solarizedDark;
    
    // Solarized Light
    ColorScheme solarizedLight = createSolarizedLightScheme();
    solarizedLight.id = "builtin-solarized-light";
    solarizedLight.isBuiltin = true;
    m_schemes[solarizedLight.id] = solarizedLight;
    
    // GitHub Dark
    ColorScheme githubDark = createGithubDarkScheme();
    githubDark.id = "builtin-github-dark";
    githubDark.isBuiltin = true;
    m_schemes[githubDark.id] = githubDark;
    
    // Dracula
    ColorScheme dracula = createDraculaScheme();
    dracula.id = "builtin-dracula";
    dracula.isBuiltin = true;
    m_schemes[dracula.id] = dracula;
    
    // Nord
    ColorScheme nord = createNordScheme();
    nord.id = "builtin-nord";
    nord.isBuiltin = true;
    m_schemes[nord.id] = nord;
    
    // Gruvbox Dark
    ColorScheme gruvbox = createGruvboxDarkScheme();
    gruvbox.id = "builtin-gruvbox-dark";
    gruvbox.isBuiltin = true;
    m_schemes[gruvbox.id] = gruvbox;
    
    // One Dark
    ColorScheme oneDark = createOneDarkScheme();
    oneDark.id = "builtin-one-dark";
    oneDark.isBuiltin = true;
    m_schemes[oneDark.id] = oneDark;
}

QString ColorSchemeManager::createScheme(const ColorScheme& scheme) {
    ColorScheme newScheme = scheme;
    newScheme.id = generateId();
    newScheme.isBuiltin = false;
    newScheme.createdAt = QDateTime::currentMSecsSinceEpoch();
    newScheme.modifiedAt = newScheme.createdAt;
    
    m_schemes[newScheme.id] = newScheme;
    
    emit schemeAdded(newScheme.id);
    
    return newScheme.id;
}

void ColorSchemeManager::deleteScheme(const QString& id) {
    if (!m_schemes.contains(id)) return;
    if (m_schemes[id].isBuiltin) return;  // 不能删除内置配色
    
    m_schemes.remove(id);
    
    emit schemeRemoved(id);
}

void ColorSchemeManager::updateScheme(const QString& id, const ColorScheme& scheme) {
    if (!m_schemes.contains(id)) return;
    
    ColorScheme& existing = m_schemes[id];
    existing.name = scheme.name;
    existing.description = scheme.description;
    existing.author = scheme.author;
    existing.foreground = scheme.foreground;
    existing.background = scheme.background;
    existing.cursor = scheme.cursor;
    existing.cursorText = scheme.cursorText;
    existing.selection = scheme.selection;
    existing.selectionText = scheme.selectionText;
    
    for (int i = 0; i < 8; ++i) {
        existing.colors[i] = scheme.colors[i];
        existing.brightColors[i] = scheme.brightColors[i];
    }
    
    existing.modifiedAt = QDateTime::currentMSecsSinceEpoch();
    
    emit schemeUpdated(id);
}

ColorScheme ColorSchemeManager::getScheme(const QString& id) const {
    return m_schemes.value(id);
}

QList<ColorScheme> ColorSchemeManager::getAllSchemes() const {
    return m_schemes.values();
}

QList<ColorScheme> ColorSchemeManager::getBuiltinSchemes() const {
    QList<ColorScheme> builtin;
    for (auto it = m_schemes.begin(); it != m_schemes.end(); ++it) {
        if (it->isBuiltin) {
            builtin.append(it.value());
        }
    }
    return builtin;
}

QList<ColorScheme> ColorSchemeManager::getUserSchemes() const {
    QList<ColorScheme> user;
    for (auto it = m_schemes.begin(); it != m_schemes.end(); ++it) {
        if (!it->isBuiltin) {
            user.append(it.value());
        }
    }
    return user;
}

void ColorSchemeManager::exportScheme(const QString& id, const QString& filePath) {
    if (!m_schemes.contains(id)) return;
    
    const ColorScheme& scheme = m_schemes[id];
    
    QJsonObject json;
    json["id"] = scheme.id;
    json["name"] = scheme.name;
    json["description"] = scheme.description;
    json["author"] = scheme.author;
    json["foreground"] = scheme.foreground.name();
    json["background"] = scheme.background.name();
    json["cursor"] = scheme.cursor.name();
    json["cursorText"] = scheme.cursorText.name();
    json["selection"] = scheme.selection.name();
    json["selectionText"] = scheme.selectionText.name();
    
    QJsonArray colorsJson;
    for (int i = 0; i < 8; ++i) {
        colorsJson.append(scheme.colors[i].name());
    }
    json["colors"] = colorsJson;
    
    QJsonArray brightJson;
    for (int i = 0; i < 8; ++i) {
        brightJson.append(scheme.brightColors[i].name());
    }
    json["brightColors"] = brightJson;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
}

void ColorSchemeManager::importScheme(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    importSchemeFromText(doc.toJson());
}

QString ColorSchemeManager::importSchemeFromText(const QString& jsonText) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
    QJsonObject json = doc.object();
    
    ColorScheme scheme;
    scheme.name = json["name"].toString("Imported Scheme");
    scheme.description = json["description"].toString();
    scheme.author = json["author"].toString();
    scheme.foreground = QColor(json["foreground"].toString("#ffffff"));
    scheme.background = QColor(json["background"].toString("#000000"));
    scheme.cursor = QColor(json["cursor"].toString("#ffffff"));
    scheme.cursorText = QColor(json["cursorText"].toString("#000000"));
    scheme.selection = QColor(json["selection"].toString("#444444"));
    scheme.selectionText = QColor(json["selectionText"].toString("#ffffff"));
    
    QJsonArray colorsJson = json["colors"].toArray();
    for (int i = 0; i < 8 && i < colorsJson.size(); ++i) {
        scheme.colors[i] = QColor(colorsJson[i].toString());
    }
    
    QJsonArray brightJson = json["brightColors"].toArray();
    for (int i = 0; i < 8 && i < brightJson.size(); ++i) {
        scheme.brightColors[i] = QColor(brightJson[i].toString());
    }
    
    return createScheme(scheme);
}

ColorScheme ColorSchemeManager::createMonokaiScheme() {
    ColorScheme scheme;
    scheme.name = "Monokai";
    scheme.description = "Classic Monokai dark theme";
    scheme.author = "Wimer Hazenberg";
    
    scheme.foreground = QColor("#f8f8f2");
    scheme.background = QColor("#272822");
    scheme.cursor = QColor("#f8f8f0");
    scheme.cursorText = QColor("#272822");
    scheme.selection = QColor("#49483e");
    
    scheme.colors[0] = QColor("#272822");
    scheme.colors[1] = QColor("#f92672");
    scheme.colors[2] = QColor("#a6e22e");
    scheme.colors[3] = QColor("#f4bf75");
    scheme.colors[4] = QColor("#66d9ef");
    scheme.colors[5] = QColor("#ae81ff");
    scheme.colors[6] = QColor("#a1efe4");
    scheme.colors[7] = QColor("#f8f8f2");
    
    scheme.brightColors[0] = QColor("#75715e");
    scheme.brightColors[1] = QColor("#f92672");
    scheme.brightColors[2] = QColor("#a6e22e");
    scheme.brightColors[3] = QColor("#f4bf75");
    scheme.brightColors[4] = QColor("#66d9ef");
    scheme.brightColors[5] = QColor("#ae81ff");
    scheme.brightColors[6] = QColor("#a1efe4");
    scheme.brightColors[7] = QColor("#f8f8f2");
    
    return scheme;
}

ColorScheme ColorSchemeManager::createSolarizedDarkScheme() {
    ColorScheme scheme;
    scheme.name = "Solarized Dark";
    scheme.description = "Precision colors for machines and people";
    scheme.author = "Ethan Schoonover";
    
    scheme.foreground = QColor("#839496");
    scheme.background = QColor("#002b36");
    scheme.cursor = QColor("#839496");
    scheme.cursorText = QColor("#002b36");
    scheme.selection = QColor("#073642");
    
    scheme.colors[0] = QColor("#073642");
    scheme.colors[1] = QColor("#dc322f");
    scheme.colors[2] = QColor("#859900");
    scheme.colors[3] = QColor("#b58900");
    scheme.colors[4] = QColor("#268bd2");
    scheme.colors[5] = QColor("#d33682");
    scheme.colors[6] = QColor("#2aa198");
    scheme.colors[7] = QColor("#eee8d5");
    
    scheme.brightColors[0] = QColor("#002b36");
    scheme.brightColors[1] = QColor("#cb4b16");
    scheme.brightColors[2] = QColor("#586e75");
    scheme.brightColors[3] = QColor("#657b83");
    scheme.brightColors[4] = QColor("#839496");
    scheme.brightColors[5] = QColor("#6c71c4");
    scheme.brightColors[6] = QColor("#93a1a1");
    scheme.brightColors[7] = QColor("#fdf6e3");
    
    return scheme;
}

ColorScheme ColorSchemeManager::createSolarizedLightScheme() {
    ColorScheme scheme = createSolarizedDarkScheme();
    scheme.name = "Solarized Light";
    scheme.id.clear();
    
    scheme.foreground = QColor("#657b83");
    scheme.background = QColor("#fdf6e3");
    scheme.cursor = QColor("#657b83");
    scheme.cursorText = QColor("#fdf6e3");
    scheme.selection = QColor("#eee8d5");
    
    return scheme;
}

ColorScheme ColorSchemeManager::createGithubDarkScheme() {
    ColorScheme scheme;
    scheme.name = "GitHub Dark";
    scheme.description = "GitHub's dark theme";
    
    scheme.foreground = QColor("#c9d1d9");
    scheme.background = QColor("#0d1117");
    scheme.cursor = QColor("#c9d1d9");
    scheme.cursorText = QColor("#0d1117");
    scheme.selection = QColor("#264f78");
    
    scheme.colors[0] = QColor("#484f58");
    scheme.colors[1] = QColor("#ff7b72");
    scheme.colors[2] = QColor("#7ee787");
    scheme.colors[3] = QColor("#d29922");
    scheme.colors[4] = QColor("#58a6ff");
    scheme.colors[5] = QColor("#bc8cff");
    scheme.colors[6] = QColor("#39d353");
    scheme.colors[7] = QColor("#c9d1d9");
    
    scheme.brightColors[0] = QColor("#6e7681");
    scheme.brightColors[1] = QColor("#ffa198");
    scheme.brightColors[2] = QColor("#aff5b4");
    scheme.brightColors[3] = QColor("#e3b341");
    scheme.brightColors[4] = QColor("#79c0ff");
    scheme.brightColors[5] = QColor("#d2a8ff");
    scheme.brightColors[6] = QColor("#56d364");
    scheme.brightColors[7] = QColor("#f0f6fc");
    
    return scheme;
}

ColorScheme ColorSchemeManager::createDraculaScheme() {
    ColorScheme scheme;
    scheme.name = "Dracula";
    scheme.description = "A dark theme for the soul";
    scheme.author = "Zeno Rocha";
    
    scheme.foreground = QColor("#f8f8f2");
    scheme.background = QColor("#282a36");
    scheme.cursor = QColor("#f8f8f2");
    scheme.cursorText = QColor("#282a36");
    scheme.selection = QColor("#44475a");
    
    scheme.colors[0] = QColor("#21222c");
    scheme.colors[1] = QColor("#ff5555");
    scheme.colors[2] = QColor("#50fa7b");
    scheme.colors[3] = QColor("#f1fa8c");
    scheme.colors[4] = QColor("#bd93f9");
    scheme.colors[5] = QColor("#ff79c6");
    scheme.colors[6] = QColor("#8be9fd");
    scheme.colors[7] = QColor("#f8f8f2");
    
    scheme.brightColors[0] = QColor("#6272a4");
    scheme.brightColors[1] = QColor("#ff6e6e");
    scheme.brightColors[2] = QColor("#69ff94");
    scheme.brightColors[3] = QColor("#ffffa5");
    scheme.brightColors[4] = QColor("#d6acff");
    scheme.brightColors[5] = QColor("#ff92df");
    scheme.brightColors[6] = QColor("#a4ffff");
    scheme.brightColors[7] = QColor("#ffffff");
    
    return scheme;
}

ColorScheme ColorSchemeManager::createNordScheme() {
    ColorScheme scheme;
    scheme.name = "Nord";
    scheme.description = "An arctic, north-bluish color palette";
    scheme.author = "Arctic Ice Studio";
    
    scheme.foreground = QColor("#d8dee9");
    scheme.background = QColor("#2e3440");
    scheme.cursor = QColor("#eceff4");
    scheme.cursorText = QColor("#3b4252");
    scheme.selection = QColor("#4c566a");
    
    scheme.colors[0] = QColor("#3b4252");
    scheme.colors[1] = QColor("#bf616a");
    scheme.colors[2] = QColor("#a3be8c");
    scheme.colors[3] = QColor("#ebcb8b");
    scheme.colors[4] = QColor("#81a1c1");
    scheme.colors[5] = QColor("#b48ead");
    scheme.colors[6] = QColor("#88c0d0");
    scheme.colors[7] = QColor("#e5e9f0");
    
    scheme.brightColors[0] = QColor("#4c566a");
    scheme.brightColors[1] = QColor("#bf616a");
    scheme.brightColors[2] = QColor("#a3be8c");
    scheme.brightColors[3] = QColor("#ebcb8b");
    scheme.brightColors[4] = QColor("#81a1c1");
    scheme.brightColors[5] = QColor("#b48ead");
    scheme.brightColors[6] = QColor("#8fbcbb");
    scheme.brightColors[7] = QColor("#eceff4");
    
    return scheme;
}

ColorScheme ColorSchemeManager::createGruvboxDarkScheme() {
    ColorScheme scheme;
    scheme.name = "Gruvbox Dark";
    scheme.description = "A retro groove color theme";
    scheme.author = "Moritz Gruenberg";
    
    scheme.foreground = QColor("#ebdbb2");
    scheme.background = QColor("#282828");
    scheme.cursor = QColor("#ebdbb2");
    scheme.cursorText = QColor("#282828");
    scheme.selection = QColor("#665c54");
    
    scheme.colors[0] = QColor("#282828");
    scheme.colors[1] = QColor("#cc241d");
    scheme.colors[2] = QColor("#98971a");
    scheme.colors[3] = QColor("#d79921");
    scheme.colors[4] = QColor("#458588");
    scheme.colors[5] = QColor("#b16286");
    scheme.colors[6] = QColor("#689d6a");
    scheme.colors[7] = QColor("#a89984");
    
    scheme.brightColors[0] = QColor("#928374");
    scheme.brightColors[1] = QColor("#fb4934");
    scheme.brightColors[2] = QColor("#b8bb26");
    scheme.brightColors[3] = QColor("#fabd2f");
    scheme.brightColors[4] = QColor("#83a598");
    scheme.brightColors[5] = QColor("#d3869b");
    scheme.brightColors[6] = QColor("#8ec07c");
    scheme.brightColors[7] = QColor("#ebdbb2");
    
    return scheme;
}

ColorScheme ColorSchemeManager::createOneDarkScheme() {
    ColorScheme scheme;
    scheme.name = "One Dark";
    scheme.description = "Atom's One Dark theme";
    
    scheme.foreground = QColor("#abb2bf");
    scheme.background = QColor("#282c34");
    scheme.cursor = QColor("#528bff");
    scheme.cursorText = QColor("#ffffff");
    scheme.selection = QColor("#3e4451");
    
    scheme.colors[0] = QColor("#181a1f");
    scheme.colors[1] = QColor("#e06c75");
    scheme.colors[2] = QColor("#98c379");
    scheme.colors[3] = QColor("#e5c07b");
    scheme.colors[4] = QColor("#61afef");
    scheme.colors[5] = QColor("#c678dd");
    scheme.colors[6] = QColor("#56b6c2");
    scheme.colors[7] = QColor("#abb2bf");
    
    scheme.brightColors[0] = QColor("#5c6370");
    scheme.brightColors[1] = QColor("#e06c75");
    scheme.brightColors[2] = QColor("#98c379");
    scheme.brightColors[3] = QColor("#e5c07b");
    scheme.brightColors[4] = QColor("#61afef");
    scheme.brightColors[5] = QColor("#c678dd");
    scheme.brightColors[6] = QColor("#56b6c2");
    scheme.brightColors[7] = QColor("#ffffff");
    
    return scheme;
}

QColor ColorSchemeManager::blendColors(const QColor& c1, const QColor& c2, qreal ratio) {
    int r = c1.red() + (c2.red() - c1.red()) * ratio;
    int g = c1.green() + (c2.green() - c1.green()) * ratio;
    int b = c1.blue() + (c2.blue() - c1.blue()) * ratio;
    return QColor(r, g, b);
}

QColor ColorSchemeManager::invertColor(const QColor& color) {
    return QColor(255 - color.red(), 255 - color.green(), 255 - color.blue());
}

QColor ColorSchemeManager::adjustBrightness(const QColor& color, int delta) {
    int h, s, l, a;
    color.getHsl(&h, &s, &l, &a);
    l = qBound(0, l + delta, 255);
    return QColor::fromHsl(h, s, l, a);
}

QColor ColorSchemeManager::getColor(const QString& schemeId, int colorIndex) const {
    if (!m_schemes.contains(schemeId)) return QColor();
    
    const ColorScheme& scheme = m_schemes[schemeId];
    
    if (colorIndex < 8) {
        return scheme.colors[colorIndex];
    } else if (colorIndex < 16) {
        return scheme.brightColors[colorIndex - 8];
    } else if (scheme.extendedColors.contains(colorIndex)) {
        return scheme.extendedColors[colorIndex];
    }
    
    return QColor();
}

QColor ColorSchemeManager::getForeground(const QString& schemeId) const {
    if (!m_schemes.contains(schemeId)) return QColor("#ffffff");
    return m_schemes[schemeId].foreground;
}

QColor ColorSchemeManager::getBackground(const QString& schemeId) const {
    if (!m_schemes.contains(schemeId)) return QColor("#000000");
    return m_schemes[schemeId].background;
}

QString ColorSchemeManager::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

#include "ColorSchemeEditor.moc"
