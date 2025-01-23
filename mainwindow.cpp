#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gameplatform.h"
#include "functb.h"
#include "render.h"
#include "updater.h"
#include "leaderboard.h"
//
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QInputDialog>
//
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QFileDialog>

#include <QActionGroup>

#include <sstream>  // Nécessaire pour std::ostringstream
#include <iostream>
#include <fstream>

#include <QMessageBox>
#include <QDesktopServices>

#include <QVBoxLayout>
#include <QListWidget>

#include <QMainWindow>
#include <QMenuBar>
#include <QStackedWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QtUiTools/QUiLoader>

// Fonction pour capturer la réponse HTTP
/*static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}*/




void sendRequest(Ui::MainWindow *ui) {
    // Création d'un gestionnaire réseau
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    // URL cible
    QUrl url("https://ishin-global.aktsk.com/ping");

    // Préparation de la requête
    QNetworkRequest request(url);
    // std::string ver_code = "5.23.0-eb08f8a58bbdef433e02ac565ae490b0966becf3519dea57a755d440421f5532";
    // request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Platform", "android");
    request.setRawHeader("X-ClientVersion", functb::ver_code.c_str());
    request.setRawHeader("X-Language", "en");
    request.setRawHeader("X-UserID", "////");

    // Optionnel : Ajouter des paramètres JSON pour une requête POST
    QJsonObject jsonBody;
    // jsonBody["param1"] = "valeur1";
    // jsonBody["param2"] = "valeur2";

    // Conversion en QByteArray
    QByteArray bodyData = QJsonDocument(jsonBody).toJson();

    // on accepte http (je crois)
    // request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    // Envoyer une requête GET
    QNetworkReply *reply = manager->get(request, bodyData);
    // ui->boitetext->append("b");

    // Gérer la réponse (asynchrone)
    QObject::connect(reply, &QNetworkReply::finished, [reply, ui, request]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Récupérer la réponse en texte brut
            QByteArray responseBytes = reply->readAll();
            //qDebug() << "Réponse brute:" << responseBytes;
            //ui->boitetext->append("Réponse brute reçue : \"" + QString(responseBytes) + "\"");
            /*// récupérer les headers
            QList<QByteArray> headers = request.rawHeaderList();
            ui->boitetext->append("Headers : ");
            for (int i = 0; i < headers.size(); i++) {
                ui->boitetext->append(headers.at(i) + " : " + reply->rawHeader(headers.at(i)));
            }*/

            // Convertir la réponse en JSON
            QJsonDocument jsonResponse = QJsonDocument::fromJson(responseBytes);
            if (!jsonResponse.isNull()) {
                // qDebug() << "Réponse JSON:" << jsonResponse;
                // ui->boitetext->append("Réponse JSON formatée : " + QString(jsonResponse.toJson(QJsonDocument::Indented)));
            } else {
                // qWarning() << "Erreur lors de la conversion en JSON";
                ui->boitetext->append(QObject::tr("Erreur lors de la conversion en JSON"));
            }
        } else {
            // Gestion des erreurs
            // qWarning() << "Erreur réseau:" << reply->errorString();
            ui->boitetext->append(QObject::tr("Erreur réseau : ") + reply->errorString());
        }

        // test
        QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if (statusCode.isValid()) {
            int httpStatus = statusCode.toInt();
            ui->boitetext->append(QObject::tr("Code de statut HTTP: ") + QString::number(httpStatus));

            if (httpStatus != 200) {
                ui->boitetext->append(QObject::tr("Le serveur a répondu avec un code d'erreur HTTP."));
            }
        } else {
            ui->boitetext->append(QObject::tr("Impossible de récupérer le code de statut HTTP."));
        }

        reply->deleteLater();
    });
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
    menu1_action1(nullptr),
    menu1_action2(nullptr),
    menu1(nullptr),
    labelDynamic(nullptr)
{
    // Ne plus faire ui->setupUi(this) ici,
    // on va charger l'UI nous-mêmes via QUiLoader
    ui->setupUi(this);

    // Mettre en anglais de base
    // loadLanguage("en_US");

    // Créer le menu afin de changer la page (Leaderboard, ...)
    // Créer le QStackedWidget

    std::cout << "v : " << Updater::polar_version << std::endl;
    stackedWidget = new QStackedWidget(this);
    QWidget *pagePrincipale = this->centralWidget();
    // Détacher le widget de la MainWindow
    pagePrincipale->setParent(nullptr);
    setCentralWidget(stackedWidget);
    stackedWidget->addWidget(pagePrincipale);
    // this->resize(1260, 717);

    // Créer les pages
    // QWidget *pagePrincipale = new QWidget(this);
    // std::cout << "a : " << Updater::polar_version << std::endl;
    // {
    //     QFile uiFile(":/mainwindow.ui");
    //     if (uiFile.open(QFile::ReadOnly)) {
    //         QUiLoader loader;
    //         QWidget *uiGraphiques = loader.load(&uiFile, pagePrincipale);
    //         uiFile.close();

    //         // Forcer l'expansion du widget
    //         uiGraphiques->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    //         QVBoxLayout *layout = new QVBoxLayout(pagePrincipale);
    //         layout->setContentsMargins(0, 0, 0, 0);
    //         layout->addWidget(uiGraphiques);
    //         pagePrincipale->setLayout(layout);

    //         // Afficher dans le std::cout ce que contient pagePrincipale
    //         std::cout << "pagePrincipale contains: " << pagePrincipale->children().size() << " children." << std::endl;
    //         for (auto child : pagePrincipale->children()) {
    //             std::cout << "Child: " << child->metaObject()->className() << std::endl;
    //         }

    //         pagePrincipale->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //     }
    //     else {
    //         qDebug() << "Erreur chargement mainwindow.ui : " << uiFile.errorString();
    //     }
    // }
    QWidget *pageSecondaire = new QWidget(this);
    QVBoxLayout *layoutSec = new QVBoxLayout(pageSecondaire);
    // nouveaux éléments

    auto contentLayout = new QHBoxLayout();

    auto playerList = new QListWidget(this);
    playerList->setFixedWidth(300);
    auto refreshButton = new QPushButton(this);
    Leaderboard::graphPlaceholder = new QGraphicsView(this);
    Leaderboard::dataPlaceholder = new QLabel(this);
    //écrire dans dataplaceholder  "refresh"
    refreshButton->setText("REFRESH");
    Leaderboard::dataPlaceholder->setText("");


    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(Leaderboard::graphPlaceholder);
    rightLayout->addWidget(Leaderboard::dataPlaceholder);
    rightLayout->setStretch(0, 2); // GraphPlaceholder takes 1 part of the space
    rightLayout->setStretch(1, 1); // DataPlaceholder takes 2 parts of the space

    // Ajouter les callbacks
    //connect(refreshButton, &QPushButton::clicked, this, Leaderboard::onRefreshClicked);
    connect(refreshButton, &QPushButton::clicked, this, [this, playerList]() {
        Leaderboard::onRefreshClicked(this, playerList);
    });

    contentLayout->addWidget(playerList);
    contentLayout->addLayout(rightLayout);
    layoutSec->addLayout(contentLayout);
    layoutSec->addWidget(refreshButton);

    labelDynamic = new QLabel(tr("Bienvenue sur le leaderboard !"), pageSecondaire);
    layoutSec->addWidget(labelDynamic);
    pageSecondaire->setLayout(layoutSec);
    std::cout << "e : " << Updater::polar_version << std::endl;
    // retrouver la page secondaire par son nom

    // Configurer les pages
    // setupPagePrincipale(pagePrincipale);
    // setupPageSecondaire(pageSecondaire);

    // Ajouter les pages au QStackedWidget
    // stackedWidget->addWidget(pagePrincipale);
    stackedWidget->addWidget(pageSecondaire);

    // Créer le QMenuBar
    QMenuBar *menuBar = this->menuBar();
    QMenu *menuNavigation = menuBar->addMenu(tr("Navigation"));
    // donner un nom pour le retrouver plus tard quand on veut le traduire
    menuNavigation->setObjectName("menuNavigation");

    // Ajouter des actions pour changer de page
    QAction *actionPagePrincipale = new QAction(tr("Graphiques"), this);
    QAction *actionPageSecondaire = new QAction(tr("Classement"), this);
    actionPageSecondaire->setObjectName("pageSecondaire");

    menuNavigation->addAction(actionPagePrincipale);
    menuNavigation->addAction(actionPageSecondaire);

    // Connecter les actions aux slots
    connect(actionPagePrincipale, &QAction::triggered, this, [this]() {
        stackedWidget->setCurrentIndex(0);
    });

    connect(actionPageSecondaire, &QAction::triggered, this, [this]() {
        stackedWidget->setCurrentIndex(1);
    });

    // Menu principal

    /*menu1 = new QMenu;
    menu1 = menuBar()->addMenu("Infos");

    menu1_action1 = new QAction("A propos", this);
    menu1_action2 = new QAction("Aide", this);

    menu1->addAction(menu1_action1);
    menu1->addAction(menu1_action2);
    */
    // appeller le menu
    stackedWidget->setCurrentIndex(0);
    createLanguageMenu();
    this->setWindowTitle("Polar " + QString::fromStdString(Updater::polar_version));

    loadLanguage("en_US");
    // mise a jour
    updater = new Updater(this);
    connect(updater, &Updater::updateAvailable, this, &MainWindow::onUpdateAvailable);
    updater->checkForUpdate();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    sendRequest(ui);
}


