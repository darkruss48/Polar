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

// pointeurs
QGraphicsView *Leaderboard::graphPlaceholder = nullptr;
QLabel *Leaderboard::dataPlaceholder = nullptr;
QLabel *Leaderboard::avgPlaceholder = nullptr; // NEW

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
    refreshButton->setText(tr("Refreshe"));
    // graphPlaceholder->setText(tr("Graph placeholder here"));
    dataPlaceholder->setText(tr("Data placeholder here"));
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
                                      bool isAfk, const QString& statusText) // CHANGED
{
    auto row = new QWidget();
    auto root = new QHBoxLayout(row);
    root->setContentsMargins(12, 8, 22, 8);
    root->setSpacing(12);

    // Left: Rank
    auto lblRank = new QLabel(QString("#%1").arg(rank));
    QFont fRank = lblRank->font();
    fRank.setBold(true);
    fRank.setPointSize(fRank.pointSize() + 3);
    lblRank->setFont(fRank);
    lblRank->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblRank->setMinimumWidth(52);
    root->addWidget(lblRank, 0);

    // Middle: Name + Status
    auto mid = new QVBoxLayout();
    mid->setSpacing(2);
    auto lblName = new QLabel(name);
    QFont fName = lblName->font(); fName.setBold(true); fName.setPointSize(fName.pointSize() + 1);
    lblName->setFont(fName);
    auto lblStatus = new QLabel(statusText);
    lblStatus->setStyleSheet(isAfk ? "color:#E74C3C;" : "color:#2ECC71;");
    mid->addWidget(lblName);
    mid->addWidget(lblStatus);
    root->addLayout(mid, 1);

    // Right: Points + Gap
    auto right = new QVBoxLayout();
    right->setSpacing(2);
    right->setContentsMargins(0, 0, 5, 0); // keep away from scrollbar
    auto lblPts = new QLabel(QString("%1 pts").arg(formatMillions(points)));
    lblPts->setStyleSheet("color:#CFCFCF;");
    lblPts->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto lblGap = new QLabel(gapBelowLabel);
    lblGap->setStyleSheet("color:#A0A0A0;");
    lblGap->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    right->addWidget(lblPts);
    right->addWidget(lblGap);
    root->addLayout(right, 0);

    row->setMinimumHeight(80);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row->setStyleSheet("QWidget { background: transparent; }");
    return row;
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

            // Gap with below only
            QString gapBelowLabel;
            if (i + 1 < rows.size()) {
                qint64 diff = r.lastPoints - rows[i+1].lastPoints;
                if (diff > 0) gapBelowLabel = QString("+%1").arg(formatMillions(diff));
            }

            QWidget* widget = createPlayerRowWidget(r.rank, r.name, r.lastPoints, gapBelowLabel, isAfk, statusText);

            auto* item = new QListWidgetItem(playerList);
            item->setData(Qt::UserRole, r.obj);
            item->setSizeHint(QSize(playerList->viewport()->width(), 80));
            playerList->addItem(item);
            playerList->setItemWidget(item, widget);
        }

        // Rewire: double-click opens graph (ensure single connection)
        QObject::disconnect(playerList, &QListWidget::itemDoubleClicked, nullptr, nullptr);
        QObject::connect(playerList, &QListWidget::itemDoubleClicked, playerList, [this_, playerList](QListWidgetItem* item){
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            Leaderboard::affichergraphiqueettexte(this_, user);
        });
    }


    // Mettre à jour la liste, le graphe, etc.
}

void Leaderboard::affichergraphiqueettexte(MainWindow * this_, QJsonObject user)
{
    // Afficher les données du joueur sélectionné
    QString ex = user["ranks"].toString().remove("[").remove("]");
    QStringList values = ex.split(",");
    QString last_ranks = values.last().trimmed();
    auto name = user["name"].toString();
    std::cout << "Utilisateur sélectionné : " + name.toStdString() << std::endl;

    QString ydata = "wins_pace";

    // Transformer les données QJsonValueRef en std::string
    QString hours = QString::fromStdString(user["hour"].toString().toStdString());
    QString points = QString::fromStdString(user[ydata].toString().toStdString());
    // Afficher le graphe
    const QString a = QString(user["name"].toString());
    Render::render_leaderboard(this_, Leaderboard::graphPlaceholder, hours, points, ydata, a);
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
    // Créer un texte explicatif
    Leaderboard::dataPlaceholder->setText("a");
    std::cout << "Utilisateur sélectionné : " + name.toStdString() << std::endl;
    Leaderboard::dataPlaceholder->setText(tr("Nom : ") + name + "\n" + tr("Rank : ") + last_ranks + "\n"
                                          + "\n" + tr("Wins : ") + last_wins + "\n" + tr("Points totaux : ") + last_points
                                          + "\n" + tr("Heures AFK : ") + QString::number(hoursWithoutPoints));
    
    std::cout << "Utilisateur sélectionné : " + name.toStdString() << std::endl;
    
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

    // Met à jour label_data (AFK formaté XhYmin)
    if (Leaderboard::dataPlaceholder) {
        Leaderboard::dataPlaceholder->setText(
            tr("Nom : ") + name + "\n" +
            tr("Rank : ") + last_ranks + "\n\n" +
            tr("Wins : ") + last_wins + "\n" +
            tr("Points totaux : ") + last_points + "\n" +
            tr("Heures AFK : ") + afkStr
        );
    }

    // Met à jour la zone "Infos Moyenne"
    if (Leaderboard::avgPlaceholder) {
        QString paceLines;
        if (topPace > 0) {
            paceLines = QString("Pace %1 : %2% (%5/%6)\nPace %3 : %4% (%7/%6)")
                            .arg(topPace)
                            .arg(QString::number(pctTop, 'f', 1))
                            .arg(secondPace)
                            .arg(QString::number(pctSecond, 'f', 1))
                            .arg(countTop)
                            .arg(totalActiveSteps)
                            .arg(countSecond);
        } else {
            paceLines = QString("Pace 0 : 0%\nPace -1 : 0%");
        }

        Leaderboard::avgPlaceholder->setText(
            QString("Non-AFK : %1\nAFK : %2\nWins/h (actif) : %3\n%4")
                .arg(activeStr)
                .arg(afkStr)
                .arg(QString::number(avgWinsPerHour, 'f', 2))
                .arg(paceLines)
        );
    }

    std::cout << "Utilisateur sélectionné : " + name.toStdString() << std::endl;
}



void Leaderboard::onPlayerDoubleClicked(QListWidgetItem *item)
{
    // Afficher plus de détails du joueur sélectionné
}
