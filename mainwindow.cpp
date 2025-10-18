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

#include <sstream>
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
#include <QFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QComboBox>
#include "appsettings.h"
#include <QMenu>
#include <QPixmap>
#include <QTransform>
#include <QPalette>
#include <QBrush>
#include <QPainter>
#include <QSlider>
#include <QCheckBox>
#include <QLineEdit>
#include <QTimer>
#include <QDateTime>
#include <QSpinBox>
#include <QClipboard>
#include <QPen> // NEW
#include <QProgressBar>  // NEW: for TB progress bar
#include <algorithm>     // NEW: for std::clamp
#include <QSignalBlocker> // NEW
#include <functional>    // NEW
#include <QLocale>      // NEW: for locale-aware % formatting
#include <QGroupBox>    // NEW: for groupbox titles retranslation
#include <QCoreApplication> // NEW: for QCoreApplication::translate (UI loaded via QUiLoader)

// NEW: lightweight 3-slot edition picker (center = selected, side = neighbors)
class EditionPickerWidget : public QWidget {
public:
    explicit EditionPickerWidget(QWidget* parent=nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setMinimumSize(120, 36); // allow it to shrink; parent/layout decides final size
    }
    void setOnChanged(std::function<void(int)> cb) { onChanged = std::move(cb); }
    void setEditions(const QVector<int>& list, int selectedEdition, int latestEdition) {
        editions = list;
        latest = latestEdition;
        // Map selectedEdition=0 to latest number
        const int desired = (selectedEdition <= 0 ? latest : selectedEdition);
        int idx = editions.indexOf(desired);
        if (idx < 0) idx = qMax(0, editions.size() - 1); // default to smallest
        if (idx != selIdx) { selIdx = idx; update(); }
    }
    QSize sizeHint() const override { return QSize(220, 54); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        // Background
        const QRectF R = rect().adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath bg; bg.addRoundedRect(R, 8, 8);
        p.fillPath(bg, QColor(30,30,30,180));
        p.setPen(QPen(QColor(255,255,255,28), 1));
        p.drawPath(bg);

        // Columns
        const int w = width(), h = height();
        const int colW = w/3;
        const QRect cols[3] = {
            QRect(0, 0, colW, h),
            QRect(colW, 0, colW, h),
            QRect(colW*2, 0, w - colW*2, h)
        };
        // Typography scaled to height
        const int pxSide   = std::max(8,  int(std::round(h * 0.36)));
        const int pxCenter = std::max(10, int(std::round(h * 0.48)));
        const int underH   = std::max(2,  h / 12);
        const int underW   = std::max(10, colW / 3);

        // Values to show
        const QString leftTxt  = (selIdx > 0)                  ? QString::number(editions.at(selIdx-1)) : QString();
        const QString midTxt   = (selIdx >=0 && selIdx<editions.size()) ? QString::number(editions.at(selIdx))   : QString();
        const QString rightTxt = (selIdx+1 < editions.size())  ? QString::number(editions.at(selIdx+1)) : QString();
        const QString labels[3] = { leftTxt, midTxt, rightTxt };

        // Draw texts
        for (int i=0;i<3;++i) {
            const bool center = (i==1);
            QRect r = cols[i];
            // subtle column separators
            if (i!=0) {
                p.setPen(QPen(QColor(255,255,255,20), 1));
                p.drawLine(r.topLeft(), r.bottomLeft());
            }
            if (labels[i].isEmpty()) continue;
            QFont f = font();
            f.setPixelSize(center ? pxCenter : pxSide);
            f.setBold(center);
            p.setFont(f);
            p.setPen(center ? QColor(255,255,255,235) : QColor(220,220,220,160));
            p.drawText(r, Qt::AlignCenter, labels[i]);
            if (center) {
                // highlight under center
                const int uw = underW;
                const int ux = r.center().x() - uw/2;
                p.setPen(Qt::NoPen);
                p.fillRect(QRect(ux, r.bottom() - (underH + 2), uw, underH), QColor(255,255,255,90));
            }
        }
    }
    void mousePressEvent(QMouseEvent* e) override {
        const int x = e->pos().x();
        const int third = width()/3;
        if (x < third) {
            if (selIdx > 0) { selIdx--; changed(); }
        } else if (x >= 2*third) {
            if (selIdx+1 < editions.size()) { selIdx++; changed(); }
        } else {
            // click on center: no-op
        }
    }
private:
    QVector<int> editions;
    int selIdx = -1;
    int latest = 0;
    std::function<void(int)> onChanged;
    void changed() {
        update();
        if (!onChanged || selIdx<0 || selIdx>=editions.size()) return;
        const int edNum = editions.at(selIdx);
        const int stored = (edNum == latest ? 0 : edNum);
        onChanged(stored);
    }
};


QString formatWithCommas(qint64 number) {
    QString numberStr = QString::number(number);
    int len = numberStr.length();

    QString result;
    int count = 0;

    // Parcourir la chaîne de droite à gauche
    for (int i = len - 1; i >= 0; --i) {
        result.prepend(numberStr[i]);
        count++;
        // Ajouter une virgule après chaque 3 chiffres sauf si c'est le dernier groupe
        if (count == 3 && i != 0) {
            result.prepend(',');
            count = 0;
        }
    }
    return result;
}


