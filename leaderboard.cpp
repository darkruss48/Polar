#include "leaderboard.h"
#include "functb.h"
#include "qdialog.h"
#include "qjsonarray.h"
#include "qjsonobject.h"
#include <QEvent>
#include <QListWidgetItem>
#include <iostream>
#include <string>
#include "render.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QMap>
#include <QMenu>
#include <QAction>
#include <QSet>
#include <cmath> // NEW: for std::llabs

// pointeurs
QGraphicsView *Leaderboard::graphPlaceholder = nullptr;
QLabel *Leaderboard::dataPlaceholder = nullptr;
QLabel *Leaderboard::avgPlaceholder = nullptr; // NEW
QLabel *Leaderboard::gapPlaceholder = nullptr; // NEW
// NEW: overlay state
QString Leaderboard::baseSeriesName = QString();
QSet<QString> Leaderboard::overlayNames;
// NEW: snapshot
QVector<Leaderboard::SimpleRow> Leaderboard::snapshotRows;

// NEW: two-column placeholders (define statics)
QLabel *Leaderboard::dataLeftPlaceholder = nullptr;
QLabel *Leaderboard::dataRightPlaceholder = nullptr;
QLabel *Leaderboard::avgLeftPlaceholder = nullptr;
QLabel *Leaderboard::avgRightPlaceholder = nullptr;
QLabel *Leaderboard::gapLeftPlaceholder = nullptr;
QLabel *Leaderboard::gapRightPlaceholder = nullptr;

Leaderboard::Leaderboard(QWidget *parent) : QWidget(parent)
{
    // mainLayout = new QVBoxLayout(this);
    // contentLayout = new QHBoxLayout();

    // playerList = new QListWidget(this);
    // refreshButton = new QPushButton(this);
    // // graphPlaceholder = new QLabel(this);
    // dataPlaceholder = new QLabel(this);

    // // Partie gauche : liste de joueurs
    // connect(playerList, &QListWidget::itemDoubleClicked, this, &Leaderboard::onPlayerDoubleClicked);

    // // Partie droite : zone graphique + données
    // QVBoxLayout *rightLayout = new QVBoxLayout();
    // // rightLayout->addWidget(graphPlaceholder);
    // rightLayout->addWidget(dataPlaceholder);

    // // contentLayout->addWidget(playerList);
    // // contentLayout->addLayout(rightLayout);
    // // mainLayout->addLayout(contentLayout);
    // // mainLayout->addWidget(refreshButton);

    // connect(refreshButton, &QPushButton::clicked, this, &Leaderboard::onRefreshClicked);
    // connect(refreshButton, &QPushButton::clicked, this, [this]() {
    //     Leaderboard::onRefreshClicked(this);
    // });
    
    std::cout << "LEADERBOARDc : " << std::endl;
    retranslateUi();
    std::cout << "LEADERBOARDu : " << std::endl;

}

Leaderboard::~Leaderboard() {}

void Leaderboard::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void Leaderboard::retranslateUi()
{
    if (refreshButton) refreshButton->setText(tr("Refreshe"));
    if (dataPlaceholder) dataPlaceholder->setText(tr("Data placeholder here"));
    // setWindowTitle(tr("Leaderboard"));
}

// Helpers (formatted numbers)
static QString formatThousands(qint64 v) {
    QString s = QString::number(v);
    return s.replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,");
}

// Points -> "X.Y M"
static QString formatMillions(qint64 v) {
    double m = static_cast<double>(v) / 1'000'000.0;
    return QString::number(m, 'f', m >= 10.0 ? 1 : 2) + " M";
}

// Format M with sign (e.g. +1.2 M / -0.7 M)
static QString formatMillionsSigned(qint64 v) {
    const qint64 av = std::llabs(v);
    const double m = static_cast<double>(av) / 1'000'000.0;
    const QString sign = (v > 0 ? "+" : (v < 0 ? "-" : ""));
    return QString("%1%2 M").arg(sign).arg(QString::number(m, 'f', m >= 10.0 ? 1 : 2));
}

