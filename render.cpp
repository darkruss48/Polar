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

    // Si minY est négatif, définir minY à 0
    if (minY < 0) {
        minY = 0;
    }

    // Ajuster les marges selon vos besoins
    double margin = (maxY - minY) * 0.1; // 10% de marge
    maxY += margin; // Ajouter la marge seulement à maxY

    // Créer un QCategoryAxis pour l'axe Y
    QCategoryAxis *axisY = new QCategoryAxis();
    axisY->setRange(minY, maxY);

    // Déterminer le nombre de graduations souhaitées
    int tickCount = 7; // Vous pouvez ajuster ce nombre selon vos besoins

    // Calculer les valeurs des graduations
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

    // Appliquer le thème céruléen
    chart->setTheme(QChart::ChartThemeBrownSand);

    // Ajuster les axes du graphique
    adjustChartAxes(chart, points);

    // Agrandir le graphique
    // Créer un QGraphicsScene et intégrer le graphique
    QGraphicsScene *scene = new QGraphicsScene();
    // QGraphicsView *view = new QGraphicsView(scene);
    QGraphicsView *view = ui->graphiqueTest;
    // Mettre la scène
    view->setScene(scene);
    view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);

    // Ajout du graphique dans la scène via un proxy
    QGraphicsProxyWidget *proxy = scene->addWidget(new QChartView(chart));
    proxy->setRotation(0); // Optionnel : manipulation dans la scène
    proxy->setPos(0, 0); // Place le graphique en haut à gauche de la scène
    QRectF adjustedRect = view->rect().adjusted(1, 1, -1, -1);
    proxy->setGeometry(adjustedRect);
    // Configurer la vue
    view->setRenderHint(QPainter::Antialiasing);
    // Avoir un graphique aussi grand que la vue
    // view->setSceneRect(0, 0, view->width()*3, view->height()*3); // utile pour avoir les "scrolls bars" sur la vue
    // view->resize(800, 600);
    view->show();
}
