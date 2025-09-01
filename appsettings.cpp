#include "appsettings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMap>

QString AppSettings::region = "Glo";
QString AppSettings::chartThemeName = "ChartThemeBrownSand";
// NEW defaults
bool AppSettings::censorIdDisplay = false;
QString AppSettings::savedIdentifier = "";
bool AppSettings::useCustomBackground = false;
QString AppSettings::backgroundPath = "";
int AppSettings::backgroundDimPercent = 0; // 0 = transparent, 100 = noir

static QString appDirPath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + "/.polar";
    }
    QDir().mkpath(base);
    return base;
}

QString AppSettings::settingsFilePath() {
    return appDirPath() + "/polar.json";
}

QJsonObject AppSettings::toJson() {
    QJsonObject obj;
    obj["region"] = region;
    obj["chartTheme"] = chartThemeName;
    // NEW
    obj["useCustomBackground"] = useCustomBackground;
    obj["backgroundPath"] = backgroundPath;
    obj["backgroundDimPercent"] = backgroundDimPercent;
    obj["censorIdDisplay"] = censorIdDisplay;
    obj["savedIdentifier"] = savedIdentifier;
    return obj;
}

void AppSettings::fromJson(const QJsonObject& obj) {
    region = obj.value("region").toString(region);
    chartThemeName = obj.value("chartTheme").toString(chartThemeName);
    // NEW
    useCustomBackground = obj.value("useCustomBackground").toBool(useCustomBackground);
    backgroundPath = obj.value("backgroundPath").toString(backgroundPath);
    backgroundDimPercent = obj.value("backgroundDimPercent").toInt(backgroundDimPercent);
    censorIdDisplay = obj.value("censorIdDisplay").toBool(censorIdDisplay);
    savedIdentifier = obj.value("savedIdentifier").toString(savedIdentifier);
}

void AppSettings::load() {
    QFile f(settingsFilePath());
    if (!f.exists()) {
        save();
        return;
    }
    if (f.open(QIODevice::ReadOnly)) {
        const auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (doc.isObject()) {
            fromJson(doc.object());
        }
    }
}

void AppSettings::save() {
    QFile f(settingsFilePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(toJson());
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
    }
}

QChart::ChartTheme AppSettings::themeFromName(const QString& name) {
    static const QMap<QString, QChart::ChartTheme> map = {
        {"Clair - Bleu 1", QChart::ChartThemeLight},
        {"Bleu Céruléen", QChart::ChartThemeBlueCerulean},
        {"Thème sombre", QChart::ChartThemeDark},
        {"Sable", QChart::ChartThemeBrownSand},
        {"Clair - Bleu 2", QChart::ChartThemeBlueNcs},
        {"Clair - Noir", QChart::ChartThemeHighContrast},
        {"Clair - Bleu 3", QChart::ChartThemeBlueIcy},
        {"Clair - Vert", QChart::ChartThemeQt}
    };
    return map.value(name, QChart::ChartThemeBrownSand);
}

QString AppSettings::nameFromTheme(QChart::ChartTheme theme) {
    switch (theme) {
        case QChart::ChartThemeLight: return "Clair - Bleu";
        case QChart::ChartThemeBlueCerulean: return "Bleu Céruléen";
        case QChart::ChartThemeDark: return "Thème sombre";
        case QChart::ChartThemeBrownSand: return "Sable";
        case QChart::ChartThemeBlueNcs: return "Clair - Bleu 2";
        case QChart::ChartThemeHighContrast: return "Clair - Noir";
        case QChart::ChartThemeBlueIcy: return "Clair - Bleu 3";
        case QChart::ChartThemeQt: return "Clair - Vert";
        default: return "Sable";
    }
}

QChart::ChartTheme AppSettings::chartThemeEnum() {
    return themeFromName(chartThemeName);
}
