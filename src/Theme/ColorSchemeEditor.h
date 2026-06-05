#ifndef COLOR_SCHEME_EDITOR_H
#define COLOR_SCHEME_EDITOR_H

#include <QObject>
#include <QMap>
#include <QColor>

struct ColorScheme {
    QString id;
    QString name;
    QString description;
    QString author;
    
    // 基础颜色
    QColor foreground;
    QColor background;
    QColor cursor;
    QColor cursorText;
    QColor selection;
    QColor selectionText;
    
    // ANSI 颜色 (0-7)
    QColor colors[8];
    
    // 高亮 ANSI 颜色 (8-15)
    QColor brightColors[8];
    
    // 扩展颜色 (16-255)
    QMap<int, QColor> extendedColors;
    
    // 元数据
    qint64 createdAt;
    qint64 modifiedAt;
    bool isBuiltin;
    QString parentScheme;  // 继承自哪个配色方案
    
    ColorScheme() : createdAt(0), modifiedAt(0), isBuiltin(false) {
        // 默认值
        foreground = QColor("#ffffff");
        background = QColor("#000000");
        cursor = QColor("#ffffff");
        cursorText = QColor("#000000");
        selection = QColor("#444444");
        selectionText = QColor("#ffffff");
        
        // 默认 ANSI 颜色
        colors[0] = QColor("#000000");  // Black
        colors[1] = QColor("#cd3131");  // Red
        colors[2] = QColor("#0dbc79");  // Green
        colors[3] = QColor("#e5e510");  // Yellow
        colors[4] = QColor("#2472c8");  // Blue
        colors[5] = QColor("#bc3fbc");  // Magenta
        colors[6] = QColor("#11a8cd");  // Cyan
        colors[7] = QColor("#e5e5e5");  // White
        
        // 默认高亮颜色
        brightColors[0] = QColor("#666666");
        brightColors[1] = QColor("#f14c4c");
        brightColors[2] = QColor("#23d18b");
        brightColors[3] = QColor("#f5f543");
        brightColors[4] = QColor("#3b8eea");
        brightColors[5] = QColor(#d670d6");
        brightColors[6] = QColor("#29b8db");
        brightColors[7] = QColor("#e5e5e5");
    }
};

class ColorSchemeManager : public QObject {
    Q_OBJECT
public:
    explicit ColorSchemeManager(QObject* parent = nullptr);
    
    static ColorSchemeManager* instance();
    
    // 配色方案管理
    QString createScheme(const ColorScheme& scheme);
    void deleteScheme(const QString& id);
    void updateScheme(const QString& id, const ColorScheme& scheme);
    
    // 查询
    ColorScheme getScheme(const QString& id) const;
    QList<ColorScheme> getAllSchemes() const;
    QList<ColorScheme> getBuiltinSchemes() const;
    QList<ColorScheme> getUserSchemes() const;
    
    // 内置配色
    void loadBuiltinSchemes();
    
    // 导入导出
    void exportScheme(const QString& id, const QString& filePath);
    void importScheme(const QString& filePath);
    QString importSchemeFromText(const QString& jsonText);
    
    // 预设
    ColorScheme createMonokaiScheme();
    ColorScheme createSolarizedDarkScheme();
    ColorScheme createSolarizedLightScheme();
    ColorScheme createGithubDarkScheme();
    ColorScheme createDraculaScheme();
    ColorScheme createNordScheme();
    ColorScheme createGruvboxDarkScheme();
    ColorScheme createOneDarkScheme();
    
    // 颜色操作
    static QColor blendColors(const QColor& c1, const QColor& c2, qreal ratio);
    static QColor invertColor(const QColor& color);
    static QColor adjustBrightness(const QColor& color, int delta);
    
    // 获取颜色
    QColor getColor(const QString& schemeId, int colorIndex) const;
    QColor getForeground(const QString& schemeId) const;
    QColor getBackground(const QString& schemeId) const;
    
signals:
    void schemeAdded(const QString& id);
    void schemeRemoved(const QString& id);
    void schemeUpdated(const QString& id);
    void currentSchemeChanged(const QString& id);

private:
    static ColorSchemeManager* s_instance;
    
    QMap<QString, ColorScheme> m_schemes;
    QString m_schemesFile;
    
    QString generateId();
};

#endif
