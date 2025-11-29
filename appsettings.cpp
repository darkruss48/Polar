#include "appsettings.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtCharts/QChart>
#include <QStandardPaths>

// Statics (valeurs par défaut)
QString AppSettings::savedIdentifier = QString();
QString AppSettings::savedLanguage   = QStringLiteral("en_US");
int     AppSettings::chartThemeIndex = 0;
QString AppSettings::region          = QStringLiteral("Glo");
bool    AppSettings::useCustomBackground = false;
QString AppSettings::backgroundPath  = QString();
int     AppSettings::backgroundDimPercent = 40;
int     AppSettings::autoRefreshExtraDelayMinutes = 0;
bool    AppSettings::transparentControls = false;
int     AppSettings::selectedEdition = 0;      // 0 = édition courante
bool    AppSettings::censorIdDisplay = false;
bool    AppSettings::updateStartShortcutOnUpgrade = true;
int     AppSettings::dateFormatIndex = 0; // NEW: default = locale

// Chemin absolu: <applicationDirPath>/polar.json
QString AppSettings::configPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("polar.json"));
}

void AppSettings::load()
{
    const QString path = configPath();
    QFile f(path);
    if (!f.exists()) {
        save(); // crée un fichier avec les valeurs par défaut
        return;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        // Fichier illisible: garder les défauts
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;
    const QJsonObject o = doc.object();

    savedIdentifier = o.value(QStringLiteral("identifier")).toString(savedIdentifier);
    savedLanguage   = o.value(QStringLiteral("language")).toString(savedLanguage);
    chartThemeIndex = o.value(QStringLiteral("chartThemeIndex")).toInt(chartThemeIndex);
    region          = o.value(QStringLiteral("region")).toString(region);
    useCustomBackground = o.value(QStringLiteral("useCustomBackground")).toBool(useCustomBackground);
    backgroundPath  = o.value(QStringLiteral("backgroundPath")).toString(backgroundPath);
    backgroundDimPercent = qBound(0, o.value(QStringLiteral("backgroundDimPercent")).toInt(backgroundDimPercent), 100);
    autoRefreshExtraDelayMinutes = qBound(0, o.value(QStringLiteral("autoRefreshExtraDelayMinutes")).toInt(autoRefreshExtraDelayMinutes), 15);
    transparentControls = o.value(QStringLiteral("transparentControls")).toBool(transparentControls);
    selectedEdition = o.value(QStringLiteral("selectedEdition")).toInt(selectedEdition);
    censorIdDisplay = o.value(QStringLiteral("censorIdDisplay")).toBool(censorIdDisplay);
    updateStartShortcutOnUpgrade = o.value(QStringLiteral("updateStartShortcutOnUpgrade")).toBool(updateStartShortcutOnUpgrade);
}

void AppSettings::save()
{
    QJsonObject o;
    o.insert(QStringLiteral("identifier"), savedIdentifier);
    o.insert(QStringLiteral("language"), savedLanguage);
    o.insert(QStringLiteral("chartThemeIndex"), chartThemeIndex);
    o.insert(QStringLiteral("region"), region);
    o.insert(QStringLiteral("useCustomBackground"), useCustomBackground);
    o.insert(QStringLiteral("backgroundPath"), backgroundPath);
    o.insert(QStringLiteral("backgroundDimPercent"), backgroundDimPercent);
    o.insert(QStringLiteral("autoRefreshExtraDelayMinutes"), autoRefreshExtraDelayMinutes);
    o.insert(QStringLiteral("transparentControls"), transparentControls);
    o.insert(QStringLiteral("selectedEdition"), selectedEdition);
    o.insert(QStringLiteral("censorIdDisplay"), censorIdDisplay);
    o.insert(QStringLiteral("updateStartShortcutOnUpgrade"), updateStartShortcutOnUpgrade);

    const QJsonDocument doc(o);
    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
}

QChart::ChartTheme AppSettings::chartThemeEnum()
{
    // Mappe un index simple vers un thème QtCharts existant
    switch (chartThemeIndex) {
    case 0:  return QChart::ChartThemeBlueCerulean;
    case 1:  return QChart::ChartThemeLight;
    case 2:  return QChart::ChartThemeBlueNcs;
    case 3:  return QChart::ChartThemeBlueIcy;
    case 4:  return QChart::ChartThemeQt;
    case 5:  return QChart::ChartThemeBrownSand;
    case 6:  return QChart::ChartThemeDark;
    case 7:  return QChart::ChartThemeHighContrast;
    default: return QChart::ChartThemeLight;
    }
}
