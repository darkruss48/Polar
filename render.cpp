#include "render.h"
#include "mainwindow.h"
#include <iostream>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>   // NEW

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
#include <QMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <algorithm> // NEW: for sort/min/max/clamp

// Helper: get the QChart currently shown in a QGraphicsView's scene
static QChart* chartFromGraphicsView(QGraphicsView* view) {
    if (!view) return nullptr;
    QGraphicsScene* scene = view->scene();
    if (!scene) return nullptr;
    const auto items = scene->items();
    for (QGraphicsItem* gi : items) {
        if (auto proxy = dynamic_cast<QGraphicsProxyWidget*>(gi)) {
            if (auto cv = qobject_cast<QChartView*>(proxy->widget())) {
                return cv->chart();
            }
        }
    }
    return nullptr;
}

// NEW: sélection de plage + bulle d’infos
class SelectionHandler : public QObject {
    // Q_OBJECT // REMOVED: no moc needed, avoids linker errors
public:
    SelectionHandler(QChartView* view, QChart* chart)
        : QObject(view), view(view), chart(chart) {
        // Install on the viewport (QGraphicsView delivers mouse events to viewport)
        if (view && view->viewport()) {
            view->viewport()->installEventFilter(this);
            view->viewport()->setMouseTracking(true); // ensure move events while dragging
        }
        view->setMouseTracking(true);
        scene = chart ? chart->scene() : nullptr;
    }

