#include "render.h"
#include "mainwindow.h"
#include <iostream>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCharts/QChart>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtCharts/QCategoryAxis>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QGraphicsProxyWidget>
#include <QTranslator>
#include <QtCharts/QChartView>
// #include <QtCharts/QChartTheme>
#include "ui_mainwindow.h"
#include "appsettings.h" // +
// NEW
#include <cmath>

Render::Render() {}

// Fonction pour convertir une chaîne JSON de tableau en liste de doubles
QList<double> parseJsonArray(const QString &jsonString) {
    QList<double> values;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (!doc.isArray())
        return values;

    QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        values.append(val.toDouble());
    }
    return values;
}

// Fonction pour ajuster les axes du graphique
void adjustChartAxes(QChart *chart, const QList<double> &points) {
    if (points.isEmpty()) return;

    // Trouver les valeurs minimales et maximales des données
    double minY = *std::min_element(points.begin(), points.end());
    double maxY = *std::max_element(points.begin(), points.end());

    if (minY < 0) {
        minY = 0;
    }

    double margin = (maxY - minY) * 0.1; // 10% de marge
    maxY += margin;

    QCategoryAxis *axisY = new QCategoryAxis();
    axisY->setRange(minY, maxY);
    axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue); // NEW: labels sur les traits

    int tickCount = 7;
    double interval = (maxY - minY) / tickCount;
    for (int i = 0; i <= tickCount; ++i) {
        double value = minY + i * interval;
        QString label;
        if (value >= 1000000) {
            label = QString::number(value / 1000000.0, 'f', 1) + " M";
        } else if (value >= 1000) {
            label = QString::number(value / 1000.0, 'f', 1) + " k";
        } else {
            label = QString::number(value, 'f', 0);
        }
        axisY->append(label, value);
    }

    // supprimer l'axe Y de base
    chart->removeAxis(chart->axes(Qt::Vertical).first());

    // mettre l'axe Y personnalisé
    chart->addAxis(axisY, Qt::AlignLeft);
    chart->series().first()->attachAxis(axisY);
}

void adjustChartAxes_leaderboard(QChart *chart, const QList<double> &points) {
    if (points.isEmpty()) return;

    // Plage Y: commence à 0, ajoute une marge haute
    double minY = 0.0;
    double maxY = *std::max_element(points.begin(), points.end());
    double margin = (maxY - minY) * 0.1;
    maxY += margin;

    // Axe Y en catégories, labels posés sur les traits
    auto axisY = new QCategoryAxis();
    axisY->setRange(minY, maxY);
    axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue); // NEW

    // Ticks tous les 2 (0,2,4,...) jusqu'au max visible
    const int topEven = static_cast<int>(std::ceil(maxY / 2.0)) * 2;
    for (int v = 0; v <= topEven; v += 2) {
        axisY->append(QString::number(v), static_cast<double>(v));
    }

    // Remplace l’axe vertical par le nouvel axe
    const auto vAxes = chart->axes(Qt::Vertical);
    if (!vAxes.isEmpty()) chart->removeAxis(vAxes.first());
    chart->addAxis(axisY, Qt::AlignLeft);
    if (!chart->series().isEmpty()) chart->series().first()->attachAxis(axisY);
}