// NEW: compact signed M for tight cell display (e.g. +1.2M)
static QString formatMillionsSignedCompact(qint64 v) {
    const qint64 av = std::llabs(v);
    const double m = static_cast<double>(av) / 1'000'000.0;
    const QString sign = (v > 0 ? "+" : (v < 0 ? "-" : ""));
    return QString("%1%2M").arg(sign).arg(QString::number(m, 'f', m >= 10.0 ? 1 : 2));
}

// NEW: ETA formatter from hours -> "~XhYmin" or "~Zmin"
static QString formatEtaFromHours(double hours)
{
    if (hours <= 0.0) return QString("~0min");
    const int totalMin = static_cast<int>(std::round(hours * 60.0));
    const int h = totalMin / 60;
    const int m = totalMin % 60;
    if (h == 0) return QString("~%1min").arg(m);
    if (m == 0) return QString("~%1h").arg(h);
    return QString("~%1h%2min").arg(h).arg(m);
}

// NEW: Points/hour on recent window (last K steps, 15min per step)
static double computePointsPerHour(const QStringList& pts, int stepsWindow = 8) {
    if (pts.size() < 2) return 0.0;
    const int iLast = pts.size() - 1;
    const int iStart = qMax(0, iLast - stepsWindow);
    bool okFirst = false, okLast = false;
    const qint64 first = pts.at(iStart).trimmed().toLongLong(&okFirst);
    const qint64 last  = pts.at(iLast).trimmed().toLongLong(&okLast);
    const int steps = iLast - iStart;
    if (steps <= 0 || !okFirst || !okLast) return 0.0;
    const double hours = steps * 0.25; // 15 minutes par step
    return (last - first) / hours;
}

// Helpers: convert step-count (15min each) to "XhYmin" string
static QString formatDurationFromSteps(int steps)
{
    if (steps <= 0) return QString("0h");
    int totalMin = steps * 15;
    int h = totalMin / 60;
    int m = totalMin % 60;
    if (m == 0) return QString::number(h) + "h";
    if (h == 0) return QString::number(m) + "min";
    return QString("%1h%2min").arg(h).arg(m);
}

// trailing stable points count -> steps AFK (15 min per step)
static int computeAfkStepsTrailing(const QStringList& pointsList) {
    if (pointsList.size() < 2) return 0;
    const QString last = pointsList.last().trimmed();
    int cnt = 0;
    for (int i = pointsList.size() - 2; i >= 0; --i) {
        if (pointsList.at(i).trimmed() == last) cnt++;
        else break;
    }
    return cnt;
}

// trailing strictly increasing points -> steps of continuous farm
static int computeFarmStepsTrailing(const QStringList& pointsList) {
    if (pointsList.size() < 2) return 0;
    int cnt = 0;
    for (int i = pointsList.size() - 1; i > 0; --i) {
        qint64 cur = pointsList.at(i).trimmed().toLongLong();
        qint64 prev = pointsList.at(i-1).trimmed().toLongLong();
        if (cur > prev) cnt++;
        else break;
    }
    return cnt;
}

