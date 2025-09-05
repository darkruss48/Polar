#include "appsettings.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

// Défauts
QString AppSettings::savedIdentifier = "";
QString AppSettings::savedLanguage = "en_US";
QString AppSettings::region = "Glo";
bool AppSettings::censorIdDisplay = false;
bool AppSettings::useCustomBackground = false;
QString AppSettings::backgroundPath = "";
int AppSettings::backgroundDimPercent = 0;
int AppSettings::autoRefreshExtraDelayMinutes = 0;
QString AppSettings::chartThemeName = "";
int AppSettings::chartThemeIndex = 0;
bool AppSettings::transparentControls = false;

static QString settingsFile()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
           .filePath("polar.json");
}

// Ordre FIXE (NE PAS CHANGER L’ORDRE)
static const QVector<QPair<QString,QChart::ChartTheme>> kThemes = {
    { QStringLiteral("Bleu Céruléen"),       QChart::ChartThemeBlueCerulean },
    { QStringLiteral("Clair - Bleu 1"),      QChart::ChartThemeLight       },
    { QStringLiteral("Clair - Bleu 2"),      QChart::ChartThemeBlueNcs     },
    { QStringLiteral("Clair - Bleu 3"),      QChart::ChartThemeBlueIcy     },
    { QStringLiteral("Clair - Noir"),        QChart::ChartThemeHighContrast},
    { QStringLiteral("Clair - Vert"),        QChart::ChartThemeQt          },
    { QStringLiteral("Sable"),               QChart::ChartThemeBrownSand   },
    { QStringLiteral("Thème sombre"),        QChart::ChartThemeDark        }
};

void AppSettings::load()
{
    QFile f(settingsFile());
    if (!f.open(QIODevice::ReadOnly)) {
        save(); // créer fichier par défaut
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;
    QJsonObject o = doc.object();

    savedIdentifier = o.value("savedIdentifier").toString(savedIdentifier);
    savedLanguage   = o.value("savedLanguage").toString(savedLanguage);
    region          = o.value("region").toString(region);
    censorIdDisplay = o.value("censorIdDisplay").toBool(censorIdDisplay);
    useCustomBackground = o.value("useCustomBackground").toBool(useCustomBackground);
    backgroundPath  = o.value("backgroundPath").toString(backgroundPath);
    backgroundDimPercent = o.value("backgroundDimPercent").toInt(backgroundDimPercent);
    autoRefreshExtraDelayMinutes = o.value("autoRefreshExtraDelayMinutes").toInt(autoRefreshExtraDelayMinutes);
    // NEW: transparence des widgets
    transparentControls = o.value("transparentControls").toBool(transparentControls);

    // Legacy name
    chartThemeName  = o.value("chartThemeName").toString(chartThemeName);

    // NEW: index prioritaire
    if (o.contains("chartThemeIndex")) {
        chartThemeIndex = o.value("chartThemeIndex").toInt(chartThemeIndex);
    } else {
        // Rétro-compatibilité: dériver index depuis l’ancien nom si possible
        if (!chartThemeName.isEmpty()) {
            for (int i = 0; i < kThemes.size(); ++i) {
                if (kThemes[i].first == chartThemeName) {
                    chartThemeIndex = i;
                    break;
                }
            }
        }
    }
    if (chartThemeIndex < 0 || chartThemeIndex >= kThemes.size())
        chartThemeIndex = 0;
}

void AppSettings::save()
{
    QDir().mkpath(QFileInfo(settingsFile()).path());
    QJsonObject o;
    o["savedIdentifier"] = savedIdentifier;
    o["savedLanguage"] = savedLanguage;
    o["region"] = region;
    o["censorIdDisplay"] = censorIdDisplay;
    o["useCustomBackground"] = useCustomBackground;
    o["backgroundPath"] = backgroundPath;
    o["backgroundDimPercent"] = backgroundDimPercent;
    o["autoRefreshExtraDelayMinutes"] = autoRefreshExtraDelayMinutes;
    // NEW: transparence des widgets
    o["transparentControls"] = transparentControls;

    // Conserver aussi le nom (lecture humaine) mais index source de vérité
    if (chartThemeIndex < 0 || chartThemeIndex >= kThemes.size())
        chartThemeIndex = 0;
    o["chartThemeIndex"] = chartThemeIndex;
    o["chartThemeName"]  = kThemes[chartThemeIndex].first;

    QFile f(settingsFile());
    if (f.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        f.close();
    }
}

QChart::ChartTheme AppSettings::chartThemeEnum()
{
    if (chartThemeIndex < 0 || chartThemeIndex >= kThemes.size())
        chartThemeIndex = 0;
    return kThemes[chartThemeIndex].second;
}