    bool eventFilter(QObject* obj, QEvent* ev) override {
        // Only handle events coming from the viewport
        if (!view || obj != view->viewport()) return QObject::eventFilter(obj, ev);
        switch (ev->type()) {
            case QEvent::MouseButtonPress: {
                auto* me = static_cast<QMouseEvent*>(ev);
                if (me->button() == Qt::LeftButton) {
                    pressPos = me->pos();
                    dragging = true;
                    moved = false;
                    ensureBand();
                    const double cx = mouseToChartX(pressPos);
                    updateBandRect(cx, cx);
                    return true;
                }
                break;
            }
            case QEvent::MouseMove: {
                auto* me = static_cast<QMouseEvent*>(ev);
                if (dragging) {
                    if (!moved && (me->pos() - pressPos).manhattanLength() > 3) moved = true;
                    const double c0 = mouseToChartX(pressPos);
                    const double c1 = mouseToChartX(me->pos());
                    updateBandRect(c0, c1);
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                auto* me = static_cast<QMouseEvent*>(ev);
                if (me->button() == Qt::LeftButton) {
                    if (!moved) {
                        clearVisuals(); // simple click: clear band + bubble
                    } else {
                        const QRectF plot = chart->plotArea();
                        const double left = std::min(selLeft, selRight);
                        const double right = std::max(selLeft, selRight);
                        if (right - left > 2.0) { // pixels in chart coords
                            const double xMin = chart->property("xMin").toDouble();
                            const double xMax = chart->property("xMax").toDouble();
                            const double v0 = chartXToValue(left, plot, xMin, xMax);
                            const double v1 = chartXToValue(right, plot, xMin, xMax);
                            showBubbleForRange(std::min(v0,v1), std::max(v0,v1));
                        }
                    }
                    dragging = false;
                    moved = false;
                    return true;
                }
                break;
            }
            default:
                break;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QChartView* view = nullptr;
    QChart* chart = nullptr;
    QGraphicsScene* scene = nullptr;
    bool dragging = false;
    bool moved = false;
    QPoint pressPos;
    QGraphicsRectItem* band = nullptr;
    // bubble
    QGraphicsRectItem* bubbleBg = nullptr;
    QGraphicsTextItem* bubbleText = nullptr;
    // selection bounds in chart coords (pixels relative to chart item)
    double selLeft = 0.0, selRight = 0.0;

    void ensureBand() {
        if (!scene) return;
        if (!band) {
            band = scene->addRect(QRectF(), QPen(Qt::NoPen), QBrush(QColor(0,0,0,80)));
            band->setZValue(1000);
        }
    }
    void clearBubble() {
        if (scene && bubbleText) { scene->removeItem(bubbleText); delete bubbleText; bubbleText = nullptr; }
        if (scene && bubbleBg)   { scene->removeItem(bubbleBg);   delete bubbleBg;   bubbleBg = nullptr; }
    }
    void clearBand() {
        if (scene && band) { scene->removeItem(band); delete band; band = nullptr; }
    }
    void clearVisuals() {
        clearBand();
        clearBubble();
    }

    double mouseToChartX(const QPoint& pos) const {
        const QPointF scenePt = view->mapToScene(pos);
        const QPointF chartPt = chart->mapFromScene(scenePt);
        return chartPt.x();
    }
    static double chartXToValue(double cx, const QRectF& plot, double xMinVal, double xMaxVal) {
        if (plot.width() <= 0.0) return xMinVal;
        const double t = std::clamp((cx - plot.left()) / plot.width(), 0.0, 1.0);
        return xMinVal + t * (xMaxVal - xMinVal);
    }
    void updateBandRect(double x0, double x1) {
        if (!scene || !chart) return;
        const QRectF plot = chart->plotArea();
        selLeft = x0; selRight = x1;

        // Map chart coords -> scene coords for a vertical full-height band over plot
        const double leftChartX  = std::min(x0, x1);
        const double rightChartX = std::max(x0, x1);

        const QPointF plotTopLeftScene    = chart->mapToScene(plot.topLeft());
        const QPointF plotBottomLeftScene = chart->mapToScene(QPointF(plot.left(), plot.bottom()));
        const double plotTopSceneY = plotTopLeftScene.y();
        const double plotHeightScene = plotBottomLeftScene.y() - plotTopSceneY;

        const double leftSceneX  = chart->mapToScene(QPointF(leftChartX,  plot.top())).x();
        const double rightSceneX = chart->mapToScene(QPointF(rightChartX, plot.top())).x();
        const double wScene = std::abs(rightSceneX - leftSceneX);

        if (band) {
            band->setRect(QRectF(QPointF(std::min(leftSceneX, rightSceneX), plotTopSceneY),
                                 QSizeF(wScene, plotHeightScene)));
            band->setVisible(true);
        }
        // moving selection hides bubble until release
        clearBubble();
    }

    void showBubbleForRange(double xFrom, double xTo) {
        if (!scene || !chart) return;

        // Collect averages per series within [xFrom, xTo]
        struct Row { int rank; QString name; double avg; };
        QVector<Row> rows;

        for (auto* s : chart->series()) {
            auto* ls = qobject_cast<QLineSeries*>(s);
            if (!ls) continue;
            const auto pts = ls->points();
            if (pts.isEmpty()) continue;

            double sum = 0.0; int cnt = 0;
            for (const QPointF& p : pts) {
                if (p.x() >= xFrom - 1e-9 && p.x() <= xTo + 1e-9) {
                    sum += p.y(); cnt++;
                }
            }
            if (cnt <= 0) continue;
            const double avg = sum / cnt;
            const int rank = ls->property("rank").isValid() ? ls->property("rank").toInt() : -1;
            rows.push_back({ rank, ls->name(), avg });
        }

        std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b){
            return a.avg > b.avg; // desc
        });

        // Header: selected time slice + percentage of total time
        const double allMin = chart->property("xMin").toDouble();
        const double allMax = chart->property("xMax").toDouble();
        double pct = 0.0;
        if (allMax > allMin + 1e-9) {
            pct = ((xTo - xFrom) / (allMax - allMin)) * 100.0;
            pct = std::clamp(pct, 0.0, 100.0);
        }
        auto formatHourLabel = [](double h)->QString {
            if (h <= 0.0) return QStringLiteral("0h");
            const int totalMin = qRound(h * 60.0);
            const int hh = totalMin / 60;
            const int mm = totalMin % 60;
            return mm == 0 ? QString("%1h").arg(hh) : QString("%1h%2").arg(hh).arg(mm, 2, 10, QLatin1Char('0'));
        };
        QStringList lines;
        lines << QString("%1 – %2 (%3%)")
                    .arg(formatHourLabel(std::min(xFrom, xTo)),
                         formatHourLabel(std::max(xFrom, xTo)),
                         QString::number(pct, 'f', 1));
        lines << ""; // blank line above ranking

        // Format rows: "#rank name : value"
        for (const Row& r : rows) {
            const QString left = (r.rank > 0) ? QString("#%1 %2").arg(r.rank).arg(r.name) : r.name;
            lines << QString("%1 : %2").arg(left).arg(QString::number(r.avg, 'f', 2));
        }
        const QString text = lines.join("\n");
        if (text.trimmed().isEmpty()) {
            clearBubble();
            return;
        }

        if (!bubbleText) {
            bubbleText = scene->addText(QString());
            bubbleText->setDefaultTextColor(Qt::white);
            bubbleText->setZValue(1100);
        }
        bubbleText->setPlainText(text);

        if (!bubbleBg) {
            bubbleBg = scene->addRect(QRectF(), QPen(QColor(255,255,255,30)), QBrush(QColor(0,0,0,200)));
            bubbleBg->setZValue(1090);
        }

        // Place bubble near right side of selection, within plot bounds
        const QRectF plot = chart->plotArea();
        const double leftChartX  = std::min(selLeft, selRight);
        const double rightChartX = std::max(selLeft, selRight);
        const double rightSceneX = chart->mapToScene(QPointF(rightChartX, plot.top())).x();
        const QPointF plotTopLeftScene    = chart->mapToScene(plot.topLeft());
        const QPointF plotTopRightScene   = chart->mapToScene(plot.topRight());

        const double plotLeftSceneX  = plotTopLeftScene.x();
        const double plotRightSceneX = plotTopRightScene.x();
        const double by = plotTopLeftScene.y() + 10.0;

        const QSizeF pad(12, 10);
        const QRectF bb = bubbleText->boundingRect();
        double bx = std::min(rightSceneX + 10.0, plotRightSceneX - (bb.width() + pad.width()));
        bx = std::max(plotLeftSceneX + 6.0, bx);

        bubbleText->setPos(QPointF(bx + pad.width()/2.0, by + pad.height()/2.0));
        bubbleBg->setRect(QRectF(QPointF(bx, by), QSizeF(bb.width() + pad.width(), bb.height() + pad.height())));
        bubbleBg->setVisible(true);
        bubbleText->setVisible(true);
    }
};

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

void Render::render_leaderboard(MainWindow *this_, QGraphicsView *graphPlaceholder, const QString &hoursStr, const QString &pointsStr, const QString element, const QString &name, int rank)
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
    series->setName(name); // base series named after the player
    series->setProperty("rank", rank); // NEW: store rank for later display

