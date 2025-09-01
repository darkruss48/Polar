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

void Leaderboard::onRefreshClicked(MainWindow * this_, QListWidget *playerList)
{
    std::cout << "Refresh clicked" << std::endl;
    // Vider playerList
    playerList->clear();
    // Ajouter un texte test
    // new QListWidgetItem("Test", playerList);
    // Récupérer le ladder
    QJsonObject ladder = functb::pologettop();
    // Afficher le nombre d'éléments dans cette liste
    std::cout << "Number of players: " << ladder["top"].toArray().size() << std::endl;
    std::cout << "Nom premier : " << ladder["top"][0]["name"].toString().toStdString() << std::endl;
    // Check si ya un "error"
    if (ladder.contains("error")) {
        QString error = QString::fromStdString(ladder["error"].toString().toStdString());
        return;
    }

    // check s'il ya plusieurs utilisateurs (la reponse est donc une liste de json)
    if (ladder.contains("top") && ladder["top"].isArray()) {
        QJsonArray jsonArray = ladder["top"].toArray();
        QStringList userList;

        // affiche dans le std tout le monde

        for (const QJsonValue &value : jsonArray) {
            QJsonObject obj = value.toObject();
            // ajouter
            QString ex = obj["ranks"].toString().remove("[").remove("]");
            QStringList values = ex.split(",");
            QString last_ranks = values.last().trimmed();
            QString ex_ = obj["points"].toString().remove("[").remove("]");
            QStringList values_ = ex_.split(",");
            QString last_points = values_.last().trimmed();
            // virgules tous les 3 chiffres
            last_points = QString::number(last_points.toInt()).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,");
            // userList.append(obj["name"].toString() + " : " + last_ranks);
            userList.append("                                   " + last_ranks + " : " + obj["name"].toString()
                            + "\n                            " + last_points + " points"
                            + "\n--------------------------------------------------\n");
            // QJsonArray users = obj["top"].toArray();
        }
        // bool ok;
        std::cout << "Taille de userList : " << userList.size() << std::endl;
        // QDialog dialog(this_);
        // QVBoxLayout layout(&dialog);
        // layout.addWidget(playerList);

        int i=0;
        for (const QJsonValue &value : jsonArray) {
            // std::cout << "azeraz" << std::endl;
            QJsonObject obj = value.toObject();
            // auto user = obj["users"][i];
            // std::cout << "id : " << obj["id"].toString().toStdString() << std::endl;
            /*QString userInfo = QString("%1 ; %2 ; %3")
                       .arg(user["name"].toString())
                       .arg(user["wins_pace"].toString())
                       .arg(user["id"].toString());
            */
            // QListWidgetItem *item = new QListWidgetItem(userInfo, &listWidget);
            QListWidgetItem *item = new QListWidgetItem(userList[i], playerList);
            // std::cout << "userList[i] : " << userList[i].toStdString() << std::endl;
            item->setData(Qt::UserRole, obj);
            i++;
        }

        connect(playerList, &QListWidget::itemDoubleClicked, [&](QListWidgetItem *item) {
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            // "mettre a jour" les données
            // ladder = user;
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
    
}



void Leaderboard::onPlayerDoubleClicked(QListWidgetItem *item)
{
    // Afficher plus de détails du joueur sélectionné
}
