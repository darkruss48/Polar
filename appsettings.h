#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>
#include <QtCharts/QChart>
#include <QJsonObject>

class AppSettings {
public:
    static QString region;              // "Glo" or "Jap"
    static QString chartThemeName;      // e.g. "ChartThemeBrownSand"
    // NEW: Background settings
    static bool useCustomBackground;
    static QString backgroundPath;
    static int backgroundDimPercent;    // 0..100

    // NEW: Privacy + persisted identifier
    static bool censorIdDisplay;      // false by default
    static QString savedIdentifier;   // persisted user ID

    // Load from polar.json (create with defaults if missing)
    static void load();
    // Save to polar.json
    static void save();

    // Helper: mapping to enum
    static QChart::ChartTheme chartThemeEnum();

private:
    static QString settingsFilePath();
    static QJsonObject toJson();
    static void fromJson(const QJsonObject& obj);
    static QChart::ChartTheme themeFromName(const QString& name);
    static QString nameFromTheme(QChart::ChartTheme theme);
};

#endif // APPSETTINGS_H