    // Création du graphique
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(name + " : " + "\""+element +QObject::tr("\" en fonction des heures"));
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText(QObject::tr("Heures"));
    chart->axes(Qt::Vertical).first()->setTitleText(QObject::tr("Points"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(false); // NEW: show legend only when overlays are added
    // NEW: style de légende (s'applique quand visible)
    chart->legend()->setBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

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

    // NEW: mémoriser la plage X pour le mapping (utilisée par la sélection)
    if (!hours.isEmpty()) {
        const double minX = hours.first();
        const double maxX = hours.last();
        chart->setProperty("xMin", minX);
        chart->setProperty("xMax", maxX);
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
    chartView->setMouseTracking(true); // keep
    if (chartView->viewport()) chartView->viewport()->setMouseTracking(true); // NEW

    QGraphicsProxyWidget *proxy = scene->addWidget(chartView);
    proxy->setPos(0, 0);
    proxy->setGeometry(vpRect);

    // Activate selection handler (install on viewport)
    if (!chartView->property("selectionHandler").toBool()) {
        new SelectionHandler(chartView, chart); // parented to chartView
        chartView->setProperty("selectionHandler", true);
    }

    graphPlaceholder->setRenderHint(QPainter::Antialiasing);
    graphPlaceholder->show();
}

// NEW: add an extra line series to the existing chart inside a QGraphicsView
bool Render::addSeriesToExistingChart(QGraphicsView *view, const QString &hoursStr, const QString &pointsStr, const QString &seriesName, int rank)
{
    QChart* chart = chartFromGraphicsView(view);
    if (!chart) return false;

    QList<double> hours = parseJsonArray(hoursStr);
    QList<double> points = parseJsonArray(pointsStr);
    if (hours.size() != points.size() || hours.isEmpty()) return false;

    // Avoid duplicates by name
    for (auto s : chart->series()) {
        if (s->name() == seriesName) return false;
    }

    auto s = new QLineSeries();
    s->setName(seriesName);
    s->setProperty("rank", rank); // NEW: store rank
    for (int i = 0; i < hours.size(); ++i) s->append(hours[i], points[i]);

    chart->addSeries(s);
    // Attach to existing axes
    if (!chart->axes(Qt::Horizontal).isEmpty()) s->attachAxis(chart->axes(Qt::Horizontal).first());
    if (!chart->axes(Qt::Vertical).isEmpty()) s->attachAxis(chart->axes(Qt::Vertical).first());

    chart->legend()->setBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    chart->legend()->setVisible(chart->series().size() > 1);
    return true;
}

// NEW: remove a series by its name (does not remove the base if name matches it is allowed)
bool Render::removeSeriesByName(QGraphicsView *view, const QString &seriesName)
{
    QChart* chart = chartFromGraphicsView(view);
    if (!chart) return false;

    for (auto s : chart->series()) {
        if (s->name() == seriesName) {
            chart->removeSeries(s);
            delete s;
            chart->legend()->setVisible(chart->series().size() > 1);
            return true;
        }
    }
    return false;
}

// NEW: clear all overlays (keep base if it exists and matches baseSeriesName)
void Render::clearAllOverlaySeries(QGraphicsView *view, const QString &baseSeriesName)
{
    QChart* chart = chartFromGraphicsView(view);
    if (!chart) return;

    // Collect to remove: all that are not the base
    QList<QAbstractSeries*> toRemove;
    for (auto s : chart->series()) {
        if (s->name() != baseSeriesName) toRemove.append(s);
    }
    for (auto s : toRemove) {
        chart->removeSeries(s);
        delete s;
    }
    chart->legend()->setVisible(chart->series().size() > 1);
}

// NEW: public wrapper
QChart* Render::chartFromView(QGraphicsView* view)
{
    return chartFromGraphicsView(view);
}