void MainWindow::onUpdateAvailable(const QString &latestVersion, const QString &changelog, const QString &downloadUrl)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Mise à jour disponible"));
    msgBox.setText(tr("Une nouvelle version (%1) est disponible.").arg(latestVersion));
    msgBox.setInformativeText(changelog);
    msgBox.setStandardButtons(QMessageBox::Ok);
    QPushButton *downloadButton = msgBox.addButton(tr("Télécharger"), QMessageBox::AcceptRole);

    msgBox.exec();
    if (msgBox.clickedButton() == downloadButton) {
        // Ouvrir le lien de téléchargement
        QDesktopServices::openUrl(QUrl(downloadUrl));
        // quitter
        QApplication::quit();
    }
}

void MainWindow::on_bouton_graphique_clicked()
{
    // Simuler des données pour le graphique
    // QString hours = "[0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5, 2.75, 3.0, 3.25, 3.5, 3.75, 4.0, 4.25, 4.5, 4.75, 5.0, 5.25, 5.5, 5.75, 6.0, 6.25, 6.5, 6.75, 7.0, 7.25, 7.5, 7.75, 8.0, 8.25, 8.5, 8.75, 9.0, 9.25, 9.5, 9.75, 10.0, 10.25, 10.5, 10.75, 11.0, 11.25, 11.5, 11.75, 12.0, 12.25, 12.5, 12.75, 13.0, 13.25, 13.5, 13.75, 14.0, 14.25, 14.5, 14.75, 15.0, 15.25, 15.5, 15.75, 16.0, 16.25, 16.5, 16.75, 17.0, 17.25, 17.5, 17.75, 18.0, 18.25, 18.5, 18.75, 19.0, 19.25, 19.5, 19.75, 20.0, 20.25, 20.5, 20.75, 21.0, 21.25, 21.5, 21.75, 22.0, 22.25, 22.5, 22.75, 23.0, 23.25, 23.5, 23.75, 24.0, 24.25, 24.5, 24.75, 25.0, 25.25, 25.5, 25.75, 26.0, 26.25, 26.5, 26.75, 27.0, 27.25, 27.5, 27.75, 28.0, 28.25, 28.5, 28.75, 29.0, 29.25, 29.5, 29.75, 30.0, 30.25, 30.5, 30.75, 31.0, 31.25, 31.5, 31.75, 32.0, 32.25, 32.5, 32.75, 33.0, 33.25, 33.5, 33.75, 34.0, 34.25, 34.5, 34.75, 35.0, 35.25, 35.5, 35.75, 36.0, 36.25, 36.5, 36.75, 37.0, 37.25, 37.5, 37.75, 38.0, 38.25, 38.5, 38.75, 39.0, 39.5, 39.75, 40.0, 40.25, 40.5, 40.75, 41.0, 41.25, 41.5, 41.75, 42.0, 42.25, 42.5, 42.75, 43.0, 43.25, 43.5, 43.75, 44.0, 44.25, 44.5, 44.75, 45.0, 45.25, 45.5, 45.75, 46.0, 46.25, 46.5, 46.75, 47.0, 47.25, 47.5, 47.75, 48.0, 48.25, 48.5, 48.75, 49.0, 49.25, 49.5, 49.75, 50.0, 50.25, 50.5, 50.75, 51.0, 51.25, 51.5, 51.75, 52.0, 52.25, 52.5, 52.75, 53.0, 53.25, 53.5, 53.75, 54.0, 54.25, 54.75, 55.0, 55.25, 55.5, 55.75, 56.0, 56.25, 56.5, 56.75, 57.0, 57.25, 57.5, 57.75, 58.0, 58.25, 58.5, 58.75, 59.0, 59.25, 59.5, 59.75, 60.0, 60.25, 60.5, 60.75, 61.0, 61.25, 61.5, 61.75, 62.0, 62.25, 62.5, 62.75, 63.0, 63.25, 63.5, 63.75]";
    // QString points = "[941080591.9161111, 954992299.3683333, 954796492.6183333, 968928798.7647221, 968725659.4555554, 960828762.8919444, 959794832.3283333, 960103803.0783333, 959108686.1419444, 959816838.5783333, 958587293.3283333, 956769817.4511111, 956525744.1419444, 956807881.5783333, 956607881.6419444, 954295580.0783333, 953043648.4555554, 952890358.5783333, 952651368.3283333, 951927051.7647221, 952229145.1419444, 951996060.5783333, 950832455.0147221, 950720249.0783333, 951320991.1419444, 951432952.5783333, 999542601.7130555, 1029240519.9333333, 1028836966.9333333, 1029277206.9333334, 1028871452.2888889, 1033396316.0525, 1042248248.3208333, 1041652944.5333333, 1041447120.9749999, 1041536954.8791666, 1041856904.3208333, 1044158942.3288889, 1068659988.1666666, 1068507978.1666666, 1068518587.0, 1068313611.1666666, 1068488670.1666666, 1074279603.6647222, 1094199310.4466667, 1093448944.6477778, 1092772835.8488889, 1092913066.6477778, 1092286172.4466667, 1092414331.6477778, 1091613328.6477778, 1091055021.8488889, 1091075003.6477778, 1090396226.05, 1090017511.05, 1089753221.05, 1089435869.6477778, 1089149797.8488889, 1088554280.05, 1088696762.05, 1088051255.8488889, 1087681893.05, 1087377153.8488889, 1086912837.8488889, 1086929223.6477778, 1086215261.05, 1085535512.8488889, 1085552888.05, 1084923028.8488889, 1084573663.05, 1084412709.05, 1084075187.05, 1083810920.6477778, 1083569612.05, 1083667276.05, 1083306044.05, 1083156911.8488889, 1082553161.05, 1081898550.05, 1081948704.05, 1081269297.8488889, 1080898378.251111, 1088646902.05, 1080160016.251111, 1079845008.8488889, 1079678985.05, 1079248541.05, 1078764408.8488889, 1077711226.8488889, 1077691214.05, 1077178608.05, 1077302976.05, 1076692617.8488889, 1076344115.05, 1076849364.05, 1075540656.05, 1075708382.8488889, 1075314150.05, 1075448712.251111, 1074886507.8488889, 1074226499.6477778, 1074225163.6477778, 1073646359.8488889, 1073838703.05, 1073155905.4466666, 1072523148.6477778, 1072528502.8488889, 1071732816.6477778, 1071091218.6477778, 1070861482.8488889, 1078572655.6477778, 1069893097.6477778, 1069479798.4466667, 1069197816.8488889, 1068534096.6477778, 1068137508.6477778, 1067982549.4466666, 1067370693.8488889, 1067339347.6477778, 1066777108.8488889, 1066848864.4466667, 1066117435.8488889, 1066233026.6477778, 1065554841.6477778, 1065159257.4466666, 1064869946.6477778, 1064226527.6477778, 1064371924.05, 1063727203.4466666, 1063005627.8488889, 1062987165.8488889, 1062214311.8488889, 1061664232.6477778, 1061667082.8488889, 1061125821.8488889, 1061129469.8488889, 1060294704.8488889, 1059808699.8488889, 1059962146.8488889, 1059722490.8488889, 1059491993.6477778, 1059192978.8488889, 1058915008.6477778, 1059177199.8488889, 1058558397.05, 1058715774.6477778, 1058202912.6477778, 1058221411.8488889, 1057711417.8488889, 1057703137.251111, 1057935174.6477778, 1057291440.8488889, 1056927797.6477778, 1056746511.8488889, 1056364596.05, 1056493701.6477778, 1055811983.6477778, 1055945840.8488889, 1054670445.8488889, 1054401797.05, 1053867624.8488889, 1053913478.8488889, 1053384024.8488889, 1052871247.6477778, 1052752160.05, 1051981828.8488889, 1051720944.8488889, 1051461135.8488889, 1050703302.8488889, 1064294627.6477778, 1051447135.8488889, 1067296427.6477778, 1071294627.6477778, 1065918447.7812119, 1085135435.5364616, 1095205229.4200351, 1099527280.932805, 1084398605.374169, 1069477042.8786293, 1050572265.6123109, 1065960045.7355093, 1070271428.2372028, 1079179193.6377938, 1058484184.0832614, 1078379869.9415963, 1092719848.0115724, 1080146537.4812322, 1066399511.081863, 1052894820.0457423, 1044650326.9108752, 1045684799.4867858, 1042838237.1053739, 1034129667.68958, 1038756483.5635549, 1023777359.9790686, 1015265455.8598633, 1009838327.6963152, 1008063839.1493576, 1019562862.1283556, 1007314803.7989633, 1007888346.2204752, 1011614088.8995569, 993261402.7992756, 997534208.8178141, 984387670.5137279, 967261356.5467328, 984628941.8946476, 1002967932.9361858, 1015340438.9667206, 1007405097.3116981, 991192810.7860737, 998497228.8416022, 996106926.0754386, 981047312.7929261, 980858045.6179382, 962590095.01009, 978350405.5991534, 968910497.4855431, 975209279.3809793, 967864435.1488886, 968641360.1081613, 970451180.4487257, 958217845.8227893, 976216420.6407113, 986959987.8416057, 1004310702.6425092, 1020171875.9917282, 1024166868.1934583, 1041449652.7556638, 1024307079.1218885, 1011850802.8739507, 993444317.5603093, 986503333.9461744, 982110524.9515643, 973128104.0541553, 985924252.4146553, 980275037.6430349, 971685260.3725352, 973344746.5597557, 959364565.7902483, 970961248.7992171, 954437455.2666767, 973025580.4175401, 983621625.403961, 971767634.5624377, 952546930.4589881, 964566603.0731025, 972547710.4921353, 981456526.3707174, 992106128.4573482, 975202411.9987965, 969681429.4826318]";

    // Récupérer les données
    QJsonObject data = functb::pologet();

    // Check si ya un "error"
    if (data.contains("error")) {
        QString error = QString::fromStdString(data["error"].toString().toStdString());
        ui->boitetext->append("Error : " + error);
        return;
    }

    // check s'il ya plusieurs utilisateurs (la reponse est donc une liste de json)
    if (data.contains("users") && data["users"].isArray()) {
        QJsonArray jsonArray = data["users"].toArray();
        QStringList userList;

        // affiche dans le std tout le monde

        for (const QJsonValue &value : jsonArray) {
            QJsonObject obj = value.toObject();
            // ajouter
            QString ex = obj["ranks"].toString().remove("[").remove("]");
            QStringList values = ex.split(",");
            QString last_ranks = values.last().trimmed();
            userList.append(obj["name"].toString() + " : " + last_ranks);
            QJsonArray users = obj["users"].toArray();
        }
        bool ok;
        QDialog dialog(this);
        QVBoxLayout layout(&dialog);
        QListWidget listWidget;
        layout.addWidget(&listWidget);

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
            QListWidgetItem *item = new QListWidgetItem(userList[i], &listWidget);
            // std::cout << "userList[i] : " << userList[i].toStdString() << std::endl;
            item->setData(Qt::UserRole, obj);
            i++;
        }

        connect(&listWidget, &QListWidget::itemDoubleClicked, [&](QListWidgetItem *item) {
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            QString ex = user["ranks"].toString().remove("[").remove("]");
            QStringList values = ex.split(",");
            QString last_ranks = values.last().trimmed();
            // ui->boitetext->append("Utilisateur sélectionné : " + last_ranks);
            dialog.accept();
            // "mettre a jour" les données
            data = user;
        });

        dialog.exec();
    }

    QString ydata = "wins_pace";
    // récupérer depuis le champ de texte
    ydata = ui->ydataBox->currentText();

    // Transformer les données QJsonValueRef en std::string
    QString hours = QString::fromStdString(data["hour"].toString().toStdString());
    QString points = QString::fromStdString(data[ydata].toString().toStdString());


    Render::createLineChartInGraphicsView(ui, hours, points);
}