// Build a compact row widget
static QWidget* createPlayerRowWidget(int rank, const QString& name, qint64 points,
                                      const QString& gapBelowLabel,
                                      bool isAfk, const QString& statusText,
                                      const QString& gapDeltaLabel, const QString& gapDeltaColor)
{
    auto row = new QWidget();
    auto root = new QHBoxLayout(row);
    // NEW: tighter layout
    root->setContentsMargins(8, 6, 16, 6);
    root->setSpacing(8);

    // Left: Rank
    auto lblRank = new QLabel(QString("#%1").arg(rank));
    QFont fRank = lblRank->font();
    fRank.setBold(true);
    fRank.setPointSize(fRank.pointSize() + 3);
    lblRank->setFont(fRank);
    lblRank->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblRank->setMinimumWidth(44);
    root->addWidget(lblRank, 0);

    // Middle: Name + Status
    auto mid = new QVBoxLayout();
    mid->setSpacing(2);
    auto lblName = new QLabel(name);
    QFont fName = lblName->font(); fName.setBold(true); fName.setPointSize(fName.pointSize() + 1);
    lblName->setFont(fName);
    auto lblStatus = new QLabel(statusText);
    // NEW: status en plus petit et discret
    lblStatus->setStyleSheet(QString("color:%1; font-size:11px;").arg(isAfk ? "#E74C3C" : "#2ECC71"));
    mid->addWidget(lblName);
    mid->addWidget(lblStatus);
    root->addLayout(mid, 1);

    // Right: Points + Gap (+ colored delta on the left)
    auto right = new QVBoxLayout();
    right->setSpacing(2);
    right->setContentsMargins(0, 0, 5, 0);
    auto lblPts = new QLabel(QString("%1 pts").arg(formatMillions(points)));
    lblPts->setStyleSheet("color:#CFCFCF;");
    lblPts->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // bottom row with colored delta then grey gap
    auto gapRow = new QHBoxLayout();
    gapRow->setContentsMargins(0,0,0,0);
    gapRow->setSpacing(6);

    auto lblDelta = new QLabel(gapDeltaLabel);
    lblDelta->setStyleSheet(QString("color:%1;").arg(gapDeltaColor));
    lblDelta->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblDelta->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    lblDelta->setFixedWidth(62);

    auto lblGap = new QLabel(gapBelowLabel);
    lblGap->setStyleSheet("color:#A0A0A0;");
    lblGap->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblGap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    gapRow->addWidget(lblDelta, 0);
    gapRow->addWidget(lblGap, 1);
    gapRow->setStretch(0, 0);
    gapRow->setStretch(1, 1);

    right->addWidget(lblPts);
    right->addLayout(gapRow);
    root->addLayout(right, 0);

    row->setMinimumHeight(68);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row->setStyleSheet("QWidget { background: transparent; }");
    return row;
}

// NEW: style all info labels (left+right)
static void ensureInfoLabelsStyled()
{
    auto apply = [](QLabel* lbl, bool isLeft){
        if (!lbl) return;
        if (lbl->property("twoColStyled").toBool()) return; // apply once

        lbl->setTextFormat(Qt::PlainText);
        lbl->setWordWrap(false);
        lbl->setAlignment(isLeft ? (Qt::AlignLeft | Qt::AlignTop) : (Qt::AlignRight | Qt::AlignTop));

        // Fixed size (slightly bigger) to avoid cumulative growth
        lbl->setStyleSheet(QString(
            "QLabel {"
            "  font-size:13px;"
            "  padding:2px 4px;"
            "%1"
            "}"
        ).arg(isLeft ? "color:#A8A8A8;" : ""));

        lbl->setMinimumSize(0, 0);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // mark as styled
        lbl->setProperty("twoColStyled", true);
    };
    apply(Leaderboard::dataLeftPlaceholder,  true);
    apply(Leaderboard::dataRightPlaceholder, false);
    apply(Leaderboard::avgLeftPlaceholder,   true);
    apply(Leaderboard::avgRightPlaceholder,  false);
    apply(Leaderboard::gapLeftPlaceholder,   true);
    apply(Leaderboard::gapRightPlaceholder,  false);
}

// NEW: helper to set two columns with same number of lines
static void setTwoColumnText(QLabel* leftLbl, QLabel* rightLbl,
                             QStringList leftLines, QStringList rightLines)
{
    if (!leftLbl || !rightLbl) return;
    const int n = qMax(leftLines.size(), rightLines.size());
    leftLines.reserve(n);
    rightLines.reserve(n);
    while (leftLines.size() < n)  leftLines << "";
    while (rightLines.size() < n) rightLines << "";
    leftLbl->setText(leftLines.join("\n"));
    rightLbl->setText(rightLines.join("\n"));
}

// Séparateur avec espace plus généreux
static QString hr() { return "<div style='border-top:1px solid rgba(255,255,255,0.14); margin:12px 0;'></div>"; }

// 2-col table helper (tuned spacing + largeur col. label fixe)
static QString kvTable(const QList<QPair<QString,QString>>& rows) {
    QString html = "<table style='width:100%; border-collapse:separate; border-spacing:0 6px;'>";
    for (const auto& r : rows) {
        html += QString(
            "<tr>"
              "<td style='color:#A8A8A8; width:52%; padding:2px 8px 2px 0; white-space:nowrap;'>%1</td>"
              "<td style='text-align:right; padding:2px 0 2px 8px;'>%2</td>"
            "</tr>"
        ).arg(r.first.toHtmlEscaped(), r.second.toHtmlEscaped());
    }
    html += "</table>";
    return html;
}