// Fonction pour créer et afficher un graphique dans un QGraphicsView
void Render::createLineChartInGraphicsView(Ui::MainWindow *ui, const QString &hoursStr, const QString &pointsStr) {
    // Conversion des chaînes JSON en listes de valeurs
    QList<double> hours = parseJsonArray(hoursStr);
    QList<double> points = parseJsonArray(pointsStr);

    // Vérification que les listes ont la même taille
    if (hours.size() != points.size()) {
        qWarning("Les tailles des données ne correspondent pas !");
        // std::cout << hours.size() << " " << points.size() << std::endl;

        return;
    }

    // Création d'une série pour le graphique en ligne
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < hours.size(); ++i) {
        series->append(hours[i], points[i]);
    }

    // Création du graphique
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("\""+ui->ydataBox->currentText() +QObject::tr("\" en fonction des heures"));
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText(QObject::tr("Heures"));
    chart->axes(Qt::Vertical).first()->setTitleText(QObject::tr("Points"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Appliquer le thème depuis les paramètres
    chart->setTheme(AppSettings::chartThemeEnum());

    // NEW: marges légères pour éviter le chevauchement des labels
    chart->setMargins(QMargins(12, 8, 8, 14));
    chart->setBackgroundRoundness(0);

    // Ajuster l’axe Y
    adjustChartAxes(chart, points);

    // NEW: Axe X personnalisé en heures (catégories à 6h ou 12h)
    if (!hours.isEmpty()) {
        const double minX = hours.first();
        const double maxX = hours.last();
        const double span = std::max(0.0, maxX - minX);
        const double step = (span <= 36.0 ? 6.0 : 12.0);

        auto axisX = new QCategoryAxis();
        axisX->setRange(minX, maxX);
        axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

        if (minX <= 0.0) axisX->append(QStringLiteral("0h"), 0.0);

        double start = std::ceil((minX > 0.0 ? minX : 0.0) / step) * step;
        for (double v = start; v <= maxX + 1e-6; v += step) {
            axisX->append(QString::number(v, 'f', 0) + "h", v);
        }

        // Remplacer l’axe horizontal par le nouvel axe
        const auto hAxes = chart->axes(Qt::Horizontal);
        for (auto a : hAxes) chart->removeAxis(a);
        chart->addAxis(axisX, Qt::AlignBottom);
        if (!chart->series().isEmpty()) chart->series().first()->attachAxis(axisX);
    }

    // Prepare view/scene
    QGraphicsView *view = ui->graphiqueTest;
    if (!view->scene()) {
        view->setScene(new QGraphicsScene(view));
    }
    QGraphicsScene* scene = view->scene();
    scene->clear(); // NEW: remove previous chart widgets

    // NEW: make scene exactly the viewport size and disable scrollbars
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QRectF vpRect(0, 0, view->viewport()->width(), view->viewport()->height());
    scene->setSceneRect(vpRect);

    // Create the chartview widget with no frame and no margins
    auto chartView = new QChartView(chart);
    chartView->setFrameShape(QFrame::NoFrame);
    chartView->setLineWidth(0);
    chartView->setContentsMargins(0, 0, 0, 0);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Add to scene and fill all space
    QGraphicsProxyWidget *proxy = scene->addWidget(chartView);
    proxy->setPos(0, 0);
    proxy->setGeometry(vpRect); // NEW: fill exactly the viewport

    view->setRenderHint(QPainter::Antialiasing);
    view->show();
}

void Render::render_leaderboard(MainWindow *this_, QGraphicsView *graphPlaceholder, const QString &hoursStr, const QString &pointsStr, const QString element, const QString &name)
{
    // Conversion des chaînes JSON en listes de valeurs
    QList<double> hours = parseJsonArray(hoursStr);
    QList<double> points = parseJsonArray(pointsStr);

    // Vérification que les listes ont la même taille
    if (hours.size() != points.size()) {
        qWarning("Les tailles des données ne correspondent pas !");
        // std::cout << hours.size() << " " << points.size() << std::endl;

        return;
    }

    // Création d'une série pour le graphique en ligne
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < hours.size(); ++i) {
        series->append(hours[i], points[i]);
    }

    // Création du graphique
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(name + " : " + "\""+element +QObject::tr("\" en fonction des heures"));
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText(QObject::tr("Heures"));
    chart->axes(Qt::Vertical).first()->setTitleText(QObject::tr("Points"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Appliquer le thème depuis les paramètres
    chart->setTheme(AppSettings::chartThemeEnum());

    // Laisse un peu d'espace pour les labels des axes (évite l'effet "sous les traits")
    chart->setMargins(QMargins(12, 8, 8, 14)); // NEW: marges légères

    // Ajuster l'axe Y
    adjustChartAxes_leaderboard(chart, points);

    // NEW: Axe X personnalisé en heures (catégories à 6h ou 12h)
    if (!hours.isEmpty()) {
        const double minX = hours.first();
        const double maxX = hours.last();
        const double span = std::max(0.0, maxX - minX);
        const double step = (span <= 36.0 ? 6.0 : 12.0);

        auto axisX = new QCategoryAxis();
        axisX->setRange(minX, maxX);
        axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

        // Ajoute 0h si visible
        if (minX <= 0.0) axisX->append(QStringLiteral("0h"), 0.0);

        // Première étiquette alignée sur un multiple de 'step'
        double start = std::ceil((minX > 0.0 ? minX : 0.0) / step) * step;
        for (double v = start; v <= maxX + 1e-6; v += step) {
            axisX->append(QString::number(v, 'f', 0) + "h", v);
        }

        // Remplace l’axe horizontal par le nouvel axe
        const auto hAxes = chart->axes(Qt::Horizontal);
        for (auto a : hAxes) chart->removeAxis(a);
        chart->addAxis(axisX, Qt::AlignBottom);
        if (!chart->series().isEmpty()) chart->series().first()->attachAxis(axisX);
    }

    // Ensure a scene exists and is empty
    if (!graphPlaceholder->scene()) {
        graphPlaceholder->setScene(new QGraphicsScene(graphPlaceholder));
    }
    QGraphicsScene *scene = graphPlaceholder->scene();
    scene->clear();
    graphPlaceholder->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphPlaceholder->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QRectF vpRect(0, 0, graphPlaceholder->viewport()->width(), graphPlaceholder->viewport()->height());
    scene->setSceneRect(vpRect);

    auto chartView = new QChartView(chart);
    chartView->setFrameShape(QFrame::NoFrame);
    chartView->setLineWidth(0);
    chartView->setContentsMargins(0, 0, 0, 0);
    chartView->setRenderHint(QPainter::Antialiasing);

    QGraphicsProxyWidget *proxy = scene->addWidget(chartView);
    proxy->setPos(0, 0);
    proxy->setGeometry(vpRect);

    graphPlaceholder->setRenderHint(QPainter::Antialiasing);
    graphPlaceholder->show();
}
