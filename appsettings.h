#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>
#include <QtCharts/QChart>
#include <QJsonObject>

class AppSettings {
public:
    // Chemin absolu du fichier de config ("polar.json" à côté de l'exécutable)
    static QString configPath();

    // (Dé)sérialisation
    static void load();
    static void save();

    // Mappage de l'index de thème -> QChart::ChartTheme
    static QChart::ChartTheme chartThemeEnum();

    // Données persistées
    static QString savedIdentifier;
    static QString savedLanguage;
    static int     chartThemeIndex;
    static QString region; // "Glo" ou "Jap"
    static bool    useCustomBackground;
    static QString backgroundPath;
    static int     backgroundDimPercent;          // 0..100
    static int     autoRefreshExtraDelayMinutes;  // 0..15
    static bool    transparentControls;
    static int     selectedEdition;               // 0 = courant
    static bool    censorIdDisplay;
};

#endif // APPSETTINGS_H