// Indentation utilitaire (décaler à droite un bloc HTML)
static QString indentBlock(const QString& inner, int px = 14) {
    return QString("<div style='margin-left:%1px;'>%2</div>").arg(px).arg(inner);
}

// NEW: builders without headings (titles now provided by QGroupBox)
static QString buildInfoHtml(const QString& name, const QString& rank, const QString& wins, const QString& totalPts, const QString& afk)
{
    return kvTable({
        { QObject::tr("Nom"), name },
        { QObject::tr("Rank"), rank },
        { QObject::tr("Wins"), wins },
        { QObject::tr("Points totaux"), totalPts },
        { QObject::tr("Heures AFK"), afk }
    });
}

// Dernier paramètre: paceHtml déjà formaté (table)
static QString buildAvgHtml(const QString& activeStr, const QString& afkStr, double avgWinsPerHour, const QString& paceHtml)
{
    const QString tbl = kvTable({
        { QObject::tr("Non-AFK"), activeStr },
        { QObject::tr("AFK"), afkStr },
        { QObject::tr("Wins/h (actif)"), QString::number(avgWinsPerHour, 'f', 2) }
    });
    const QString paceBlock = paceHtml.isEmpty()
        ? QString()
        : QString("<div style='margin-top:8px;'>%1</div>").arg(paceHtml);
    return tbl + paceBlock;
}