void MainWindow::on_idButton_clicked()
{
    // Pop up demandant un nouvel identifiant
    bool ok;
    QString text = QInputDialog::getText(this, tr("Nouvel identifiant"),
                                         tr("Identifiant:"), QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty()) {
        // mettre l'identifiant dans le champ de texte
        ui->label->setText(tr("Identifiant actuel : ") + text);
        // mettre a jour l'identifiant dans functb::identifier
        functb::identifier = text.toStdString();
    }
}

void MainWindow::createLanguageMenu()
{
    QMenu* languageMenu = menuBar()->addMenu(tr("Langue"));

    QActionGroup* langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    connect(langGroup, &QActionGroup::triggered, this, &MainWindow::slotLanguageChanged);

    // Liste des langues disponibles
    QList<QPair<QString, QString>> languages;
    languages.append(qMakePair(QString("en_US"), QString("English")));
    languages.append(qMakePair(QString("fr_FR"), QString("Français")));

    for (const auto& lang : languages) {
        QAction* action = new QAction(lang.second, this);
        action->setCheckable(true);
        action->setData(lang.first);

        if (lang.first == m_currLang) {
            action->setChecked(true);
        }

        languageMenu->addAction(action);
        langGroup->addAction(action);
    }
}


void switchTranslator(QTranslator& translator, const QString& filename) {
    // remove the old translator
    qApp->removeTranslator(&translator);

    // load the new translator
    QString path = QApplication::applicationDirPath();
    // path.append("/languages/");
    if(translator.load(path + filename)) //Here Path and Filename has to be entered because the system didn't find the QM Files else
        qApp->installTranslator(&translator);
}

void MainWindow::loadLanguage(const QString& locale)
{
    if (m_currLang != locale) {
        m_currLang = locale;
        // std::cout << "selectedLocale : " << locale.toStdString() << std::endl;

        // Supprimez le traducteur existant
        qApp->removeTranslator(&m_translator);

        // Chargez le nouveau fichier de traduction
        QString baseName = "Polar_" + locale;
        if (m_translator.load(":/i18n/" + baseName)) {
            qApp->installTranslator(&m_translator);
        } else {
            qDebug() << QObject::tr("Impossible de charger la traduction pour") << baseName;
        }

        // Retraduisez l'interface utilisateur
        ui->retranslateUi(this);
        this->setWindowTitle("Polar " + QString::fromStdString(Updater::polar_version));
    }
}

// Called every time, when a menu entry of the language menu is called
void MainWindow::slotLanguageChanged(QAction* action)
{
    if (action) {
        QString selectedLocale = action->data().toString();
        loadLanguage(selectedLocale);
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        labelDynamic->setText(tr("Bienvenue sur le leaderboard !"));
    } else {
        QMainWindow::changeEvent(event);
    }
}
