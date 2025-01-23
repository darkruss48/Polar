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
    static void render_leaderboard(MainWindow *this_, QGraphicsView *graphPlaceholder, const QString &hoursStr, const QString &pointsStr, const QString element, const QString &name);
};

#endif // RENDER_H
