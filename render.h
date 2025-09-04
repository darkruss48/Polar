#ifndef RENDER_H
#define RENDER_H
#include "mainwindow.h"

#include <QMainWindow>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QGraphicsProxyWidget>

class Render
{
public:
    Render();
    static void createLineChartInGraphicsView(Ui::MainWindow *ui, const QString &hoursStr, const QString &pointsStr);
    static void render_leaderboard(MainWindow *this_, QGraphicsView *graphPlaceholder, const QString &hoursStr, const QString &pointsStr, const QString element, const QString &name, int rank = -1);
    // NEW: overlay helpers
    static bool addSeriesToExistingChart(QGraphicsView *view, const QString &hoursStr, const QString &pointsStr, const QString &seriesName, int rank = -1);
    static bool removeSeriesByName(QGraphicsView *view, const QString &seriesName);
    static void clearAllOverlaySeries(QGraphicsView *view, const QString &baseSeriesName);
    // NEW: query current chart
    static QChart* chartFromView(QGraphicsView* view);
};

#endif // RENDER_H