void MainWindow::formatNumberWithCommas(const QString &text, QString &outFormattedNumber) {
    QString numericString = text;
    numericString.remove(QRegularExpression("[^0-9]"));

    bool ok;
    qint64 number = numericString.toLongLong(&ok);
    if (ok) {
        outFormattedNumber = formatWithCommas(number);
    } else {
        outFormattedNumber = "";
    }
}



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
    , ui(new Ui::MainWindow)
    , menu1_action1(nullptr)
    , menu1_action2(nullptr)
    , menu1(nullptr)
    , labelDynamic(nullptr)
{
    ui->setupUi(this);

    // rendre invisible combo_tb_edition et pushButton
    // if (ui->combo_tb_edition) {
    //     ui->combo_tb_edition->setVisible(false);
    // }
    // if (ui->pushButton) {
    //     ui->pushButton->setVisible(false);
    // }

    functb::setLogBox(ui->boitetext);

    // + Charger l'ID sauvegardé et rafraîchir l'affichage
    if (!AppSettings::savedIdentifier.isEmpty()) {
        functb::identifier = AppSettings::savedIdentifier.toStdString();
    }
    updateIdLabelDisplay();

    // langue sauvegardée dans les paramètres
    loadLanguage(AppSettings::savedLanguage.isEmpty() ? QStringLiteral("en_US")
                                                      : AppSettings::savedLanguage);

    // Stabiliser ydataBox: affecter des clés internes (UserRole) indépendantes de la traduction
    if (ui->ydataBox) {
        auto assignYKeys = [this]() {
            const QString prevKey = ui->ydataBox->currentData(Qt::UserRole).toString();
            const QStringList rawKeys = { "points","wins","ranks","wins_pace","points_wins","points_pace" };
            for (int i = 0; i < ui->ydataBox->count(); ++i) {
                const QString t = ui->ydataBox->itemText(i).trimmed();
                QString l = t.toLower();
                QString key;
                // 1) Si déjà une clé brute
                if (rawKeys.contains(t)) key = t;
                // 2) Heuristiques multilingues
                else if (l.contains("wins_pace") || (l.contains("win") && (l.contains("pace") || l.contains("heure") || l.contains("hour") || l.contains("h/"))))
                    key = "wins_pace";
                else if (l.contains("points_wins") || (l.contains("point") && l.contains("win")))
                    key = "points_wins";
                else if (l.contains("points_pace") || (l.contains("point") && (l.contains("pace") || l.contains("heure") || l.contains("hour"))))
                    key = "points_pace";
                else if (l == "ranks" || l.contains("rank") || l.contains("rang"))
                    key = "ranks";
                else if (l == "wins" || l.contains("victoire"))
                    key = "wins";
                else if (l == "points" || l.contains("points"))
                    key = "points";
                // Affecter
                if (!key.isEmpty())
                    ui->ydataBox->setItemData(i, key, Qt::UserRole);
                else
                    ui->ydataBox->setItemData(i, QVariant(), Qt::UserRole);
            }
            // Restaurer sélection précédente si possible
            if (!prevKey.isEmpty()) {
                for (int i = 0; i < ui->ydataBox->count(); ++i) {
                    if (ui->ydataBox->itemData(i, Qt::UserRole).toString() == prevKey) {
                        ui->ydataBox->setCurrentIndex(i);
                        break;
                    }
                }
            }
        };
        assignYKeys();
    }

    // Que des chiffres dans les goals, et mettre des virgules tous les 3 chiffres
    ui->lineEdit_goal->setValidator( new QIntValidator(0, 10000000000, this) );
    ui->lineEdit_goal->setMaxLength(13);
    ui->lineEdit_afk->setValidator( new QIntValidator(0, 10000000000, this) );
    ui->lineEdit_afk->setMaxLength(2);
    // NEW: Rank target validator + live updates
    if (ui->lineEdit_goal_2) {
        ui->lineEdit_goal_2->setValidator(new QIntValidator(1, 10000, this));
        connect(ui->lineEdit_goal_2, &QLineEdit::textChanged, this, [this]() { updateRankEstimation(); });
    }

    // Connecter le signal pour formatter le nombre avec des virgules
    // connect(ui->lineEdit_goal, &QLineEdit::textChanged, this, &MainWindow::formatNumberWithCommas);
    // connect(ui->lineEdit_afk, &QLineEdit::textChanged, this, &MainWindow::formatNumberWithCommas);

    connect(ui->lineEdit_goal, &QLineEdit::textChanged, [=]() {
        QString formattedNumber;
        formatNumberWithCommas(ui->lineEdit_goal->text(), formattedNumber);
        ui->lineEdit_goal->setText(formattedNumber);
    });
    connect(ui->lineEdit_afk, &QLineEdit::textChanged, [=]() {
        QString formattedNumber;
        formatNumberWithCommas(ui->lineEdit_afk->text(), formattedNumber);
        ui->lineEdit_afk->setText(formattedNumber);
    });

    // pr page rank
    if (ui->lineEdit_afk_2) {
        connect(ui->lineEdit_afk_2, &QLineEdit::textChanged, this, [this](){ updateRankEstimation(); });
    }
    if (ui->combo_afk_minutes_2) {
        connect(ui->combo_afk_minutes_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int){ updateRankEstimation(); });
    }
    // NEW: rank overlay checkbox wiring
    if (ui->checkBox_2) {
        connect(ui->checkBox_2, &QCheckBox::clicked, this, QOverload<bool>::of(&MainWindow::on_checkBox_2_clicked));
    }

    // NEW: recalculer quand les minutes AFK changent
    if (ui->combo_afk_minutes) {
        connect(ui->combo_afk_minutes, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int){ on_lineEdit_afk_textEdited(ui->lineEdit_afk->text()); });
        // Also refresh Rank estimation pace (depends on AFK)
        connect(ui->combo_afk_minutes, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int){ updateRankEstimation(); });
    }
    // Also refresh Rank estimation when AFK hours text changes
    if (ui->lineEdit_afk) {
        connect(ui->lineEdit_afk, &QLineEdit::textChanged, this, [this](){ updateRankEstimation(); });
    }

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

    // Charger la page Classement depuis classement.ui
    QFile classementUi(":/classement.ui");
    QWidget *pageClassement = nullptr;
    if (classementUi.open(QFile::ReadOnly)) {
        QUiLoader loader;
        pageClassement = loader.load(&classementUi, this);
        classementUi.close();
    }
    if (!pageClassement) {
        pageClassement = new QWidget(this);
    }
    stackedWidget->addWidget(pageClassement);

    // Récupérer les widgets de la page Classement
    auto playerList_   = pageClassement->findChild<QListWidget*>("list_players");
    auto refreshButton = pageClassement->findChild<QPushButton*>("button_refresh");
    Leaderboard::graphPlaceholder = pageClassement->findChild<QGraphicsView*>("view_graph");
    // NEW: expose list widget for auto-refresh
    Leaderboard::playerListPtr = playerList_; // NEW

    // NEW: two-column placeholders
    Leaderboard::dataLeftPlaceholder  = pageClassement->findChild<QLabel*>("label_data_left");
    Leaderboard::dataRightPlaceholder = pageClassement->findChild<QLabel*>("label_data_right");
    Leaderboard::avgLeftPlaceholder   = pageClassement->findChild<QLabel*>("label_avg_left");
    Leaderboard::avgRightPlaceholder  = pageClassement->findChild<QLabel*>("label_avg_right");
    Leaderboard::gapLeftPlaceholder   = pageClassement->findChild<QLabel*>("label_gap_left");
    Leaderboard::gapRightPlaceholder  = pageClassement->findChild<QLabel*>("label_gap_right");

    // Legacy single-label pointers not used anymore
    Leaderboard::dataPlaceholder = nullptr;
    Leaderboard::avgPlaceholder  = nullptr;
    Leaderboard::gapPlaceholder  = nullptr;

    labelDynamic = pageClassement->findChild<QLabel*>("labelDynamic");

    if (refreshButton && playerList_) {
        connect(refreshButton, &QPushButton::clicked, this, [this, playerList_]() {
            Leaderboard::onRefreshClicked(this, playerList_);
        });
    }

    // NEW: wire copy button + confirm label + Ctrl+C shortcut
    if (pageClassement) {
        if (auto btnCopy = pageClassement->findChild<QPushButton*>("copy_graph")) {
            connect(btnCopy, &QPushButton::clicked, this, &MainWindow::copyClassementGraphToClipboard);
        }
        confirmCopyLabel = pageClassement->findChild<QLabel*>("confirm_copy");
        if (confirmCopyLabel) {
            confirmCopyLabel->clear();
            confirmCopyLabel->setVisible(false);
            confirmCopyEffect = new QGraphicsOpacityEffect(confirmCopyLabel);
            confirmCopyEffect->setOpacity(0.0);
            confirmCopyLabel->setGraphicsEffect(confirmCopyEffect);
            confirmCopyFade = new QPropertyAnimation(confirmCopyEffect, "opacity", this);
            confirmCopyFade->setDuration(1200);
            confirmCopyHoldTimer = new QTimer(this);
            confirmCopyHoldTimer->setSingleShot(true);
            connect(confirmCopyHoldTimer, &QTimer::timeout, this, [this]() {
                if (confirmCopyFade && confirmCopyEffect) {
                    confirmCopyFade->stop();
                    confirmCopyFade->setStartValue(1.0);
                    confirmCopyFade->setEndValue(0.0);
                    connect(confirmCopyFade, &QPropertyAnimation::finished, this, [this]() {
                        if (confirmCopyLabel) confirmCopyLabel->setVisible(false);
                    }, Qt::SingleShotConnection);
                    confirmCopyFade->start();
                }
            });
        }
        // Ctrl+C shortcut (only acts on Classement page)
        auto actCopy = new QAction(this);
        actCopy->setShortcut(QKeySequence::Copy);
        addAction(actCopy);
        connect(actCopy, &QAction::triggered, this, &MainWindow::copyAnyGraphToClipboard); // CHANGED
    }

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
    // QWidget *pageSecondaire = new QWidget(this);
    // QVBoxLayout *layoutSec = new QVBoxLayout(pageSecondaire);
    // nouveaux éléments

    // auto contentLayout = new QHBoxLayout();

    // auto playerList = new QListWidget(this);
    // playerList->setFixedWidth(300);
    // auto refreshButton = new QPushButton(this);
    // Leaderboard::graphPlaceholder = new QGraphicsView(this);
    // Leaderboard::dataPlaceholder = new QLabel(this);
    //écrire dans dataplaceholder  "refresh"
    // refreshButton->setText("REFRESH");
    // Leaderboard::dataPlaceholder->setText("");


    // QVBoxLayout *rightLayout = new QVBoxLayout();
    // rightLayout->addWidget(Leaderboard::graphPlaceholder);
    // rightLayout->addWidget(Leaderboard::dataPlaceholder);
    // rightLayout->setStretch(0, 2); // GraphPlaceholder takes 1 part of the space
    // rightLayout->setStretch(1, 1); // DataPlaceholder takes 2 parts of the space

    // Ajouter les callbacks
    //connect(refreshButton, &QPushButton::clicked, this, Leaderboard::onRefreshClicked);
    // connect(refreshButton, &QPushButton::clicked, this, [this, playerList]() {
    //     Leaderboard::onRefreshClicked(this, playerList);
    // });

    // contentLayout->addWidget(playerList);
    // contentLayout->addLayout(rightLayout);
    // layoutSec->addLayout(contentLayout);
    // layoutSec->addWidget(refreshButton);

    // labelDynamic = new QLabel(tr("Bienvenue sur le leaderboard !"), pageSecondaire);
    // layoutSec->addWidget(labelDynamic);
    // pageSecondaire->setLayout(layoutSec);
    // std::cout << "e : " << Updater::polar_version << std::endl;
    // retrouver la page secondaire par son nom

    // Configurer les pages
    // setupPagePrincipale(pagePrincipale);
    // setupPageSecondaire(pageSecondaire);

    // Ajouter les pages au QStackedWidget
    // stackedWidget->addWidget(pagePrincipale);
    // stackedWidget->addWidget(pageSecondaire);

    // Créer le QMenuBar
    QMenuBar *menuBar = this->menuBar();
    QMenu *menuNavigation = menuBar->addMenu(tr("Navigation"));
    // donner un nom pour le retrouver plus tard quand on veut le traduire
    menuNavigation->setObjectName("menuNavigation");

    // Ajouter des actions pour changer de page
    QAction *actionPagePrincipale = new QAction(tr("Graphiques"), this);
    actionPagePrincipale->setObjectName("actionGraphiques"); // NEW
    QAction *actionPageSecondaire = new QAction(tr("Classement"), this);
    actionPageSecondaire->setObjectName("pageSecondaire");   // keep

    menuNavigation->addAction(actionPagePrincipale);
    menuNavigation->addAction(actionPageSecondaire);

    // Ajouter des icônes
    actionPagePrincipale->setIcon(QIcon(":/images/chart.png"));
    actionPageSecondaire->setIcon(QIcon(":/images/medal.png"));

    // + Menu Options
    QMenu *menuOptions = menuBar->addMenu(tr("Options"));
    menuOptions->setObjectName("menuOptions");              // NEW
    QAction *actionOptions = new QAction(tr("Paramètres"), this);
    actionOptions->setObjectName("actionOptions");          // NEW
    menuOptions->addAction(actionOptions);
    connect(actionOptions, &QAction::triggered, this, &MainWindow::showOptionsDialog);

    // Ajouter l'icone "settings_icon.png"
    actionOptions->setIcon(QIcon(":/images/settings_icon.png"));

    // Connecter les actions aux slots
    connect(actionPagePrincipale, &QAction::triggered, this, [this]() {
        stackedWidget->setCurrentIndex(0); // mainwindow.ui
    });

    connect(actionPageSecondaire, &QAction::triggered, this, [this]() {
        stackedWidget->setCurrentIndex(1); // classement.ui
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

    // REMOVE: hardcoded English load; language is applied from settings above
    // loadLanguage("en_US");

    // mise a jour
    updater = new Updater(this);
    connect(updater, &Updater::updateAvailable, this, &MainWindow::onUpdateAvailable);
    updater->checkForUpdate();

    // Initialiser l'easter-egg (détection sur label_time_left)
    setupEasterEgg();

    // Démarrer la rotation des tips
    setupTipsRotation();

    // REMPLACE l'ancien code de fond par:
    updateBackgroundPalette();

    // NEW: auto-refresh timer
    autoRefreshTimer = new QTimer(this);
    autoRefreshTimer->setSingleShot(true);
    autoRefreshTimer->setTimerType(Qt::PreciseTimer); // fire as close as possible to scheduled time
    connect(autoRefreshTimer, &QTimer::timeout, this, &MainWindow::doAutoRefreshIfClassement);
    scheduleNextAutoRefresh();

    // NEW: TB widgets explicit + timer
    tbProgressBar = ui->progressBar;
    if (tbProgressBar) {
        tbProgressBar->setRange(0, 1000); // tenths of percent
        tbProgressBar->setTextVisible(true);
        tbProgressBar->setFormat(QStringLiteral("0.0%"));
    }
    tbTitleLabel = ui->label_8;
    if (!tbTimer) {
        tbTimer = new QTimer(this);
        tbTimer->setInterval(30000);
        connect(tbTimer, &QTimer::timeout, this, &MainWindow::updateTbUiFromTimes);
        tbTimer->start();
    }
    // Fetch metadata for selected edition and initialize the UI
    fetchAndInitTbMetadata();
    // Build and wire the custom editions picker
    tbPickerHost = this->findChild<QWidget*>("tbEditionPicker");
    if (tbPickerHost) {
        tbPicker = new EditionPickerWidget(tbPickerHost);
        tbPicker->setGeometry(tbPickerHost->rect()); // fill host
        tbPicker->show();
        // follow host size changes
        tbPickerHost->installEventFilter(this);
        tbPicker->setOnChanged([this](int storedEd){
            AppSettings::selectedEdition = storedEd;
            AppSettings::save();
            fetchAndInitTbMetadata();
        });
    }
    buildTbEditionCombo();

    // NEW: TB widgets autodetect + periodic update
    // 1) Progress bar: pick the first QProgressBar on the main page
    tbProgressBar = ui->progressBar;
    if (tbProgressBar) {
        tbProgressBar->setRange(0, 1000); // tenths of percent
        tbProgressBar->setTextVisible(true);
        tbProgressBar->setFormat(QStringLiteral("0.0%"));
    }
    tbTitleLabel = ui->label_8;

    // 3) Timer to refresh remaining time/progress
    if (!tbTimer) {
        tbTimer = new QTimer(this);
        tbTimer->setInterval(30000); // 30s
        connect(tbTimer, &QTimer::timeout, this, &MainWindow::updateTbUiFromTimes);
        tbTimer->start();
    }
    // 4) Fetch metadata now (edition/start/end) and initialize the UI
    fetchAndInitTbMetadata();
    // NEW: run initial estimation once the UI is ready
    QTimer::singleShot(0, this, [this](){ updateRankEstimation(); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Connecter le redimensionnement de la fenêtre pour ajuster l'image de fond
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateBackgroundPalette();
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
    // QString hours = "[0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5, 2.75, 3.0, 3.25, 3.5, 3.75, 4.0, 4.25, 4.5, 4.75, 5.0, 5.25, 5.5, 5.75, 6.0, 6.25, 6.5, 6.75, 7.0, 7.25, 7.5, 7.75, 8.0, 8.25, 8.5, 8.75, 9.0, 9.25, 9.5, 9.75, 10.0, 10.25, 10.5, 10.75, 11.0, 11.25, 11.5, 11.75, 12.0, 12.25, 12.5, 12.75, 13.0, 13.25, 13.5, 13.75, 14.0, 14.25, 14.5, 14.75, 15.0, 15.25, 15.5, 15.75, 16.0, 16.25, 16.5, 16.75, 17.0, 17.25, 17.5, 17.75, 18.0, 18.25, 18.5, 18.75, 19.0, 19.25, 19.5, 19.75, 20.0, 20.25, 20.5, 20.75, 21.0, 21.25, 21.5, 21.75, 22.0, 22.25, 22.5, 22.75, 23.0, 23.25, 23.5, 23.75, 24.0, 24.25, 24.5, 24.75, 25.0, 25.25, 25.5, 25.75, 26.0, 26.25, 26.5, 26.75, 27.0, 27.25, 27.5, 27.75, 28.0, 28.25, 28.5, 28.75, 29.0, 29.25, 29.5, 29.75, 30.0, 30.25, 30.5, 30.75, 31.0, 31.25, 31.5, 31.75, 32.0, 32.25, 32.5, 32.75, 33.0, 33.25, 33.5, 33.75, 34.0, 34.25, 34.5, 34.75, 35.0, 35.25, 35.5, 35.75, 36.0, 36.25, 36.5, 36.75, 37.0, 37.25, 37.5, 37.75, 38.0, 38.25, 38.5, 38.75, 39.0, 39.25, 39.5, 39.75, 40.0, 40.25, 40.5, 40.75, 41.0, 41.25, 41.5, 41.75, 42.0, 42.25, 42.5, 42.75, 43.0, 43.25, 43.5, 43.75, 44.0, 44.25, 44.5, 44.75, 45.0, 45.25, 45.5, 45.75, 46.0, 46.25, 46.5, 46.75, 47.0, 47.25, 47.5, 47.75, 48.0, 48.25, 48.5, 48.75, 49.0, 49.25, 49.5, 49.75, 50.0, 50.25, 50.5, 50.75, 51.0, 51.25, 51.5, 51.75, 52.0, 52.25, 52.5, 52.75, 53.0, 53.25, 53.5, 53.75, 54.0, 54.25, 54.75, 55.0, 55.25, 55.5, 55.75, 56.0, 56.25, 56.5, 56.75, 57.0, 57.25, 57.5, 57.75, 58.0, 58.25, 58.5, 58.75, 59.0, 59.25, 59.5, 59.75, 60.0, 60.25, 60.5, 60.75, 61.0, 61.25, 61.5, 61.75, 62.0, 62.25, 62.5, 62.75, 63.0, 63.25, 63.5, 63.75]";
    // QString points = "[941080591.9161111, 954992299.3683333, 954796492.6183333, 968928798.7647221, 968725659.4555554, 960828762.8919444, 959794832.3283333, 960103803.0783333, 959108686.1419444, 959816838.5783333, 958587293.3283333, 956769817.4511111, 956525744.1419444, 956807881.5783333, 956607881.6419444, 954295580.0783333, 953043648.4555554, 952890358.5783333, 952651368.3283333, 951927051.7647221, 952229145.1419444, 951996060.5783333, 950832455.0147221, 950720249.0783333, 951320991.1419444, 951432952.5783333, 999542601.7130555, 1029240519.9333333, 1028836966.9333333, 1029277206.9333334, 1028871452.2888889, 1033396316.0525, 1042248248.3208333, 1041652944.5333333, 1041447120.9749999, 1041536954.8791666, 1041856904.3208333, 1044158942.3288889, 1068659988.1666666, 1068507978.1666666, 1068518587.0, 1068313611.1666666, 1068488670.1666666, 1074279603.6647222, 1094199310.4466667, 1093448944.6477778, 1092772835.8488889, 1092913066.6477778, 1092286172.4466667, 1092414331.6477778, 1091613328.6477778, 1091055021.8488889, 1091075003.6477778, 1090396226.05, 1090017511.05, 1089753221.05, 1089435869.6477778, 1089149797.8488889, 1088554280.05, 1088696762.05, 1088051255.8488889, 1087681893.05, 1087377153.8488889, 1086912837.8488889, 1086929223.6477778, 1086215261.05, 1085535512.8488889, 1085552888.05, 1084923028.8488889, 1084573663.05, 1084412709.05, 1084075187.05, 1083810920.6477778, 1083569612.05, 1083667276.05, 1083306044.05, 1083156911.8488889, 1082553161.05, 1081898550.05, 1081948704.05, 1081269297.8488889, 1080898378.251111, 1088646902.05, 1080160016.251111, 1079845008.8488889, 1079678985.05, 1079248541.05, 1078764408.8488889, 1077711226.8488889, 1077691214.05, 1077178608.05, 1077302976.05, 1076692617.8488889, 1076344115.05, 1076849364.05, 1075540656.05, 1075708382.8488889, 1075314150.05, 1075448712.251111, 1074886507.8488889, 1074226499.6477778, 1074225163.6477778, 1073646359.8488889, 1073838703.05, 1073155905.4466666, 1072523148.6477778, 1072528502.8488889, 1071732816.6477778, 1071091218.6477778, 1070861482.8488889, 1078572655.6477778, 1069893097.6477778, 1069479798.4466667, 1069197816.8488889, 1068534096.6477778, 1068137508.6477778, 1067982549.4466666, 1067370693.8488889, 1067339347.6477778, 1066777108.8488889, 1066848864.4466667, 1066117435.8488889, 1066233026.6477778, 1065554841.6477778, 1065159257.4466666, 1064869946.6477778, 1064226527.6477778, 1064371924.05, 1063727203.4466666, 1063005627.8488889, 1062987165.8488889, 1062214311.8488889, 1061664232.6477778, 1061667082.8488889, 1061125821.8488889, 1061129469.8488889, 1060294704.8488889, 1059808699.8488889, 1059962146.8488889, 1059722490.8488889, 1059491993.6477778, 1059192978.8488889, 1058915008.6477778, 1059177199.8488889, 1058558397.05, 1058715774.6477778, 1058202912.6477778, 1058221411.8488889, 1057711417.8488889, 1057703137.251111, 1057935174.6477778, 1057291440.8488889, 1056927797.6477778, 1056746511.8488889, 1056364596.05, 1056493701.6477778, 1055811983.6477778, 1055945840.8488889, 1054670445.8488889, 1054401797.05, 1053867624.8488889, 1053913478.8488889, 1053384024.8488889, 1052871247.6477778, 1052752160.05, 1051981828.8488889, 1051720944.8488889, 1051461135.8488889, 1050703302.8488889, 1064294627.6477778, 1051447135.8488889, 1067296427.6477778, 1071294627.6477778, 1065918447.7812119, 1085135435.5364616, 1095205229.4200351, 1099527280.932805, 1084398605.374169, 1069477042.8786293, 1050572265.6123109, 1065960045.7355093, 1070271428.2372028, 1079179193.6377938, 1058484184.0832614, 1078379869.9415963, 1092719848.0115724, 1080146537.4812322, 1066399511.081863, 1052894820.0457423, 1044650326.9108752, 1045684799.4867858, 1042838237.1053739, 1034129667.68958, 1038756483.5635549, 1023777359.9790686, 1015265455.8598633, 1009838327.6963152, 1008063839.1493576, 1019562862.1283556, 1007314803.7989633, 1007888346.2204752, 1011614088.8995569, 993261402.7992756, 997534208.8178141, 984387670.5137279, 967261356.5467328, 984628941.8946476, 1002967932.9361858, 1015340438.9667206, 1007405097.3116981, 991192810.7860737, 998497228.8416022, 996106926.0754386, 981047312.7929261, 980858045.6179382, 962590095.01009, 978350405.5991534, 968910497.4855431, 975209279.3809793, 967864435.1488886, 968641360.1081613, 970451180.4487257, 958217845.8227893, 976216420.6407113, 986959987.8416057, 1004310702.6425092, 1020171875.9917282, 1024166868.1934583, 1041449652.7556638, 1024307079.1218885, 1011850802.8739507, 993444317.5603093, 986503333.9461744, 982110524.9515643, 973128104.0541553, 985924252.4146553, 980275037.6430349, 971685260.3725352, 973344746.5597557, 959364565.7902483, 970961248.7992171, 954437455.2666767, 973025580.4175401, 983621625.403961, 971767634.5624377, 952546930.4589881, 964566603.0731025, 972547710.4921353, 981456526.3707174, 992106128.4573482, 975202411.9987965, 969681429.4826318]";

    // Récupérer les données
    QJsonObject data = functb::pologet(AppSettings::selectedEdition);

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

    // Clé interne stable (résolue via traductions)
    QString ydata;
    if (ui->ydataBox) {
        // Table canonique -> texte localisé (contexte "MainWindow")
        const QStringList keys = { "wins_pace","points","points_pace","points_wins","ranks","wins" };
        QStringList locs; locs.reserve(keys.size());
        for (const QString& k : keys) {
            locs << QCoreApplication::translate("MainWindow", k.toUtf8().constData());
        }
        auto resolveKey = [&](const QString& txt)->QString {
            const QString tl = txt.trimmed().toLower();
            for (int i = 0; i < keys.size(); ++i) {
                const QString loc = locs.at(i).trimmed();
                if (tl == loc.toLower() || tl == keys.at(i)) return keys.at(i);
            }
            return QString();
        };
        // (1) Ré-attacher UserRole à tous les items selon la table de traduction
        for (int i = 0; i < ui->ydataBox->count(); ++i) {
            const QString itemTxt = ui->ydataBox->itemText(i);
            const QString k = resolveKey(itemTxt);
            if (!k.isEmpty()) ui->ydataBox->setItemData(i, k, Qt::UserRole);
        }
        // (2) Obtenir la clé sélectionnée (UserRole ou via texte localisé)
        ydata = ui->ydataBox->currentData(Qt::UserRole).toString();
        if (ydata.isEmpty()) ydata = resolveKey(ui->ydataBox->currentText());
        // (3) Valider contre le JSON et corriger si besoin
        auto isValidKey = [&data](const QString& k)->bool {
            if (k.isEmpty() || !data.contains(k)) return false;
            return !data.value(k).toString().isEmpty();
        };
        if (!isValidKey(ydata)) {
            // ordre de préférence sur clés existantes
            const QStringList prefer = { "points","wins","wins_pace","points_pace","points_wins","ranks" };
            for (const QString& k : prefer) { if (isValidKey(k)) { ydata = k; break; } }
            if (!isValidKey(ydata)) {
                for (const QString& k : data.keys()) { if (isValidKey(k)) { ydata = k; break; } }
            }
        }
        if (ydata.isEmpty()) ydata = QStringLiteral("wins_pace");
    } else {
        ydata = QStringLiteral("wins_pace");
    }

    // Transformer les données QJsonValueRef en std::string
    QString hours = QString::fromStdString(data["hour"].toString().toStdString());
    QString points = QString::fromStdString(data[ydata].toString().toStdString());

    Render::createLineChartInGraphicsView(ui, hours, points, ydata);

    // Variables
    functb::points = points.toStdString();

    // Wins
    QString wins_ex = data["wins"].toString().remove("[").remove("]");
    QStringList wins_values = wins_ex.split(",");
    functb::wins = wins_values.last().trimmed().toStdString();

    // Seed
    QString seed_ex = data["points_wins"].toString().remove("[").remove("]");
    QStringList seed_values = seed_ex.split(",");
    functb::seed = seed_values.last().trimmed().toStdString();

    // Points
    QString points_ex = data["points"].toString().remove("[").remove("]");
    QStringList points_values = points_ex.split(",");
    functb::points = points_values.last().trimmed().toStdString();

    // Hour
    QString hour_ex = data["hour"].toString().remove("[").remove("]");
    QStringList hour_values = hour_ex.split(",");
    double hourValue = std::stod(hour_values.last().trimmed().toStdString());
    hourValue = 71.51 - hourValue;
    functb::hour_missing = std::to_string(hourValue);

    // on reset le "goal"
    MainWindow::on_lineEdit_afk_textEdited(ui->lineEdit_afk->text());

    // NEW: (re)apply goal overlay if needed on newly created chart
    updateGoalOverlayOnGraphs(true); // NEW
}


void MainWindow::on_idButton_clicked()
{
    // Pop up demandant un nouvel identifiant
    bool ok;
    QString text = QInputDialog::getText(this, tr("Nouvel identifiant"),
                                         tr("Identifiant:"), QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty()) {
        // Persist identifier (real value)
        AppSettings::savedIdentifier = text;
        AppSettings::save();
        // Update runtime value used by code
        functb::identifier = text.toStdString();
        // Update label display honoring censor setting
        updateIdLabelDisplay();
    }
}

void MainWindow::createLanguageMenu()
{
    QMenu* languageMenu = menuBar()->addMenu(tr("Langue"));
    languageMenu->setObjectName("menuLangue"); // NEW: for retranslation

    QActionGroup* langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    connect(langGroup, &QActionGroup::triggered, this, &MainWindow::slotLanguageChanged);

    // Liste des langues disponibles
    QList<QPair<QString, QString>> languages;
    languages.append(qMakePair(QString("en_US"), QString("English")));
    languages.append(qMakePair(QString("fr_FR"), QString("Français")));
    languages.append(qMakePair(QString("es_ES"), QString("Español")));
    languages.append(qMakePair(QString("it_IT"), QString("Italiano")));
    for (const auto& lang : languages) {
        QAction* action = new QAction(this);
        action->setCheckable(true);
        action->setData(lang.first);

        if (lang.first == m_currLang) {
            action->setChecked(true);
        }

        // Charger l'icône du drapeau depuis les ressources intégrées
        QString flagResourcePath = ":/images/" + lang.first + ".png";
        QIcon flagIcon;
        if (QFile::exists(flagResourcePath)) {
            flagIcon.addFile(flagResourcePath);
        } else {
            qDebug() << "Flag icon not found for language:" << lang.first << ", resource path:" << flagResourcePath;
        }

        // Configurer l'action avec l'icône et le texte
        // languageMenu->setStyleSheet("QIcon { outline: 0; }"); // marche pas mdrr
        action->setIcon(flagIcon);
        action->setText(lang.second);

        languageMenu->addAction(action);
        langGroup->addAction(action);
    }

    // Ajouter un keybind : quand j'appuie sur "Tab", on switch entre le "classement" et le "graphique"
    QAction* switchAction = new QAction(this);
    switchAction->setShortcut(QKeySequence(Qt::Key_Tab));
    addAction(switchAction);

    connect(switchAction, &QAction::triggered, this, [this]() {
        int currentIndex = stackedWidget->currentIndex();
        int nextIndex = (currentIndex + 1) % stackedWidget->count();
        stackedWidget->setCurrentIndex(nextIndex);
    });
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
        // NEW: persist language
        AppSettings::savedLanguage = selectedLocale;
        AppSettings::save();
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        // Sauver la clé interne sélectionnée AVANT la retraduction (retranslateUi remplace les items)
        QString prevYKey;
        if (ui->ydataBox) {
            prevYKey = ui->ydataBox->currentData(Qt::UserRole).toString();
            // Si vide, essayer d'utiliser le texte brut (utile si items = clés brutes)
            if (prevYKey.isEmpty()) prevYKey = ui->ydataBox->currentText().trimmed().toLower();
        }

        ui->retranslateUi(this);
        if (labelDynamic) {
            labelDynamic->setText(tr("Bienvenue sur le leaderboard !"));
        }
        // Refresh ID label with new translation prefix
        updateIdLabelDisplay();
        // Rebuild TB localized texts from cached metadata (no network)
        refreshTbLocalizedTexts();
        // NEW: Retraduire les menus et actions existants
        if (menuBar()) {
            if (auto nav = menuBar()->findChild<QMenu*>("menuNavigation")) {
                nav->setTitle(tr("Navigation"));
                if (auto actGraphs = nav->findChild<QAction*>("actionGraphiques")) {
                    actGraphs->setText(tr("Graphiques"));
                }
                if (auto actClassement = nav->findChild<QAction*>("pageSecondaire")) {
                    actClassement->setText(tr("Classement"));
                }
            }
            if (auto lang = menuBar()->findChild<QMenu*>("menuLangue")) {
                lang->setTitle(tr("Langue"));
            }
            if (auto opts = menuBar()->findChild<QMenu*>("menuOptions")) {
                opts->setTitle(tr("Options"));
                if (auto actOpts = opts->findChild<QAction*>("actionOptions")) {
                    actOpts->setText(tr("Paramètres"));
                }
            }
        }

        // NEW: Retraduire les widgets de la page Classement (chargée via QUiLoader)
        QWidget* pageClassement = (stackedWidget && stackedWidget->count() > 1)
                                  ? stackedWidget->widget(1) : nullptr;
        if (pageClassement) {
            // Use the ClassementPage context so translations from classement.ui are applied
            if (auto g = pageClassement->findChild<QGroupBox*>("group_infos"))
                g->setTitle(QCoreApplication::translate("ClassementPage", "Infos"));
            if (auto g = pageClassement->findChild<QGroupBox*>("group_avg"))
                g->setTitle(QCoreApplication::translate("ClassementPage", "Infos Moyenne"));
            if (auto g = pageClassement->findChild<QGroupBox*>("group_gap"))
                g->setTitle(QCoreApplication::translate("ClassementPage", "Gap"));
            if (auto b = pageClassement->findChild<QPushButton*>("button_refresh"))
                b->setText(QCoreApplication::translate("ClassementPage", "REFRESH"));
            if (auto b = pageClassement->findChild<QPushButton*>("copy_graph"))
                b->setText(QCoreApplication::translate("ClassementPage", "Copier le graphique"));
        }

        // NEW: Retraduire les "tips" SANS redémarrer le cycle ni réinitialiser le timing
        if (ui->label_tips) {
            // Mémoriser l'index courant (sélection active) et le texte actuel
            const int curIdx = lastTipIndex;
            // Reconstituer la liste localisée (mêmes entrées que dans setupTipsRotation)
            QStringList newTips = {
                tr("Ne lâche rien !"),
                tr("Tu peux accomplir tes objectifs !"),
                tr("Il est normal d'être fatigué, mais je crois en toi !"),
                tr("Personne ne peut le faire à ta place,\nalors tu vas me le gravir ce classement !"),
                tr("Tu peux le faire !"),
                tr("Prouve-nous que tu es meilleur que ce qu'on peut penser !"),
                tr("Tout le monde est passé par là, ne te décourage pas !"),
                tr("C'est pas le moment de se décourager !"),
                tr("Pense à ceux qui croient en toi ... Tu ne\npeux PAS les décevoir !"),
                // Conseils
                tr("Si tu es fatigué, tu peux prendre une pause\navant la nuit. Ça t'évitera de tomber de fatigue 😉"),
                tr("Ne néglige pas la douche.\nL'hygiène avant tout ... non ?"),
                tr("Il vaudrait mieux que tu aies préparé de quoi\nmanger avant de commencer le tournoi."),
                tr("Se concentrer sur le tournoi est important, mais\navoir un autre centre d'attention en a déjà aidé plus d'un."),
                tr("Fatigué pendant la nuit ? Marcher, boire de l'eau et se rafraîchir\naident à lutter temporairement contre la fatigue."),
                tr("La nuit est souvent dure à passer, mais le matin peut te\nsurprendre. Fais attention."),
                // Applications
                tr("Tu peux regarder le classement en cliquant sur l'onglet\nNavigation, puis sur \"Classement\"."),
                tr("Un objectif en tête ? Tu peux calculer le nombre de\nvictoires/heures à gauche de cette fenêtre."),
                tr("Tu peux générer les graphiques de plusieurs statistiques : \nRang, Points, Points/heure, ..."),
                tr("La touche \"Tab\" te permet de rapidement changer de page. Essaye donc !"),
                // Questions
                tr("Team Café, Team Boisson énergisante ou Team Eau ?"),
                // Bref...
                QStringLiteral("\nYou can't fall asleep if you have to piss\" - Lotad")
            };
            tipsPhrases = newTips;
            // Appliquer la traduction du tip affiché actuellement (même index), sans toucher aux animations
            if (curIdx >= 0 && curIdx < tipsPhrases.size()) {
                ui->label_tips->setText(tipsPhrases.at(curIdx));
            } else if (!tipsPhrases.isEmpty()) {
                ui->label_tips->setText(tipsPhrases.first());
                lastTipIndex = 0;
            }
            // Ne pas appeler restartTipsCycle() ni modifier tipsGroup/tipsEffect => le timing reste identique
        }
            
        // Ré-appliquer les clés stables via traductions et restaurer la sélection
        if (ui->ydataBox) {
            const QStringList keys = { "wins_pace","points","points_pace","points_wins","ranks","wins" };
            QStringList locs; locs.reserve(keys.size());
            for (const QString& k : keys) locs << QCoreApplication::translate("MainWindow", k.toUtf8().constData());
            auto resolveKey = [&](const QString& txt)->QString {
                const QString tl = txt.trimmed().toLower();
                for (int i = 0; i < keys.size(); ++i) {
                    const QString loc = locs.at(i).trimmed();
                    if (tl == loc.toLower() || tl == keys.at(i)) return keys.at(i);
                }
                return QString();
            };
            for (int i = 0; i < ui->ydataBox->count(); ++i) {
                const QString k = resolveKey(ui->ydataBox->itemText(i));
                if (!k.isEmpty()) ui->ydataBox->setItemData(i, k, Qt::UserRole);
            }
            if (!prevYKey.isEmpty()) {
                for (int i = 0; i < ui->ydataBox->count(); ++i) {
                    if (ui->ydataBox->itemData(i, Qt::UserRole).toString() == prevYKey) {
                        ui->ydataBox->setCurrentIndex(i);
                        break;
                    }
                }
            }
        }
    } else {
        QMainWindow::changeEvent(event);
    }
}

// Tips rotation implementation
void MainWindow::setupTipsRotation()
{
    // If the UI label doesn't exist, do nothing.
    if (!ui->label_tips) return;

    // Configure label for better multiline display
    ui->label_tips->setWordWrap(true);
    ui->label_tips->setAlignment(Qt::AlignCenter);

    // Fill phrases (merged categories)
    tipsPhrases = {
        tr("Ne lâche rien !"),
        tr("Tu peux accomplir tes objectifs !"),
        tr("Il est normal d'être fatigué, mais je crois en toi !"),
        tr("Personne ne peut le faire à ta place,\nalors tu vas me le gravir ce classement !"),
        tr("Tu peux le faire !"),
        tr("Prouve-nous que tu es meilleur que ce qu'on peut penser !"),
        tr("Tout le monde est passé par là, ne te décourage pas !"),
        tr("C'est pas le moment de se décourager !"),
        tr("Pense à ceux qui croient en toi ... Tu ne\npeux PAS les décevoir !"),
        // Conseils
        tr("Si tu es fatigué, tu peux prendre une pause\navant la nuit. Ça t'évitera de tomber de fatigue 😉"),
        tr("Ne néglige pas la douche.\nL'hygiène avant tout ... non ?"),
        tr("Il vaudrait mieux que tu aies préparé de quoi\nmanger avant de commencer le tournoi."),
        tr("Se concentrer sur le tournoi est important, mais\navoir un autre centre d'attention en a déjà aidé plus d'un."),
        tr("Fatigué pendant la nuit ? Marcher, boire de l'eau et se rafraîchir\naident à lutter temporairement contre la fatigue."),
        tr("La nuit est souvent dure à passer, mais le matin peut te\nsurprendre. Fais attention."),
        // Applications
        tr("Tu peux regarder le classement en cliquant sur l'onglet\nNavigation, puis sur \"Classement\"."),
        tr("Un objectif en tête ? Tu peux calculer le nombre de\nvictoires/heures à gauche de cette fenêtre."),
        tr("Tu peux générer les graphiques de plusieurs statistiques : \nRang, Points, Points/heure, ..."),
        tr("La touche \"Tab\" te permet de rapidement changer de page. Essaye donc !"),
        // Questions
        tr("Team Café, Team Boisson énergisante ou Team Eau ?"),
        // Bref...
        QStringLiteral("\nYou can't fall asleep if you have to piss\" - Lotad")
    };

    // Opacity effect
    tipsEffect = new QGraphicsOpacityEffect(ui->label_tips);
    tipsEffect->setOpacity(0.0);
    ui->label_tips->setGraphicsEffect(tipsEffect);

    // Couleur vert clair
    ui->label_tips->setStyleSheet("color: lightgreen;");

    // Build animations
    auto fadeIn = new QPropertyAnimation(tipsEffect, "opacity", this);
    fadeIn->setDuration(1200);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);

    auto visiblePause = new QPauseAnimation(12000, this);

    auto fadeOut = new QPropertyAnimation(tipsEffect, "opacity", this);
    fadeOut->setDuration(1200);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    auto blankPause = new QPauseAnimation(200, this);

    tipsGroup = new QSequentialAnimationGroup(this);
    tipsGroup->addAnimation(fadeIn);
    tipsGroup->addAnimation(visiblePause);
    tipsGroup->addAnimation(fadeOut);
    tipsGroup->addAnimation(blankPause);

    connect(tipsGroup, &QSequentialAnimationGroup::finished, this, &MainWindow::restartTipsCycle);

    // Start first cycle
    restartTipsCycle();
}

void MainWindow::restartTipsCycle()
{
    if (!ui->label_tips) return;
    if (tipsPhrases.isEmpty()) return;

    // Pick a new random index, avoid repeating the previous one if possible
    int idx;
    do {
        idx = QRandomGenerator::global()->bounded(tipsPhrases.size());
    } while (tipsPhrases.size() > 1 && idx == lastTipIndex);
    lastTipIndex = idx;

    // Set text while opacity is 0 (between cycles)
    ui->label_tips->setText(tipsPhrases.at(idx));

    // Restart animation group
    if (tipsGroup) {
        tipsGroup->start();
    }
}

// + Easter-egg setup: overlay label and filter on label_time_left
void MainWindow::setupEasterEgg()
{
    if (!ui->label_time_left) return;

    // Rendre le label sensible aux survols et clics
    ui->label_time_left->setAttribute(Qt::WA_Hover, true);
    ui->label_time_left->setMouseTracking(true);
    ui->label_time_left->installEventFilter(this);

    // Créer l’overlay image
    if (!easterEggLabel) {
        easterEggLabel = new QLabel(this);
        easterEggLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        easterEggLabel->setStyleSheet("background: transparent;");
        QPixmap pix(":/images/image.png");
        easterEggLabel->setPixmap(pix);
        easterEggLabel->adjustSize();
        easterEggLabel->hide();

        // Opacité (restée à 1 pour un slide pur)
        easterEggOpacity = new QGraphicsOpacityEffect(easterEggLabel);
        easterEggOpacity->setOpacity(1.0);
        easterEggLabel->setGraphicsEffect(easterEggOpacity);

        // Animation de slide en diagonale
        easterEggSlideAnim = new QPropertyAnimation(easterEggLabel, "pos", this);
        easterEggSlideAnim->setDuration(250); // 0.25s
        // easterEggSlideAnim->setEasingCurve(QEasingCurve::OutCubic); // optionnel
    }
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (ui->label_time_left && watched == ui->label_time_left) {
        switch (event->type()) {
        case QEvent::Enter:
        case QEvent::HoverEnter:
        case QEvent::MouseButtonPress:
            if (!easterEggDone) {
                easterEggDone = true;
                easterEggActive = true;
                showEasterEgg();
            }
            break;
        case QEvent::Leave:
        case QEvent::HoverLeave:
        case QEvent::MouseButtonRelease:
            if (easterEggActive) {
                hideEasterEgg();
            }
            break;
        default:
            break;
        }
    }
    // Keep the custom picker sized to its host placeholder
    if (tbPickerHost && watched == tbPickerHost && event->type() == QEvent::Resize) {
        if (tbPicker) tbPicker->setGeometry(tbPickerHost->rect());
    }
    return false;
}

void MainWindow::showEasterEgg()
{
    if (!easterEggLabel || !easterEggSlideAnim) return;

    // Calculer positions (bas-gauche, moitié visible)
    const int margin = 8;
    const int labelW = easterEggLabel->width();
    const int labelH = easterEggLabel->height();
    const int winH = this->height();

    // Départ: totalement en dehors (bas-gauche)
    QPoint startPos(-labelW, winH);
    // Arrivée: moitié de l'image visible horizontalement, 8px du bas
    QPoint endPos(-labelW / 2, winH - (labelH / 2) - margin);

    easterEggSlideAnim->stop();
    easterEggLabel->move(startPos);
    easterEggLabel->raise();
    easterEggLabel->show();

    easterEggSlideAnim->setStartValue(startPos);
    easterEggSlideAnim->setEndValue(endPos);
    easterEggSlideAnim->setDuration(250);
    // easterEggSlideAnim->setEasingCurve(QEasingCurve::OutCubic); // optionnel
    easterEggSlideAnim->start();
}

void MainWindow::hideEasterEgg()
{
    if (!easterEggLabel || !easterEggSlideAnim) return;

    const int labelW = easterEggLabel->width();
    const int winH = this->height();

    // Sortie: glisse vers l’extérieur (bas-gauche)
    QPoint endPos(-labelW, winH);

    easterEggSlideAnim->stop();
    easterEggSlideAnim->setStartValue(easterEggLabel->pos());
    easterEggSlideAnim->setEndValue(endPos);
    easterEggSlideAnim->setDuration(250);
    // easterEggSlideAnim->setEasingCurve(QEasingCurve::InCubic); // optionnel

    QObject::connect(easterEggSlideAnim, &QPropertyAnimation::finished, this, [this]() {
        if (easterEggLabel) easterEggLabel->hide();
        easterEggActive = false;
    }, Qt::SingleShotConnection);

    easterEggSlideAnim->start();
}

void MainWindow::on_lineEdit_afk_textEdited(const QString &arg1)
{
    // On veut tout simplement récupérer la valeur et calculer le goal
    // Avoir la nouvelle valeur
    std::cout<<"arg1 : " << arg1.toStdString() << std::endl;

    // Vérifier si on_lineEdit_afk et on_lineEdit_goal ont des valeurs
    if (!ui->lineEdit_afk->text().isEmpty() && !ui->lineEdit_goal->text().isEmpty()) {

        QLabel *labelWinPace = ui->label_win_pace;
        QLabel *labeltext = ui->label_7;

        // Calculer le goal en fonction de la valeur AFK
        std::string winsStr = functb::wins;
        std::cout << "Step 1: Retrieved winsStr: " << winsStr << std::endl;

        std::string pointsStr = functb::points;
        std::cout << "Step 2: Retrieved pointsStr: " << pointsStr << std::endl;

        std::string seedStr = functb::seed;
        std::cout << "Step 3: Retrieved seedStr: " << seedStr << std::endl;

        std::string hourStr = functb::hour_missing;
        std::cout << "Step 4: Retrieved hourStr: " << hourStr << std::endl;

        if (pointsStr == "-1") {
            std::cout << "Step 5: pointsStr is -1, exiting function." << std::endl;
            ui->label_win_pace->setText("");
            QString color = "red";
            QString comment = tr("Rentrez votre identifiant\net générer un graphique\nd'abord !");
            QString style = QString("color: %1; font-size: 14px;").arg(color);
            labeltext->setStyleSheet(style);
            labeltext->setText(comment);
            hasWinPace = false; // NEW: invalidate
            updateGoalOverlayOnGraphs(true); // NEW: remove if showing
            return;
        }

        int points = std::stoi(pointsStr);
        std::cout << "Step 6: Converted pointsStr to integer: " << points << std::endl;

        int seedValue = std::stoi(seedStr);
        std::cout << "Step 7: Converted seedStr to integer: " << seedValue << std::endl;

        // Hours/minutes AFK (minutes come from combo box 0/15/30/45)
        int afkHoursInt = ui->lineEdit_afk->text().remove(',').toInt();
        int afkMinutesInt = 0;
        if (ui->combo_afk_minutes) {
            afkMinutesInt = ui->combo_afk_minutes->currentText().toInt(); // 0,15,30,45
        }
        double afkTotalHours = static_cast<double>(afkHoursInt) + (static_cast<double>(afkMinutesInt) / 60.0);
        std::cout << "Step 8: AFK hours=" << afkHoursInt << " AFK minutes=" << afkMinutesInt
                  << " => AFK total=" << afkTotalHours << "h" << std::endl;

        int goalValue = ui->lineEdit_goal->text().remove(',').toInt();
        std::cout << "Step 9: Retrieved goalValue from lineEdit_goal: " << goalValue << std::endl;

        float hours_left_total = std::stof(hourStr);
        std::cout << "Step 10: Converted hourStr to float: " << hours_left_total << std::endl;

        if(points > goalValue) {
            std::cout << "Step 10.1: points is greater than goalValue, exiting function." << std::endl;
            ui->label_win_pace->setText("");
            QString color = "red";
            QString comment = tr("Vous avez dépassé \nvotre objectif !");
            QString style = QString("color: %1; font-size: 14px;").arg(color);
            labeltext->setStyleSheet(style);
            labeltext->setText(comment);
            return;
        }

        if(seedValue == 0) {
            std::cout << "Step 10.2: seedValue is 0, exiting function to avoid division by zero." << std::endl;
            ui->label_win_pace->setText("");
            QString color = "red";
            QString comment = tr("Le seed est nul !");
            QString style = QString("color: %1; font-size: 14px;").arg(color);
            labeltext->setStyleSheet(style);
            labeltext->setText(comment);
            hasWinPace = false; // NEW
            updateGoalOverlayOnGraphs(true); // NEW
            return;
        }

        // NEW: validate AFK vs remaining time using minutes too
        if (afkTotalHours >= hours_left_total) {
            std::cout << "Step 10.3: AFK total >= remaining hours, exiting function." << std::endl;
            ui->label_win_pace->setText("");
            QString color = "red";
            QString comment = tr("Impossible");
            QString style = QString("color: %1; font-size: 14px;").arg(color);
            labeltext->setStyleSheet(style);
            labeltext->setText(comment);
            hasWinPace = false; // NEW
            updateGoalOverlayOnGraphs(true); // NEW
            return;
        }

        // NEW: compute active hours and use them in pace calculation
        const double activeHours = hours_left_total - afkTotalHours;
        std::cout << "Active hours: " << activeHours << std::endl;

        float winsPerHour = (goalValue - points) / (seedValue * static_cast<float>(activeHours));
        std::cout << "Step 11: Calculated winsPerHour: " << winsPerHour << std::endl;

        QString color;
        QString comment;

        if (winsPerHour < 8) {
            color = "blue";
            comment = tr("Très Facile");
        } else if (winsPerHour < 10) {
            color = "green";
            comment = tr("Facile");
        } else if (winsPerHour < 12) {
            color = "yellow";
            comment = tr("Moyen");
        } else if (winsPerHour < 13) {
            color = "orange";
            comment = tr("Difficile");
        } else if (winsPerHour < 14) {
            color = "red";
            comment = tr("Très difficile");
        } else {
            color = "darkred";
            comment = tr("bonne chance mdr");
        }

        QString style = QString("color: %1; font-size: 47px;").arg(color);
        labelWinPace->setStyleSheet(style);
        labelWinPace->setText(QString::number(winsPerHour, 'f', 2));

        QString style_ = QString("color: %1; font-size: 14px;").arg(color);
        labeltext->setStyleSheet(style_);
        labeltext->setText(comment);

        // NEW: store last computed win pace and update overlay if asked
        lastWinPace = static_cast<double>(winsPerHour);
        hasWinPace = std::isfinite(lastWinPace) && lastWinPace > 0.0;
        updateGoalOverlayOnGraphs(false);
    }
}


void MainWindow::on_lineEdit_goal_textEdited(const QString &arg1)
{
    MainWindow::on_lineEdit_afk_textEdited(arg1);
}

void MainWindow::showOptionsDialog()
{
    QFile uiFile(":/options.ui");
    if (!uiFile.open(QFile::ReadOnly)) {
        return;
    }
    QUiLoader loader;
    QWidget *content = loader.load(&uiFile, nullptr);
    uiFile.close();
    if (!content) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Options"));
    QVBoxLayout layout(&dlg);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.addWidget(content);

    // Find widgets
    auto radioGlo = content->findChild<QRadioButton*>("radioGlo");
    auto radioJap = content->findChild<QRadioButton*>("radioJap");
    auto comboTheme = content->findChild<QComboBox*>("comboTheme");
    auto buttonBox = content->findChild<QDialogButtonBox*>("buttonBox");
    // NEW: privacy
    auto checkCensorId = content->findChild<QCheckBox*>("checkCensorId");
    // NEW: background selectors
   
    auto checkCustom = content->findChild<QCheckBox*>("checkCustomBackground");
    auto editPath = content->findChild<QLineEdit*>("editBackgroundPath");
    auto btnBrowse = content->findChild<QPushButton*>("buttonBrowseBackground");
    auto sliderOpacity = content->findChild<QSlider*>("sliderOpacity");
    auto labelOpacityValue = content->findChild<QLabel*>("labelOpacityValue");
    // NEW: extra delay spin
    auto spinExtraDelay = content->findChild<QSpinBox*>("spinExtraDelay"); // NEW
    // NEW: transparent controls
    auto checkTransparent = content->findChild<QCheckBox*>("checkTransparentControls"); // NEW

    // Initialize from settings
    if (radioGlo && radioJap) {
        if (AppSettings::region == "Jap") radioJap->setChecked(true);
        else radioGlo->setChecked(true);
    }
    if (comboTheme) {
        // OLD (par texte) supprimé
        // int idx = comboTheme->findText(AppSettings::chartThemeName);
        // if (idx >= 0) comboTheme->setCurrentIndex(idx);
        comboTheme->setCurrentIndex(
            qBound(0, AppSettings::chartThemeIndex,
                   comboTheme->count() > 0 ? comboTheme->count()-1 : 0));
    }
    if (checkCensorId) checkCensorId->setChecked(AppSettings::censorIdDisplay);
    if (checkCustom) checkCustom->setChecked(AppSettings::useCustomBackground);
    if (editPath) {
        editPath->setText(AppSettings::backgroundPath);
        editPath->setEnabled(AppSettings::useCustomBackground);
    }
    if (btnBrowse) btnBrowse->setEnabled(AppSettings::useCustomBackground);
    if (sliderOpacity) {
        sliderOpacity->setRange(0, 100);
        sliderOpacity->setValue(AppSettings::backgroundDimPercent);
    }
    if (labelOpacityValue && sliderOpacity) {
        labelOpacityValue->setText(QString::number(sliderOpacity->value()) + "%");
        QObject::connect(sliderOpacity, &QSlider::valueChanged, labelOpacityValue, [labelOpacityValue](int v){
            labelOpacityValue->setText(QString::number(v) + "%");
        });
    }
    if (checkCustom && editPath && btnBrowse) {
        QObject::connect(checkCustom, &QCheckBox::toggled, editPath, &QWidget::setEnabled);
        QObject::connect(checkCustom, &QCheckBox::toggled, btnBrowse, &QWidget::setEnabled);
    }
    if (btnBrowse && editPath) {
        QObject::connect(btnBrowse, &QPushButton::clicked, &dlg, [this, editPath]() {
            QString file = QFileDialog::getOpenFileName(this, tr("Choisir une image"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp)"));
            if (!file.isEmpty()) {
                editPath->setText(file);
            }
        });
    }
    // NEW: init extra delay from saved settings
    if (spinExtraDelay) {
        spinExtraDelay->setRange(0, 15); // safety (also set in .ui)
        spinExtraDelay->setValue(AppSettings::autoRefreshExtraDelayMinutes);
    }
    // NEW: init transparent controls checkbox
    if (checkTransparent) {
        checkTransparent->setChecked(AppSettings::transparentControls);
    }

    if (buttonBox) {
        connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    }

    if (dlg.exec() == QDialog::Accepted) {
        const QString prevRegion = AppSettings::region;
        if (radioGlo && radioGlo->isChecked()) AppSettings::region = "Glo";
        if (radioJap && radioJap->isChecked()) AppSettings::region = "Jap";
        if (comboTheme) {
            AppSettings::chartThemeIndex = comboTheme->currentIndex(); // NEW
            // (optionnel: garder compat nom lisible)
            // AppSettings::chartThemeName = comboTheme->currentText(); // plus nécessaire
        }
        if (checkCensorId) AppSettings::censorIdDisplay = checkCensorId->isChecked();

        if (checkCustom) AppSettings::useCustomBackground = checkCustom->isChecked();
        if (editPath) AppSettings::backgroundPath = editPath->text();
        if (sliderOpacity) AppSettings::backgroundDimPercent = sliderOpacity->value();
        // NEW: save extra delay 0..15
        if (spinExtraDelay) AppSettings::autoRefreshExtraDelayMinutes = qBound(0, spinExtraDelay->value(), 15);
        // NEW: save transparent controls setting
        if (checkTransparent) AppSettings::transparentControls = checkTransparent->isChecked();
        AppSettings::save();
        updateBackgroundPalette();
        updateIdLabelDisplay();
        this->update();
        // NEW: reschedule auto-refresh with new offset
        scheduleNextAutoRefresh();
        // NEW: if region changed, refetch TB metadata and rebuild editions
        if (AppSettings::region != prevRegion) {
            // Reset selection to current tournament when region changes
            AppSettings::selectedEdition = 0;
            AppSettings::save();
            buildTbEditionCombo(); // updates custom picker
             fetchAndInitTbMetadata();
        }
    }
}

QString MainWindow::formatDhMin(qint64 secs)
{
    if (secs <= 0) {
        // 0 minute (translatable forms)
        return QStringLiteral("0 ") + tr("minute");
    }
    qint64 days = secs / 86400; secs %= 86400;
    qint64 hours = secs / 3600; secs %= 3600;
    qint64 minutes = secs / 60;

    const QString dWord = (days == 1) ? tr("jour") : tr("jours");
    const QString hWord = (hours == 1) ? tr("heure") : tr("heures");
    const QString mWord = (minutes == 1) ? tr("minute") : tr("minutes");

    QStringList parts;
    if (days > 0)    parts << QString("%1 %2").arg(days).arg(dWord);
    if (hours > 0)   parts << QString("%1 %2").arg(hours).arg(hWord);
    if (minutes > 0 || parts.isEmpty())
                     parts << QString("%1 %2").arg(minutes).arg(mWord);
    return parts.join(", ");
}

void MainWindow::updateTbUiFromTimes()
{
    if (tbStartEpoch <= 0 || tbEndEpoch <= 0) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // Progress as double [0..100] with 1 decimal
    double pctD = 0.0;
    if (now <= tbStartEpoch) {
        pctD = 0.0;
    } else if (now >= tbEndEpoch) {
        pctD = 100.0;
    } else {
        const double total = static_cast<double>(tbEndEpoch - tbStartEpoch);
        const double done  = static_cast<double>(now - tbStartEpoch);
        pctD = std::clamp(done / total, 0.0, 1.0) * 100.0;
    }
    if (tbProgressBar) {
        const int scaled = qBound(0, static_cast<int>(qRound(pctD * 10.0)), 1000); // 0..1000
        tbProgressBar->setValue(scaled);
        tbProgressBar->setFormat(QLocale().toString(pctD, 'f', 1) + QStringLiteral("%"));
    }

    // Time left (localized with placeholder)
    const qint64 remain = std::max<qint64>(0, tbEndEpoch - now);
    if (ui && ui->label_time_left) {
        ui->label_time_left->setText(tr("Temps restant : \n%1").arg(formatDhMin(remain)));
    }
}

// NEW: rank target (1..10000)
QString MainWindow::formatMillionsCompact(qint64 v)
{
    const double m = static_cast<double>(v) / 1'000'000.0;
    const int decimals = (m >= 10.0 ? 1 : 2);
    return QString::number(m, 'f', decimals) + QStringLiteral("M");
}

void MainWindow::updateRankEstimation()
{
    if (!ui) return;
    auto edit = ui->lineEdit_goal_2;
    auto lbl  = ui->label_estimation_rank;
    if (!edit || !lbl) return;
    // reset style when recomputing
    lbl->setStyleSheet("");

    // Validate input rank
    bool okRank = false;
    const int rank = edit->text().toInt(&okRank);
    if (!okRank || rank < 1 || rank > 10000) {
        lbl->clear();
        if (ui->label_win_pace_rank) ui->label_win_pace_rank->clear();
        hasRankWinPace = false;
        updateRankOverlayOnGraphs(true);
        return;
    }

    // Determine base edition (current selected metadata), and min start per region
    int baseEd = tbEdition;
    if (baseEd <= 0) {
        const QJsonObject m = functb::pologetmetadata(AppSettings::selectedEdition);
        baseEd = m.value(QStringLiteral("id")).toInt();
    }
    if (baseEd <= 0) {
        lbl->clear();
        if (ui->label_win_pace_rank) ui->label_win_pace_rank->clear();
        hasRankWinPace = false;
        updateRankOverlayOnGraphs(true);
        return;
    }

    const bool isJP = (AppSettings::region == "Jap" || AppSettings::region == "JP");
    const int start = isJP ? 56 : 55;

    // Build up to 3 editions: base, base-1, base-2 (bounded)
    QVector<int> eds;
    eds.push_back(baseEd);
    if (baseEd - 1 >= start) eds.push_back(baseEd - 1);
    if (baseEd - 2 >= start) eds.push_back(baseEd - 2);

    // Query each edition for the rank points and collect last values
    QStringList lines;
    QVector<qint64> lastValsNewestFirst; // newest -> older
    for (int ed : eds) {
        qint64 lastPts = -1;
        const QJsonObject obj = functb::pologetrank(ed, rank);
        if (!obj.isEmpty() && obj.contains(QStringLiteral("points"))) {
            const QString ptsStr = obj.value(QStringLiteral("points")).toString().remove('[').remove(']');
            const QStringList vals = ptsStr.split(',', Qt::SkipEmptyParts);
            if (!vals.isEmpty()) {
                bool okNum = false;
                lastPts = vals.last().trimmed().toLongLong(&okNum);
                if (!okNum) lastPts = -1;
            }
        }
        if (lastPts >= 0) {
            lines << tr("Score édition %1 : %2").arg(ed).arg(formatMillionsCompact(lastPts));
            lastValsNewestFirst << lastPts;
        } else {
            lines << tr("Score édition %1 : %2").arg(ed).arg(QStringLiteral("—"));
        }
    }

    // Compute points estimation using average delta on chronological order
    qint64 estimatedPoints = -1;
    if (!lastValsNewestFirst.isEmpty()) {
        QVector<qint64> valsChrono = lastValsNewestFirst;
        std::reverse(valsChrono.begin(), valsChrono.end()); // oldest -> newest
        const int n = valsChrono.size();
        if (n == 1) {
            estimatedPoints = valsChrono.last();
        } else {
            double sumDelta = 0.0;
            for (int i = 1; i < n; ++i) sumDelta += static_cast<double>(valsChrono[i] - valsChrono[i-1]);
            const double avgDelta = sumDelta / static_cast<double>(n - 1);
            const double est = static_cast<double>(valsChrono.last()) + avgDelta;
            estimatedPoints = static_cast<qint64>(std::max(0.0, est));
        }
        // Append estimation line
        lines << QString();
        lines << tr("Estimation points : %1").arg(formatMillionsCompact(estimatedPoints));
    }

    lbl->setText(lines.join('\n'));

    // Compute wins/hour needed to reach estimated points (same as Points tab logic)
    if (ui->label_win_pace_rank) {
        // prerequisites
        if (estimatedPoints <= 0) {
            ui->label_win_pace_rank->clear();
            hasRankWinPace = false;
            updateRankOverlayOnGraphs(true);
            return;
        }
        // Current variables from functb
        if (functb::points == "-1") {
            ui->label_win_pace_rank->clear();
            hasRankWinPace = false;
            updateRankOverlayOnGraphs(true);
            return;
        }
        bool okP=true, okS=true, okH=true;
        const int curPoints = QString::fromStdString(functb::points).toInt(&okP);
        const int seedValue = QString::fromStdString(functb::seed).toInt(&okS);
        const double hours_left_total = QString::fromStdString(functb::hour_missing).toDouble(&okH);
        if (!okP || !okS || !okH || seedValue == 0) {
            ui->label_win_pace_rank->clear();
            hasRankWinPace = false;
            updateRankOverlayOnGraphs(true);
            return;
        }

        // AFK total from Rank tab UI (fallback to main if missing)
        int afkHoursInt = 0;
        int afkMinutesInt = 0;
        if (ui->lineEdit_afk_2 && !ui->lineEdit_afk_2->text().isEmpty())
            afkHoursInt = ui->lineEdit_afk_2->text().remove(',').toInt();
        else if (ui->lineEdit_afk)
            afkHoursInt = ui->lineEdit_afk->text().remove(',').toInt();
        if (ui->combo_afk_minutes_2)
            afkMinutesInt = ui->combo_afk_minutes_2->currentText().toInt();
        else if (ui->combo_afk_minutes)
            afkMinutesInt = ui->combo_afk_minutes->currentText().toInt();
        const double afkTotalHours = static_cast<double>(afkHoursInt) + (static_cast<double>(afkMinutesInt) / 60.0);
        const double activeHours = hours_left_total - afkTotalHours;
        if (activeHours <= 0.0) {
            // Impossible: show same red text as Points tab
            lbl->setStyleSheet("color: red;");
            lbl->setText(tr("Impossible"));
            ui->label_win_pace_rank->clear();
            hasRankWinPace = false;
            updateRankOverlayOnGraphs(true);
            return;
        }

        // Target delta
        const qint64 deltaPts = static_cast<qint64>(estimatedPoints) - static_cast<qint64>(curPoints);
        if (deltaPts <= 0) {
            ui->label_win_pace_rank->setText(QStringLiteral("0.00"));
            ui->label_win_pace_rank->setStyleSheet("color: green; font-size: 47px;");
            lastRankWinPace = 0.0;
            hasRankWinPace = true;
            updateRankOverlayOnGraphs(false);
            return;
        }

        const double winsPerHour = static_cast<double>(deltaPts) / (static_cast<double>(seedValue) * activeHours);

        // Color mapping (same thresholds)
        QString color;
        if (winsPerHour < 8)        color = "blue";
        else if (winsPerHour < 10)  color = "green";
        else if (winsPerHour < 12)  color = "yellow";
        else if (winsPerHour < 13)  color = "orange";
        else if (winsPerHour < 14)  color = "red";
        else                        color = "darkred";

        ui->label_win_pace_rank->setStyleSheet(QString("color: %1; font-size: 47px;").arg(color));
        ui->label_win_pace_rank->setText(QString::number(winsPerHour, 'f', 2));
                // Store and update overlay if checkbox_2 is enabled
        lastRankWinPace = winsPerHour;
        hasRankWinPace = std::isfinite(lastRankWinPace) && lastRankWinPace >= 0.0;
        updateRankOverlayOnGraphs(false);
    }
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    // If user enables the overlay but we don't have a computed pace yet, compute it now
    if (checked && !hasWinPace) {
        if (ui && ui->lineEdit_afk) {
            on_lineEdit_afk_textEdited(ui->lineEdit_afk->text());
        }
    }
    updateGoalOverlayOnGraphs(true);
}

// NEW: rank overlay checkbox handlers
void MainWindow::on_checkBox_2_clicked() { /* unused */ }

void MainWindow::on_checkBox_2_clicked(bool checked)
{
    if (checked && !hasRankWinPace) {
        updateRankEstimation();
    }
    updateRankOverlayOnGraphs(true);
}

// NEW: add/remove/update rank overlay on main Graphs chart
void MainWindow::updateRankOverlayOnGraphs(bool allowAxisAdjust)
{
    // Do not resize axes
    allowAxisAdjust = false;
    if (!ui || !ui->graphiqueTest) return;
    QGraphicsView* view = ui->graphiqueTest;
    QChart* chart = Render::chartFromView(view);
    if (!chart) return;

    const QString kRankName = tr("Objectif (Rank)");
    // Remove existing "rank" line if any
    QAbstractSeries* rankSeries = nullptr;
    for (auto s : chart->series()) {
        if (s->name() == kRankName) { rankSeries = s; break; }
    }
    if (rankSeries) {
        chart->removeSeries(rankSeries);
        delete rankSeries;
        rankSeries = nullptr;
    }

    const bool wantOverlay = (ui->checkBox_2 && ui->checkBox_2->isChecked());
    if (!wantOverlay) return;

    // Only show on wins_pace and if we have a computed pace
    const bool yIsWinsPace = (ui->ydataBox &&
                              (ui->ydataBox->currentData(Qt::UserRole).toString() == QStringLiteral("wins_pace") ||
                               ui->ydataBox->currentText() == QStringLiteral("wins_pace"))); // fallback
    if (!hasRankWinPace || !yIsWinsPace) return;

    const double minX = 0.0;
    const double maxX = 71.75;

    auto line = new QLineSeries();
    line->setName(kRankName);
    line->append(minX, lastRankWinPace);
    line->append(maxX, lastRankWinPace);

    QPen pen(goalLineColorForTheme(chart->theme()));
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    line->setPen(pen);

    chart->addSeries(line);
    if (!chart->axes(Qt::Horizontal).isEmpty())
        line->attachAxis(chart->axes(Qt::Horizontal).first());
    if (!chart->axes(Qt::Vertical).isEmpty())
        line->attachAxis(chart->axes(Qt::Vertical).first());
}

void MainWindow::fetchAndInitTbMetadata()
{
    // Use selected edition (0 = current) to fetch metadata
    const QJsonObject m = functb::pologetmetadata(AppSettings::selectedEdition);
    if (m.isEmpty()) return;

    // Cache times
    tbStartEpoch = m.value(QStringLiteral("start_at")).toVariant().toLongLong();
    tbEndEpoch   = m.value(QStringLiteral("end_at")).toVariant().toLongLong();
    // Cache and show title with edition (localized placeholder)
    tbEdition = m.value(QStringLiteral("id")).toInt();
    if (tbTitleLabel && tbEdition > 0) {
        tbTitleLabel->setText(tr("%1ème Tenkaichi Budokai").arg(tbEdition));
    }
    // First UI refresh
    updateTbUiFromTimes();
    // NEW: also refresh rank estimation (depends on edition/region)
    updateRankEstimation();
}

// NEW: rebuild localized title/time using cached metadata (no refetch)
void MainWindow::refreshTbLocalizedTexts()
{
    if (tbTitleLabel && tbEdition > 0) {
        tbTitleLabel->setText(tr("%1ème Tenkaichi Budokai").arg(tbEdition));
    }
    updateTbUiFromTimes();
}

// NEW: apply background from settings (image + dim overlay)
void MainWindow::updateBackgroundPalette()
{
    const bool useBg = AppSettings::useCustomBackground && !AppSettings::backgroundPath.isEmpty();
    const bool transparentControls = AppSettings::transparentControls; // NEW
    QMenuBar* mb = this->menuBar();
    QWidget* central = this->centralWidget();

    // Always keep menu bar and status bar transparent
    if (mb) {
        mb->setAttribute(Qt::WA_StyledBackground, true);
        mb->setAutoFillBackground(false);
        mb->setStyleSheet("QMenuBar { background: transparent; }");
    }
    if (statusBar()) {
        statusBar()->setAttribute(Qt::WA_StyledBackground, true);
        statusBar()->setAutoFillBackground(false);
        statusBar()->setStyleSheet("QStatusBar { background: transparent; }");
    }

    if (useBg) {
        QPixmap src(AppSettings::backgroundPath);
        if (!src.isNull()) {
            // Paint the wallpaper on the whole main window (client area)
            QPixmap scaled = src.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            QPixmap composed(scaled.size());
            composed.fill(Qt::transparent);
            {
                QPainter p(&composed);
                p.drawPixmap(0, 0, scaled);
                if (AppSettings::backgroundDimPercent > 0) {
                    const int a = qBound(0, AppSettings::backgroundDimPercent, 100) * 255 / 100;
                    p.fillRect(composed.rect(), QColor(0, 0, 0, a));
                               }
            }
            QPalette pal = this->palette();
            pal.setBrush(QPalette::Window, QBrush(composed));
            this->setPalette(pal);
            this->setAutoFillBackground(true);
        }
    } else {
        // No wallpaper
        this->setAutoFillBackground(false);
        this->setPalette(QPalette());
    }

    // Central content transparency follows the setting
    if (central) {
        if (transparentControls) {
            // All content transparent (inherit), wallpaper visible everywhere
            central->setAutoFillBackground(false);
            central->setPalette(QPalette());
            central->setAttribute(Qt::WA_StyledBackground, true);
            central->setStyleSheet("background: transparent;");
        } else {
            // Keep wallpaper visible in gaps (no fill), but let children paint opaque by removing inherited transparency
            central->setAttribute(Qt::WA_StyledBackground, false);
            central->setAutoFillBackground(false); // CHANGED: was true (hid the wallpaper)
            central->setPalette(QPalette());
            central->setStyleSheet("");            // remove transparent background inheritance
        }
        central->update();
    }

    if (mb) mb->update();
    if (statusBar()) statusBar()->update();
    this->update();
}

// NEW: mask helper for ID display
QString MainWindow::maskedIdentifier(const QString& id) const
{
    if (id.isEmpty()) return QString();
    // Full password-like masking with bullet characters
    return QString(id.size(), QChar(0x2022)); // •••••
}

// NEW: refresh the "Identifiant actuel" label
void MainWindow::updateIdLabelDisplay()
{
    if (!ui || !ui->label) return;
    const QString prefix = tr("Identifiant actuel : ");
    const QString realId = QString::fromStdString(functb::identifier).trimmed();
    const QString shown = AppSettings::censorIdDisplay ? maskedIdentifier(realId) : realId;
    ui->label->setText(prefix + shown);
}

// NEW: schedule next trigger at next quarter + offset (in minutes)
void MainWindow::scheduleNextAutoRefresh()
{
    if (!autoRefreshTimer) return;
    const QDateTime now = QDateTime::currentDateTime();
    const QTime t = now.time();
    const int totalMin = t.hour() * 60 + t.minute();
    const int sec = t.second();
    const int msec = t.msec();

    const int d = (AppSettings::autoRefreshExtraDelayMinutes % 15 + 15) % 15;
    int rem = ((d - (totalMin % 15)) + 15) % 15;
    if (rem == 0 && (sec > 0 || msec > 0)) rem = 15;

    const qint64 toNextMinute = (60 - sec) * 1000 - msec;
    qint64 msToNext = (rem == 0) ? toNextMinute : (toNextMinute + (rem - 1) * 60 * 1000);
    if (msToNext < 250) msToNext = 250;

    // NEW: log scheduling info
    std::cout << "[AutoRefresh] offset=" << d
              << "min, next in " << (msToNext / 1000) << "s (rem=" << rem << "min)"
              << std::endl;

    autoRefreshTimer->start(msToNext);
}

// NEW: perform refresh if Classement page is selected; always reschedule
void MainWindow::doAutoRefreshIfClassement()
{
    std::cout << "[AutoRefresh] Timer fired" << std::endl;
    // enlever la condition
    Leaderboard::autoRefresh(this);
    scheduleNextAutoRefresh();
}

// NEW: copy graph and show confirmation
void MainWindow::copyClassementGraphToClipboard()
{
    // Only when Classement page is active
    if (!stackedWidget || stackedWidget->currentIndex() != 1) return;
    if (!Leaderboard::graphPlaceholder) {
        // Other error
        if (confirmCopyLabel && confirmCopyEffect && confirmCopyHoldTimer) {
            confirmCopyLabel->setStyleSheet("color: red;");
            confirmCopyLabel->setText(tr("Une erreur est survenue lors de la copie du graphique."));
            confirmCopyLabel->setVisible(true);
            confirmCopyHoldTimer->stop();
            if (confirmCopyFade) confirmCopyFade->stop();
            confirmCopyEffect->setOpacity(1.0);
            confirmCopyHoldTimer->start(3000);
        }
        return;
    }

    QGraphicsView* view = Leaderboard::graphPlaceholder;

    // If no chart currently displayed
    if (!view->scene() || view->scene()->items().isEmpty()) {
        if (confirmCopyLabel && confirmCopyEffect && confirmCopyHoldTimer) {
            confirmCopyLabel->setStyleSheet("color: red;");
            confirmCopyLabel->setText(tr("Aucun graphique n'a\nété affiché !"));
            confirmCopyLabel->setVisible(true);
            confirmCopyHoldTimer->stop();
            if (confirmCopyFade) confirmCopyFade->stop();
            confirmCopyEffect->setOpacity(1.0);
            confirmCopyHoldTimer->start(3000);
        }
        return;
    }

    // Grab image of current chart (base + overlays) - CHANGED: only the chart, not the whole view
    QImage img = Render::grabChartOnly(view);
    if (img.isNull()) {
        img = Render::grabChartImage(view); // fallback
    }
    if (img.isNull()) {
        // Other error
        if (confirmCopyLabel && confirmCopyEffect && confirmCopyHoldTimer) {
            confirmCopyLabel->setStyleSheet("color: red;");
            confirmCopyLabel->setText(tr("Erreur lors de la copie."));
            confirmCopyLabel->setVisible(true);
            confirmCopyHoldTimer->stop();
            if (confirmCopyFade) confirmCopyFade->stop();
            confirmCopyEffect->setOpacity(1.0);
            confirmCopyHoldTimer->start(3000);
        }
        return;
    }

    // Copy to clipboard
    QClipboard* cb = QApplication::clipboard();
    cb->setImage(img);

    // Show confirmation text: "Graphique copié !" (reset any error style)
    if (confirmCopyLabel && confirmCopyEffect && confirmCopyHoldTimer) {
        confirmCopyLabel->setStyleSheet(""); // reset style (no red on success)
        confirmCopyLabel->setText(tr("Graphique copié !"));
        confirmCopyLabel->setVisible(true);
        confirmCopyHoldTimer->stop();
        if (confirmCopyFade) confirmCopyFade->stop();
        confirmCopyEffect->setOpacity(1.0);
        // Hold 3 seconds then fade 1.2s (wired in constructor)
        confirmCopyHoldTimer->start(3000);
    }
}

// NEW: copy context-aware (Graphs or Classement)
void MainWindow::copyAnyGraphToClipboard()
{
    if (!stackedWidget) return;
    const int idx = stackedWidget->currentIndex();
    if (idx == 1) {
        // Classement page
        copyClassementGraphToClipboard();
        return;
    }
    if (!ui || !ui->graphiqueTest) return;
    QGraphicsView* view = ui->graphiqueTest;
    if (!view->scene() || view->scene()->items().isEmpty()) {
        if (ui->boitetext) {
            auto prev = ui->boitetext->textColor();
            ui->boitetext->setTextColor(Qt::red);
            ui->boitetext->append(tr("Erreur : aucun graphique affiché."));
            ui->boitetext->setTextColor(prev);
        }
        return;
    }

    // CHANGED: only the chart, not the entire viewport
    QImage img = Render::grabChartOnly(view);
    if (img.isNull()) {
        img = Render::grabChartImage(view); // fallback
    }
    if (img.isNull()) {
        if (ui->boitetext) {
            auto prev = ui->boitetext->textColor();
            ui->boitetext->setTextColor(Qt::red);
            ui->boitetext->append(tr("Erreur : une erreur s'est produite lors de la copie du graphique"));
            ui->boitetext->setTextColor(prev);
        }
        return;
    }
    QClipboard* cb = QApplication::clipboard();
    cb->setImage(img);
}

// NEW: checkbox toggled -> compute pace if needed, then update overlay
void MainWindow::onGoalOverlayToggled(bool checked)
{
    // If user enables the overlay but we don't have a computed pace yet, compute it now
    if (checked && !hasWinPace) {
        if (ui && ui->lineEdit_afk) {
            on_lineEdit_afk_textEdited(ui->lineEdit_afk->text());
        }
    }
    updateGoalOverlayOnGraphs(true);
}

// NEW: theme-based color for the goal line
QColor MainWindow::goalLineColorForTheme(QChart::ChartTheme t)
{
    switch (t) {
    case QChart::ChartThemeDark:         return QColor("#FFD700"); // gold
    case QChart::ChartThemeBlueCerulean: return QColor("#DC143C"); // crimson
    case QChart::ChartThemeBlueNcs:      return QColor("#FF8C00"); // dark orange
    case QChart::ChartThemeBlueIcy:      return QColor("#8B0000"); // dark red
    case QChart::ChartThemeHighContrast: return QColor("#00FFFF"); // cyan
    case QChart::ChartThemeQt:           return QColor("#8A2BE2"); // blue violet
    case QChart::ChartThemeBrownSand:    return QColor("#00BFFF"); // deep sky blue
    case QChart::ChartThemeLight:
    default:                             return QColor("#FF0000"); // red
    }
}

// NEW: add/remove/update goal overlay on main Graphs chart
void MainWindow::updateGoalOverlayOnGraphs(bool allowAxisAdjust)
{
    // Tout compte fait, on ne fera PAS de resize des abscisses
    allowAxisAdjust = false;
    if (!ui || !ui->graphiqueTest) return;
    QGraphicsView* view = ui->graphiqueTest;
    QChart* chart = Render::chartFromView(view);
    if (!chart) return;

    // Remove existing goal line if any
    const QString kGoalName = tr("Objectif");
    QAbstractSeries* goalSeries = nullptr;
    for (auto s : chart->series()) {
        if (s->name() == kGoalName) { goalSeries = s; break; }
    }
    if (goalSeries) {
        chart->removeSeries(goalSeries);
        delete goalSeries;
        goalSeries = nullptr;
    }

    // When unchecked: optionally restore X axis to data range
    const bool wantOverlay = (ui->checkBox && ui->checkBox->isChecked());
    if (!wantOverlay) {
        if (allowAxisAdjust) {
            // Restore X range from first series data if available
            if (!chart->series().isEmpty()) {
                if (auto ls = qobject_cast<QLineSeries*>(chart->series().first())) {
                    const auto pts = ls->pointsVector();
                    if (!pts.isEmpty()) {
                        double minX = pts.first().x();
                        double maxX = pts.last().x();
                        for (const auto& p : pts) { minX = std::min(minX, p.x()); maxX = std::max(maxX, p.x()); }
                        const auto hAxes = chart->axes(Qt::Horizontal);
                        if (!hAxes.isEmpty()) {
                            hAxes.first()->setRange(minX, maxX);
                        }
                    }
                }
            }
        }
        return;
    }

    // Need overlay: require valid pace and ydata == wins_pace
    //const bool yIsWinsPace = (ui->ydataBox && ui->ydataBox->currentText() == QStringLiteral("wins_pace"));
    const bool yIsWinsPace = (ui->ydataBox &&
                              (ui->ydataBox->currentData(Qt::UserRole).toString() == QStringLiteral("wins_pace") ||
                               ui->ydataBox->currentText() == QStringLiteral("wins_pace"))); // fallback
    if (!hasWinPace || !yIsWinsPace) {
        // Clamp X anyway if requested and checkbox is checked
        if (allowAxisAdjust) {
            const double minX = 0.0;
            const double maxX = 71.75;
            const auto hAxes = chart->axes(Qt::Horizontal);
            if (!hAxes.isEmpty()) hAxes.first()->setRange(minX, maxX);
        }
        return;
    }

    // Clamp X to [0; 71.75]
    const double minX = 0.0;
    const double maxX = 71.75;
    if (allowAxisAdjust) {
        const auto hAxes = chart->axes(Qt::Horizontal);
        if (!hAxes.isEmpty()) hAxes.first()->setRange(minX, maxX);
    }

    // Add horizontal line at y = lastWinPace
    auto line = new QLineSeries();
    line->setName(kGoalName);
    line->append(minX, lastWinPace);
    line->append(maxX, lastWinPace);

    QPen pen(goalLineColorForTheme(chart->theme()));
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    line->setPen(pen);

    chart->addSeries(line);
    // Attach to current axes
    if (!chart->axes(Qt::Horizontal).isEmpty())
        line->attachAxis(chart->axes(Qt::Horizontal).first());
    if (!chart->axes(Qt::Vertical).isEmpty())
        line->attachAxis(chart->axes(Qt::Vertical).first());
}

// NEW: populate the TB edition ComboBox based on region + latest edition
void MainWindow::buildTbEditionCombo()
{
    // Fetch latest/current edition number from /api/0/metadata
    const QJsonObject cur = functb::pologetmetadata(0);
    const int latest = cur.value(QStringLiteral("id")).toInt();
    if (latest <= 0) return;
    const bool isJP = (AppSettings::region == "Jap" || AppSettings::region == "JP");
    const int start = isJP ? 56 : 55;

    // Build list [latest..start] descending
    QVector<int> list; list.reserve(qMax(0, latest - start + 1));
    for (int ed = latest; ed >= start; --ed) list.push_back(ed);

    if (tbPicker) {
        tbPicker->setEditions(list, AppSettings::selectedEdition, latest);
    }
}