void Leaderboard::onRefreshClicked(MainWindow * this_, QListWidget *playerList)
{
    std::cout << "Refresh clicked" << std::endl;
    playerList->clear();

    // Ensure custom row sizes are respected and compact
    playerList->setUniformItemSizes(false);
    playerList->setSpacing(3);
    playerList->setStyleSheet("");
    playerList->setSelectionMode(QAbstractItemView::SingleSelection);

    // Récupérer le ladder
    QJsonObject ladder = functb::pologettop();
    if (ladder.contains("error")) {
        QString error = QString::fromStdString(ladder["error"].toString().toStdString());
        return;
    }

    if (ladder.contains("top") && ladder["top"].isArray()) {
        QJsonArray jsonArray = ladder["top"].toArray();

        struct RowData { int rank; QString name; qint64 lastPoints; QStringList pointsList; QJsonObject obj; };
        QVector<RowData> rows; rows.reserve(jsonArray.size());

        for (const QJsonValue &value : jsonArray) {
            QJsonObject obj = value.toObject();

            const QString ranksStr = obj["ranks"].toString().remove("[").remove("]");
            const QStringList ranksVals = ranksStr.split(",", Qt::SkipEmptyParts);
            int rank = ranksVals.isEmpty() ? 0 : ranksVals.last().trimmed().toInt();

            const QString name = obj["name"].toString();

            const QString pointsStr = obj["points"].toString().remove("[").remove("]");
            const QStringList pointsList = pointsStr.split(",", Qt::SkipEmptyParts);

            qint64 lastPoints = 0;
            if (!pointsList.isEmpty()) {
                bool ok = false;
                lastPoints = pointsList.last().trimmed().toLongLong(&ok);
                if (!ok) lastPoints = 0;
            }

            rows.push_back({rank, name, lastPoints, pointsList, obj});
        }

        std::sort(rows.begin(), rows.end(), [](const RowData& a, const RowData& b){ return a.rank < b.rank; });

        // NEW: fill snapshot
        snapshotRows.clear();
        snapshotRows.reserve(rows.size());

        for (int i = 0; i < rows.size(); ++i) {
            const auto& r = rows[i];

            // trailing status
            const int afkStepsTrail = computeAfkStepsTrailing(r.pointsList);
            const int farmStepsTrail = computeFarmStepsTrailing(r.pointsList);
            const bool isAfk = (afkStepsTrail > 0);
            const int statusSteps = isAfk ? afkStepsTrail : farmStepsTrail;
            const QString statusText = isAfk
                ? QString("AFK %1").arg(formatDurationFromSteps(statusSteps))
                : QString("Farm %1").arg(formatDurationFromSteps(statusSteps));

            // Gap with below only (current - below)
            QString gapBelowLabel;
            qint64 gapBelow = 0;
            if (i + 1 < rows.size()) {
                gapBelow = r.lastPoints - rows[i+1].lastPoints;
                if (gapBelow > 0) gapBelowLabel = QString("+%1").arg(formatMillions(gapBelow));
            }

            // NEW: delta gap (last step) with compact display and color
            QString gapDeltaLabel = "0";
            QString gapDeltaColor = "#A0A0A0";
            if (i + 1 < rows.size()) {
                const auto& b = rows[i+1];
                auto prevVal = [&](const QStringList& lst)->qint64 {
                    if (lst.size() >= 2) return lst.at(lst.size()-2).trimmed().toLongLong();
                    return lst.isEmpty() ? 0 : lst.last().trimmed().toLongLong();
                };
                const qint64 prevA = prevVal(r.pointsList);
                const qint64 prevB = prevVal(b.pointsList);
                const qint64 prevGap = prevA - prevB;
                const qint64 deltaGap = gapBelow - prevGap;
                gapDeltaLabel = formatMillionsSignedCompact(deltaGap); // NEW
                if (deltaGap > 0) gapDeltaColor = "#2ECC71"; // green = on creuse
                else if (deltaGap < 0) gapDeltaColor = "#E74C3C"; // red = on perd
            }

            QWidget* widget = createPlayerRowWidget(
                r.rank, r.name, r.lastPoints, gapBelowLabel, isAfk, statusText,
                gapDeltaLabel, gapDeltaColor
            );

            auto* item = new QListWidgetItem(playerList);
            item->setData(Qt::UserRole, r.obj);
            item->setSizeHint(QSize(playerList->viewport()->width(), 80));
            playerList->addItem(item);
            playerList->setItemWidget(item, widget);

            // snapshot item
            snapshotRows.push_back({r.rank, r.name, r.lastPoints, r.pointsList});
        }

        // Fix: disconnect only what we rewire (avoid breaking other handlers)
        QObject::disconnect(playerList, &QListWidget::itemDoubleClicked, nullptr, nullptr);
        QObject::disconnect(playerList, &QListWidget::customContextMenuRequested, nullptr, nullptr);

        QObject::connect(playerList, &QListWidget::itemDoubleClicked, playerList, [this_, playerList](QListWidgetItem* item){
            if (!item) return;
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            Leaderboard::affichergraphiqueettexte(this_, user);
        });

        // Right-click context menu on the list to add overlay series
        playerList->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(playerList, &QListWidget::customContextMenuRequested, playerList,
                         [this_, playerList](const QPoint& pos){
            QListWidgetItem* item = playerList->itemAt(pos);
            if (!item) return;
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            const QString name = user["name"].toString();
            const QString hoursStr = user["hour"].toString();
            const QString paceStr  = user["wins_pace"].toString();

            QMenu menu(playerList);
            QAction* actAdd = menu.addAction(QString("Rajouter %1 au graphique").arg(name));

            const bool alreadyAdded = Leaderboard::overlayNames.contains(name);
            const bool isBase = (!Leaderboard::baseSeriesName.isEmpty() && Leaderboard::baseSeriesName == name);
            if ((Leaderboard::overlayNames.size() >= 4 && !alreadyAdded) || isBase || name.isEmpty()) {
                actAdd->setEnabled(false);
            }

            QObject::connect(actAdd, &QAction::triggered, playerList, [this_, name, hoursStr, paceStr, user](){
                // If no chart yet, make this selection the base
                QChart* chart = Render::chartFromView(Leaderboard::graphPlaceholder);
                // Parse rank from 'user'
                int r = 0;
                const QString rs = user["ranks"].toString().remove("[").remove("]");
                const QStringList rv = rs.split(",", Qt::SkipEmptyParts);
                if (!rv.isEmpty()) r = rv.last().trimmed().toInt();

                if (!chart) {
                    QJsonObject u; u["name"] = name; u["hour"] = hoursStr; u["wins_pace"] = paceStr; u["ranks"] = user["ranks"];
                    Leaderboard::affichergraphiqueettexte(this_, u);
                    return;
                }
                // Otherwise, add as overlay (with rank)
                if (Render::addSeriesToExistingChart(Leaderboard::graphPlaceholder, hoursStr, paceStr, name, r)) {
                    Leaderboard::overlayNames.insert(name);
                }
            });

            menu.exec(playerList->mapToGlobal(pos));
        });

        // Chart right-click to remove overlays
        if (Leaderboard::graphPlaceholder) {
            QObject::disconnect(Leaderboard::graphPlaceholder, &QWidget::customContextMenuRequested, nullptr, nullptr);
            Leaderboard::graphPlaceholder->setContextMenuPolicy(Qt::CustomContextMenu);
            QObject::connect(Leaderboard::graphPlaceholder, &QWidget::customContextMenuRequested,
                             Leaderboard::graphPlaceholder, [this_](const QPoint& pos){
                if (!Leaderboard::graphPlaceholder) return;
                QMenu menu(Leaderboard::graphPlaceholder);
                if (Leaderboard::overlayNames.isEmpty()) {
                    QAction* none = menu.addAction("Aucun joueur ajouté");
                    none->setEnabled(false);
                } else {
                    for (const QString& name : std::as_const(Leaderboard::overlayNames)) {
                        QAction* act = menu.addAction(QString("Supprimer %1").arg(name));
                        QObject::connect(act, &QAction::triggered, Leaderboard::graphPlaceholder, [name](){
                            if (Render::removeSeriesByName(Leaderboard::graphPlaceholder, name)) {
                                Leaderboard::overlayNames.remove(name);
                            }
                        });
                    }
                    menu.addSeparator();
                    QAction* actClear = menu.addAction("Supprimer tout le monde");
                    QObject::connect(actClear, &QAction::triggered, Leaderboard::graphPlaceholder, [](){
                        Render::clearAllOverlaySeries(Leaderboard::graphPlaceholder, Leaderboard::baseSeriesName);
                        Leaderboard::overlayNames.clear();
                    });
                }
                QPoint globalPos = Leaderboard::graphPlaceholder->viewport()->mapToGlobal(pos);
                menu.exec(globalPos);
            });
        }
    }


    // Mettre à jour la liste, le graphe, etc.
}

void Leaderboard::affichergraphiqueettexte(MainWindow * this_, QJsonObject user)
{
    // Afficher les données du joueur sélectionné
    QString ex = user["ranks"].toString().remove("[").remove("]");
    QStringList values = ex.split(",");
    QString last_ranks = values.last().trimmed();
    int baseRank = last_ranks.toInt();
    auto name = user["name"].toString();
    std::cout << "Utilisateur sélectionné : " + name.toStdString() << std::endl;

    QString ydata = "wins_pace";

    // Transformer les données QJsonValueRef en std::string
    QString hours = QString::fromStdString(user["hour"].toString().toStdString());
    QString points = QString::fromStdString(user[ydata].toString().toStdString());
    // Afficher le graphe
    const QString a = QString(user["name"].toString());
    // NEW: set base series name and clear overlays whenever we render a new base chart
    Leaderboard::baseSeriesName = a;
    Leaderboard::overlayNames.clear();

    // Render + passer le rang
    Render::render_leaderboard(this_, Leaderboard::graphPlaceholder, hours, points, ydata, a, baseRank);
    QString last_points = user["points"].toString().remove("[").remove("]").split(",").last().trimmed();
    last_points = QString::number(last_points.toInt()).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,");
    QString last_wins = user["wins"].toString().remove("[").remove("]").split(",").last().trimmed();
    QString last_hours = user["hour"].toString().remove("[").remove("]").split(",").last().trimmed();
    // Calculer le nombre d'heures où l'utilisateur n'a pas gagné de points
    QStringList pointsList = user["points"].toString().remove("[").remove("]").split(",");
    int zeroPointsCount = 0;
    auto last_point = QString("");
    for (const QString &point : pointsList) {
        if (point.trimmed() == last_point) {
            zeroPointsCount++;
        }
        last_point = point.trimmed();
    }
    double hoursWithoutPoints = zeroPointsCount * 0.25; // 15 minutes = 0.25 heures
    std::cout << "Nombre d'heures sans points : " << hoursWithoutPoints << std::endl;

    // REMOVE legacy single-label writes (dataPlaceholder is nullptr with new UI)
    // Leaderboard::dataPlaceholder->setText("a");
    // std::cout << "Utilisateur sélectionné : " + name.toStdString() << std::endl;
    // Leaderboard::dataPlaceholder->setText(tr("Nom : ") + name + "\n" + tr("Rank : ") + last_ranks + "\n"
    //                                       + "\n" + tr("Wins : ") + last_wins + "\n" + tr("Points totaux : ") + last_points
    //                                       + "\n" + tr("Heures AFK : ") + QString::number(hoursWithoutPoints));

    // Parse lists pour stats
    const QStringList pointsSteps = user["points"].toString().remove("[").remove("]").split(",", Qt::SkipEmptyParts);
    const QStringList winsList    = user["wins"].toString().remove("[").remove("]").split(",", Qt::SkipEmptyParts);
    const QStringList paceList    = user["wins_pace"].toString().remove("[").remove("]").split(",", Qt::SkipEmptyParts);

    int afkSteps = 0, activeSteps = 0;
    qint64 activeWinsGained = 0;
    QMap<int,int> paceCounts; // pace entier -> occurrences (pas AFK)

    for (int i = 1; i < pointsSteps.size(); ++i) {
        qint64 curP  = pointsSteps.at(i).trimmed().toLongLong();
        qint64 prevP = pointsSteps.at(i-1).trimmed().toLongLong();
        if (curP == prevP) {
            afkSteps++;
        } else if (curP > prevP) {
            activeSteps++;
            if (i < winsList.size()) {
                qint64 curW  = winsList.at(i).trimmed().toLongLong();
                qint64 prevW = winsList.at(i-1).trimmed().toLongLong();
                activeWinsGained += qMax<qint64>(0, curW - prevW);
            }
            if (i < paceList.size()) {
                bool ok = false;
                double paceVal = paceList.at(i).trimmed().toDouble(&ok);
                if (ok) {
                    int paceInt = static_cast<int>(paceVal + 1e-9); // pas d'arrondi vers le haut
                    paceCounts[paceInt] += 1;
                }
            }
        }
    }

    const QString afkStr = formatDurationFromSteps(afkSteps);
    const QString activeStr = formatDurationFromSteps(activeSteps);
    const double activeHours = activeSteps * 0.25; // 15 min par pas
    const double avgWinsPerHour = (activeHours > 0.0) ? (activeWinsGained / activeHours) : 0.0;

    // Meilleures paces: max P et P-1
    int topPace = 0;
    for (auto it = paceCounts.constBegin(); it != paceCounts.constEnd(); ++it) {
        if (it.key() > topPace) topPace = it.key();
    }
    const int secondPace = (topPace > 0) ? (topPace - 1) : 0;
    const int countTop = paceCounts.value(topPace, 0);
    const int countSecond = paceCounts.value(secondPace, 0);
    const double totalActiveSteps = qMax(1, activeSteps); // évite /0
    const double pctTop = (countTop * 100.0) / totalActiveSteps;
    const double pctSecond = (countSecond * 100.0) / totalActiveSteps;

    ensureInfoLabelsStyled(); // ensure styles applied

    // INFOS: left labels + right values
    if (Leaderboard::dataLeftPlaceholder && Leaderboard::dataRightPlaceholder) {
        QStringList left = {
            tr("Nom"), tr("Rank"), tr("Wins"), tr("Points totaux"), tr("Heures AFK")
        };
        QStringList right = { name, last_ranks, last_wins, last_points, afkStr };
        setTwoColumnText(Leaderboard::dataLeftPlaceholder, Leaderboard::dataRightPlaceholder, left, right);
    }

    // INFOS MOYENNE: base rows + paces as rows
    if (Leaderboard::avgLeftPlaceholder && Leaderboard::avgRightPlaceholder) {
        QStringList left = { tr("Non-AFK"), tr("AFK"), tr("Wins/h (actif)") };
        QStringList right = { activeStr, afkStr, QString::number(avgWinsPerHour, 'f', 2) };
        if (topPace > 0) {
            left  << QString("Pace %1").arg(topPace)    << QString("Pace %1").arg(secondPace);
            right << QString("%1% (%2/%3)").arg(QString::number(pctTop, 'f', 1)).arg(countTop).arg(int(totalActiveSteps))
                  << QString("%1% (%2/%3)").arg(QString::number(pctSecond, 'f', 1)).arg(countSecond).arg(int(totalActiveSteps));
        }
        setTwoColumnText(Leaderboard::avgLeftPlaceholder, Leaderboard::avgRightPlaceholder, left, right);
    }

    // GAP: au-dessus (devant) + ligne vide + en-dessous (derrière)
    if (Leaderboard::gapLeftPlaceholder && Leaderboard::gapRightPlaceholder && !snapshotRows.isEmpty()) {
        // Rank of selected
        const QString name = user["name"].toString();
        const int rank = [&]{
            QString ex = user["ranks"].toString().remove("[").remove("]");
            const QStringList vals = ex.split(",", Qt::SkipEmptyParts);
            return vals.isEmpty() ? 0 : vals.last().trimmed().toInt();
        }();

        int idx = -1;
        for (int i = 0; i < snapshotRows.size(); ++i) {
            if (snapshotRows[i].rank == rank) { idx = i; break; }
        }

        auto prevVal = [&](const QStringList& lst)->qint64 {
            if (lst.size() >= 2) return lst.at(lst.size()-2).trimmed().toLongLong();
            return lst.isEmpty() ? 0 : lst.last().trimmed().toLongLong();
        };

        auto pph = [&](const QStringList& lst)->double { return computePointsPerHour(lst, 8); };

        QString html;

        if (idx >= 0) {
            const auto& me = snapshotRows[idx];

            QStringList left, right;

            // Above
            if (idx - 1 >= 0) {
                const auto& up = snapshotRows[idx-1];
                const qint64 gapUp = up.lastPoints - me.lastPoints;
                const qint64 prevGapUp = prevVal(up.pointsList) - prevVal(me.pointsList);
                const qint64 dGapUp = gapUp - prevGapUp;
                const double dvUp = computePointsPerHour(me.pointsList, 8) - computePointsPerHour(up.pointsList, 8);
                const QString etaUp = (dvUp > 1e-6 && gapUp > 0) ? formatEtaFromHours(gapUp / dvUp) : QString("—");

                left  << tr("Au-dessus") << tr("Gap") << tr("Rattraper");
                right << QString("#%1 %2").arg(up.rank).arg(up.name)
                      << QString("%1 (%2)").arg(formatMillions(gapUp)).arg(formatMillionsSigned(dGapUp))
                      << etaUp;
            } else {
                left  << tr("Au-dessus");
                right << tr("Aucun");
            }

            // blank separator line
            left  << "";
            right << "";

            // Below
            if (idx + 1 < snapshotRows.size()) {
                const auto& down = snapshotRows[idx+1];
                const qint64 gapDown = me.lastPoints - down.lastPoints;
                const qint64 prevGapDown = prevVal(me.pointsList) - prevVal(down.pointsList);
                const qint64 dGapDown = gapDown - prevGapDown;
                const double dvDown = computePointsPerHour(down.pointsList, 8) - computePointsPerHour(me.pointsList, 8);
                const QString etaDown = (dvDown > 1e-6 && gapDown > 0) ? formatEtaFromHours(gapDown / dvDown) : QString("—");

                left  << tr("En-dessous") << tr("Gap") << tr("Se faire rattraper");
                right << QString("#%1 %2").arg(down.rank).arg(down.name)
                      << QString("%1 (%2)").arg(formatMillions(gapDown)).arg(formatMillionsSigned(dGapDown))
                      << etaDown;
            } else {
                left  << tr("En-dessous");
                right << tr("Aucun");
            }

            setTwoColumnText(Leaderboard::gapLeftPlaceholder, Leaderboard::gapRightPlaceholder, left, right);
        }
    }

    std::cout << "Utilisateur sélectionné : " + name.toStdString() << std::endl;
}



void Leaderboard::onPlayerDoubleClicked(QListWidgetItem *item)
{
    // Afficher plus de détails du joueur sélectionné
}