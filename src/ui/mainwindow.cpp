#include "raceanalysisdialog.h"
#include "editionpicker.h"
#include "wtapi.h"
#include "wtdata.h"
#include "performanceanalysis.h"
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
#include <QStandardPaths> // NEW
#include <cmath> // FIX: for std::round/std::clamp usage with cmath

// NEW: Qt Charts includes for Joueur page
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX 1 // FIX: prevent windows.h from defining min/max macros
#endif
#include <windows.h>
#include <shobjidl.h>
#include <objbase.h>
// faire le prototype
static bool createStartMenuShortcut(const QString& displayName);
#endif

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



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , menu1_action1(nullptr)
    , menu1_action2(nullptr)
    , menu1(nullptr)
    , labelDynamic(nullptr)
{
    ui->setupUi(this);

    // Set up standard widgets to follow transparency settings. Done dynamically in updateBackgroundPalette() now.

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

    m_debounceTimerRank = new QTimer(this);
    m_debounceTimerRank->setSingleShot(true);
    m_debounceTimerRank->setInterval(660);
    connect(m_debounceTimerRank, &QTimer::timeout, this, &MainWindow::updateRankEstimation);

    m_debounceTimerGoal = new QTimer(this);
    m_debounceTimerGoal->setSingleShot(true);
    m_debounceTimerGoal->setInterval(660);
    connect(m_debounceTimerGoal, &QTimer::timeout, this, [this]() {
        if (ui->lineEdit_afk) doGoalEstimation();
    });

    // NEW: Rank target validator + live updates
    if (ui->lineEdit_goal_2) {
        ui->lineEdit_goal_2->setValidator(new QIntValidator(1, 10000, this));
        connect(ui->lineEdit_goal_2, &QLineEdit::textChanged, this, [this]() { m_debounceTimerRank->start(); });
    }
    
    // Connect estimation label interaction
    if (ui->label_estimation_rank) {
        ui->label_estimation_rank->setContextMenuPolicy(Qt::NoContextMenu);
        connect(ui->label_estimation_rank, &QLabel::linkActivated, this, &MainWindow::showRankAnalysisDialog);
    }

    // Connecter le signal pour formatter le nombre avec des virgules
    // connect(ui->lineEdit_goal, &QLineEdit::textChanged, this, &MainWindow::formatNumberWithCommas);
    // connect(ui->lineEdit_afk, &QLineEdit::textChanged, this, &MainWindow::formatNumberWithCommas);

    connect(ui->lineEdit_goal, &QLineEdit::textChanged, [=]() {
        QString formattedNumber;
        formatNumberWithCommas(ui->lineEdit_goal->text(), formattedNumber);
        if (ui->lineEdit_goal->text() != formattedNumber) ui->lineEdit_goal->setText(formattedNumber);
        m_debounceTimerGoal->start();
    });
    connect(ui->lineEdit_afk, &QLineEdit::textChanged, [=]() {
        QString formattedNumber;
        formatNumberWithCommas(ui->lineEdit_afk->text(), formattedNumber);
        if (ui->lineEdit_afk->text() != formattedNumber) ui->lineEdit_afk->setText(formattedNumber);
        m_debounceTimerGoal->start();
    });

    // pr page rank
    if (ui->lineEdit_afk_2) {
        connect(ui->lineEdit_afk_2, &QLineEdit::textChanged, this, [this](){ updateAfkSummaries(); m_debounceTimerRank->start(); });
    }
    if (ui->combo_afk_minutes_2) {
        connect(ui->combo_afk_minutes_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int){ updateAfkSummaries(); m_debounceTimerRank->start(); });
    }
    // NEW: rank overlay checkbox wiring
    if (ui->checkBox_2) {
        connect(ui->checkBox_2, &QCheckBox::clicked, this, QOverload<bool>::of(&MainWindow::on_checkBox_2_clicked));
    }

    // NEW: recalculer quand les minutes AFK changent
    if (ui->combo_afk_minutes) {
        connect(ui->combo_afk_minutes, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int){ updateAfkSummaries(); m_debounceTimerGoal->start(); m_debounceTimerRank->start(); });
    }
    // Also refresh Rank estimation when AFK hours text changes
    if (ui->lineEdit_afk) {
        connect(ui->lineEdit_afk, &QLineEdit::textChanged, this, [this](){ updateAfkSummaries(); m_debounceTimerRank->start(); });
    }

    // NEW: Create AFK summary labels
    m_lblAfkSummary1 = new QLabel(ui->tab);
    m_lblAfkSummary1->setGeometry(10, 220, 250, 16);
    m_lblAfkSummary1->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    connect(m_lblAfkSummary1, &QLabel::linkActivated, this, [this](const QString&) { this->showOptionsDialog(); });
    
    m_lblAfkSummary2 = new QLabel(ui->tab_2);
    m_lblAfkSummary2->setGeometry(10, 140, 250, 16);
    m_lblAfkSummary2->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    connect(m_lblAfkSummary2, &QLabel::linkActivated, this, [this](const QString&) { this->showOptionsDialog(); });
    
    updateAfkSummaries();

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

    // NEW: Charger la page Joueur depuis joueur.ui
    QFile joueurUi(":/joueur.ui");
    if (joueurUi.open(QFile::ReadOnly)) {
        QUiLoader loader;
        pageJoueur = loader.load(&joueurUi, this);
        joueurUi.close();
    }
    if (!pageJoueur) {
        pageJoueur = new QWidget(this);
    }
    stackedWidget->addWidget(pageJoueur);
    setupJoueurPage();

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
    actionPagePrincipale->setObjectName("actionGraphiques");
    QAction *actionPageSecondaire = new QAction(tr("Classement"), this);
    actionPageSecondaire->setObjectName("pageSecondaire");
    // NEW: action Joueur
    QAction *actionPageJoueur = new QAction(tr("Joueur"), this);
    actionPageJoueur->setObjectName("actionJoueur");
    actionPageJoueur->setIcon(QIcon(":/images/guy.png")); // CHANGED: utiliser guy.png

    menuNavigation->addAction(actionPagePrincipale);
    menuNavigation->addAction(actionPageSecondaire);
    menuNavigation->addAction(actionPageJoueur); // NEW

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

    // NEW: connect Joueur
    connect(actionPageJoueur, &QAction::triggered, this, [this]() {
        stackedWidget->setCurrentIndex(2);
        populateJoueurEditions(); // refresh editions list
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
    // Progress UI for updater
    connect(updater, &Updater::downloadStarted,  this, &MainWindow::onUpdateDownloadStarted);
    connect(updater, &Updater::downloadProgress, this, &MainWindow::onUpdateDownloadProgress);
    connect(updater, &Updater::downloadFinished, this, &MainWindow::onUpdateDownloadFinished);
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

    QTimer::singleShot(0, this, &MainWindow::fetchAndInitTbMetadata);
    auto *analysisAction = this->menuBar()->addAction(tr("Race analysis"));
    analysisAction->setToolTip(tr("Top 20 pace, observed activity, finish scenarios and catch-up estimates."));
    connect(analysisAction, &QAction::triggered, this, [this, analysisAction] {
        const auto region = AppSettings::region;
        const int edition = AppSettings::selectedEdition;
        analysisAction->setEnabled(false);
        WtApi::instance().get(WtApi::endpoint(edition,"get-top100",region),this,
            [this,region,edition,analysisAction](const QByteArray &bytes,const QString &error) {
                if (region!=AppSettings::region || edition!=AppSettings::selectedEdition) {analysisAction->setEnabled(true);return;}
                const auto data=WtData::normalize(QJsonDocument::fromJson(bytes),"top");
                if(!error.isEmpty() || !data.value("top").isArray()) {
                    analysisAction->setEnabled(true);
                    ui->boitetext->append(tr("Race analysis unavailable: %1").arg(error.isEmpty()?tr("Invalid response"):error));return;
                }
                WtApi::instance().get(WtApi::endpoint(edition,"metadata",region),this,
                    [this,region,edition,analysisAction,data](const QByteArray &meta,const QString &) {
                        analysisAction->setEnabled(true);
                        if(region!=AppSettings::region || edition!=AppSettings::selectedEdition) return;
                        auto *dialog=new RaceAnalysisDialog(data.value("top").toArray(),WtData::metadata(QJsonDocument::fromJson(meta)),this);
                        dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->show();
                    },300);
            });
    });

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

void MainWindow::onUpdateAvailable(const QString &latestVersion, const QString &changelog, const QString &downloadUrl)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Mise à jour disponible"));
    msgBox.setText(tr("Une nouvelle version (%1) est disponible.").arg(latestVersion));
    msgBox.setInformativeText(changelog);
    msgBox.setStandardButtons(QMessageBox::Ok);
    QPushButton *downloadButton = msgBox.addButton(tr("Télécharger"), QMessageBox::AcceptRole);
    QPushButton *directLinkButton = msgBox.addButton("GitHub", QMessageBox::AcceptRole);

    msgBox.exec();
    if (msgBox.clickedButton() == directLinkButton) {
        // Ouvrir le lien de téléchargement
        QDesktopServices::openUrl(QUrl(downloadUrl));
        // quitter
        QApplication::quit();
    }
    #ifdef Q_OS_WIN
        // Si "Télécharger", alors on télécharge le fichier
        if (msgBox.clickedButton() == downloadButton) {
            if (updater) {
                // ligne 747: initier la MAJ (télécharger + lancer + fermer l'appli courante)
                updater->startDownloadLatestAsset();
            }
        }
    #else
        if (msgBox.clickedButton() == downloadButton) QDesktopServices::openUrl(QUrl(downloadUrl));
    #endif
}

void MainWindow::on_bouton_graphique_clicked()
{
    // Simuler des données pour le graphique

    // Récupérer les données
    const auto region=AppSettings::region;
    const int edition=AppSettings::selectedEdition;
    const auto generation=++graphGeneration;
    QUrlQuery query;query.addQueryItem("identifier",QString::fromStdString(functb::identifier));
    WtApi::instance().get(WtApi::endpoint(edition,"get-user",region,query),this,
        [this,region,edition,generation](const QByteArray &bytes,const QString &error) {
    if(generation!=graphGeneration || region!=AppSettings::region || edition!=AppSettings::selectedEdition) return;
    auto data=WtData::normalize(QJsonDocument::fromJson(bytes),"users");
    if(!error.isEmpty() || data.isEmpty() || data.contains("error")) {
        ui->boitetext->append(error.isEmpty()?tr("Player data unavailable"):error);return;
    }
    if(AppSettings::hideNegativeTimes) WtData::filterNegativeHours(data);


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
    bool validHour=false;
    const double sampleHour=hour_values.last().trimmed().toDouble(&validHour);
    const double duration=tbEndEpoch>tbStartEpoch?(tbEndEpoch-tbStartEpoch)/3600.0:0;
    double hourValue=validHour && duration>0?std::max(0.0,duration-sampleHour):0;
    if(tbEndEpoch>0 && tbEndEpoch<=QDateTime::currentSecsSinceEpoch()) hourValue=0;
    functb::hour_missing = std::to_string(hourValue);

    // on reset le "goal"
    doGoalEstimation();

    // NEW: (re)apply goal overlay if needed on newly created chart
    updateGoalOverlayOnGraphs(true); // NEW
    });
}


void MainWindow::on_idButton_clicked()
{
    if (ui->lineEdit_id) {
        ui->lineEdit_id->setReadOnly(false);
        // Style changes happen via stylesheet pseudo-state QLineEdit[readOnly="false"]
        ui->lineEdit_id->style()->unpolish(ui->lineEdit_id);
        ui->lineEdit_id->style()->polish(ui->lineEdit_id);
        
        // Force rendering clear of dots if we want to toggle visible
        for(QAction* act : ui->lineEdit_id->actions()) {
            if (act->property("isEyeIcon").toBool()) {
                act->setVisible(true); // make icon visible while editing
                break;
            }
        }
        
        ui->lineEdit_id->setFocus();
        ui->lineEdit_id->selectAll();
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
                if (auto actGraphs = this->findChild<QAction*>("actionGraphiques")) {
                    actGraphs->setText(tr("Graphiques"));
                }
                if (auto actClassement = this->findChild<QAction*>("pageSecondaire")) {
                    actClassement->setText(tr("Classement"));
                }
                if (auto actClassement = this->findChild<QAction*>("actionJoueur")) {
                    actClassement->setText(tr("Joueur"));
                }
            }
            if (auto lang = menuBar()->findChild<QMenu*>("menuLangue")) {
                lang->setTitle(tr("Langue"));
            }
            if (auto opts = menuBar()->findChild<QMenu*>("menuOptions")) {
                opts->setTitle(tr("Options"));
                if (auto actOpts = this->findChild<QAction*>("actionOptions")) {
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
        // NEW: Retraduire la page Joueur
        if (pageJoueur) {
            // Utiliser le contexte "JoueurPage" pour réutiliser les traductions existantes du fichier .ui
            if (auto g = pageJoueur->findChild<QGroupBox*>("group_player"))
                g->setTitle(QCoreApplication::translate("JoueurPage", "Joueur"));
            if (auto g = pageJoueur->findChild<QGroupBox*>("group_editions"))
                g->setTitle(QCoreApplication::translate("JoueurPage", "Éditions"));
            if (auto g = pageJoueur->findChild<QGroupBox*>("group_yaxis"))
                g->setTitle(QCoreApplication::translate("JoueurPage", "Ordonnée"));
            if (auto g = pageJoueur->findChild<QGroupBox*>("group_list"))
                g->setTitle(QCoreApplication::translate("JoueurPage", "Joueurs ajoutés"));

            if (auto l = pageJoueur->findChild<QLabel*>("label_identifier"))
                l->setText(QCoreApplication::translate("JoueurPage", "Identifiant :"));
            if (auto l = pageJoueur->findChild<QLabel*>("label_from"))
                l->setText(QCoreApplication::translate("JoueurPage", "De :"));
            if (auto l = pageJoueur->findChild<QLabel*>("label_to"))
                l->setText(QCoreApplication::translate("JoueurPage", "À :"));

            if (auto b = pageJoueur->findChild<QPushButton*>("button_useCurrentId"))
                b->setText(QCoreApplication::translate("JoueurPage", "Mon ID"));
            if (auto b = pageJoueur->findChild<QPushButton*>("button_generate"))
                b->setText(QCoreApplication::translate("JoueurPage", "Générer"));
            if (auto b = pageJoueur->findChild<QPushButton*>("button_copy"))
                b->setText(QCoreApplication::translate("JoueurPage", "Copier"));
            if (auto b = pageJoueur->findChild<QPushButton*>("button_addPlayer"))
                b->setText(QCoreApplication::translate("JoueurPage", "Ajouter"));
            if (auto b = pageJoueur->findChild<QPushButton*>("button_removePlayer"))
                b->setText(QCoreApplication::translate("JoueurPage", "Suppr."));
            if (auto b = pageJoueur->findChild<QPushButton*>("button_clear"))
                b->setText(QCoreApplication::translate("JoueurPage", "Effacer"));

            // Top 100 translations
            if (auto l = pageJoueur->findChild<QLabel*>("label_top100"))
                l->setText(QCoreApplication::translate("JoueurPage", "Ou choisir dans le top 100 :"));
            if (auto b = pageJoueur->findChild<QPushButton*>("button_loadTop100"))
                b->setText(QCoreApplication::translate("JoueurPage", "Charger"));
            if (auto b = pageJoueur->findChild<QPushButton*>("button_addFromTop100"))
                b->setText(QCoreApplication::translate("JoueurPage", "Ajouter ce joueur"));
            if (auto e = pageJoueur->findChild<QLineEdit*>("lineEdit_playerId"))
                e->setPlaceholderText(QCoreApplication::translate("JoueurPage", "Entrez l'identifiant..."));
            if (auto c = pageJoueur->findChild<QComboBox*>("combo_top100_players"))
                c->setPlaceholderText(QCoreApplication::translate("JoueurPage", "Sélectionner un joueur..."));
        }

        // NEW: Retraduire l'action Joueur dans le menu
        if (menuBar()) {
            if (auto nav = menuBar()->findChild<QMenu*>("menuNavigation")) {
                if (auto actJoueur = nav->findChild<QAction*>("actionJoueur")) {
                    actJoueur->setText(tr("Joueur"));
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
    if (ui->lineEdit_id && watched == ui->lineEdit_id) {
        if (!ui->lineEdit_id->isReadOnly()) {
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent *ke = static_cast<QKeyEvent *>(event);
                if (ke->key() == Qt::Key_Escape) {
                    updateIdLabelDisplay(); // resets text and readonly
                    return true;
                }
            } else if (event->type() == QEvent::FocusOut) {
                // If it loses focus while editing but wasn't Enter/Escape
                // We use single shot to avoid destroying focus during focus event
                QTimer::singleShot(0, this, &MainWindow::updateIdLabelDisplay);
            }
        }
    }

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

void MainWindow::updateAfkSummaries()
{
    auto updateLbl = [this](QLabel* lbl, QLineEdit* leH, QComboBox* cbM) {
        if (!lbl || !leH) return;
        int h = leH->text().remove(',').toInt();
        int m = cbM ? cbM->currentText().toInt() : 0;
        
        double afkTotal = h + m / 60.0;
        double baseDur = (AppSettings::region == "Jap") ? AppSettings::durationJp : AppSettings::durationGlo;
        
        if (baseDur <= 0) return;
        
        double pct = std::clamp((afkTotal / baseDur) * 100.0, 0.0, 100.0);
        
        int bDurH = static_cast<int>(baseDur);
        int bDurM = qRound((baseDur - bDurH) * 60.0);

        QString text = QString("%1% - %2h%3 sur <a href='#options' style='color:#7f8c8d; text-decoration:none; border-bottom:1px dashed #7f8c8d;'>%4h%5</a>")
                           .arg(QString::number(pct, 'f', 1))
                           .arg(h, 2, 10, QLatin1Char('0'))
                           .arg(m, 2, 10, QLatin1Char('0'))
                           .arg(bDurH, 2, 10, QLatin1Char('0'))
                           .arg(bDurM, 2, 10, QLatin1Char('0'));
        lbl->setText(text);
    };

    updateLbl(m_lblAfkSummary1, ui->lineEdit_afk, ui->combo_afk_minutes);
    updateLbl(m_lblAfkSummary2, ui->lineEdit_afk_2, ui->combo_afk_minutes_2);
}

void MainWindow::on_lineEdit_afk_textEdited(const QString &arg1)
{
    updateAfkSummaries();
    if (m_debounceTimerGoal) {
        m_debounceTimerGoal->start();
    }
}

void MainWindow::doGoalEstimation()
{
    // On veut tout simplement récupérer la valeur et calculer le goal
    // Avoir la nouvelle valeur
    QString arg1 = ui->lineEdit_afk->text();
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

        bool isSimulation = (ui->checkBox_estimation && ui->checkBox_estimation->isChecked());

        if (pointsStr == "-1") {
            if (!isSimulation) {
                std::cout << "Step 5: pointsStr is -1, exiting function." << std::endl;
                ui->label_win_pace->setText("");
                QString color = "red";
                QString comment = tr("On ne peut pas deviner\nvos points actuels !\nGénérez un graphique\nou activez la Simulation.");
                QString style = QString("color: %1; font-size: 14px;").arg(color);
                labeltext->setStyleSheet(style);
                labeltext->setText(comment);
                hasWinPace = false;
                updateGoalOverlayOnGraphs(true);
                return;
            }
        }

        bool pointsOk=true;
        qint64 points = (pointsStr == "-1") ? 0 : QString::fromStdString(pointsStr).toLongLong(&pointsOk);
        if(!pointsOk) {hasWinPace=false;ui->label_win_pace->clear();return;}
        std::cout << "Step 6: Converted pointsStr to integer: " << points << std::endl;

        int seedValue;
        if (seedStr == "-1" || seedStr.empty()) {
            seedValue = 950000; // Realistic WT average points per win fallback
        } else {
            seedValue = QString::fromStdString(seedStr).toInt();
        }
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

        bool goalOk=false;
        const qint64 goalValue = ui->lineEdit_goal->text().remove(',').toLongLong(&goalOk);
        if(!goalOk || goalValue<0) {hasWinPace=false;ui->label_win_pace->clear();return;}
        std::cout << "Step 9: Retrieved goalValue from lineEdit_goal: " << goalValue << std::endl;

        double hours_left_total = remainingTournamentHours();
        if (isSimulation) {
            hours_left_total = (AppSettings::region == "Jap") ? AppSettings::durationJp : AppSettings::durationGlo;
        }
        std::cout << "Step 10: Converted hourStr to float: " << hours_left_total << std::endl;

        if(!isSimulation && points > goalValue) {
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
        double activeHours = hours_left_total - afkTotalHours;
        qint64 currentPoints = points;
        if (ui->checkBox_estimation && ui->checkBox_estimation->isChecked()) {
            activeHours = ((AppSettings::region == "Jap") ? AppSettings::durationJp : AppSettings::durationGlo) - afkTotalHours;
            currentPoints = 0;
        }
        std::cout << "Active hours: " << activeHours << std::endl;

        float winsPerHour = activeHours > 0 ? (goalValue - currentPoints) / (seedValue * static_cast<float>(activeHours)) : 0;
        float totalWins = (goalValue - currentPoints) / static_cast<float>(seedValue);
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

        if (ui->checkBox_estimation && ui->checkBox_estimation->isChecked()) {
            double baseDur = (AppSettings::region == "Jap") ? AppSettings::durationJp : AppSettings::durationGlo;
            comment += tr("\n\n-- Mode Simulation --\nDurée théorique : %1h (%2)").arg(baseDur).arg(AppSettings::region);
            comment += tr("\nVictoires totales estimées : %1").arg(static_cast<int>(totalWins));
        }

        QString style = QString("color: %1; font-size: 47px;").arg(color);
        labelWinPace->setStyleSheet(style);
        labelWinPace->setText(QString::number(winsPerHour, 'f', 2));

        QString style_ = QString("color: %1; font-size: 14px;").arg(color);
        labeltext->setStyleSheet(style_);
        labeltext->setText(comment);

        // NEW: store last computed win pace and update overlay if asked
        lastWinPace = winsPerHour;
        hasWinPace = std::isfinite(lastWinPace) && lastWinPace >= 0.0;
        updateGoalOverlayOnGraphs(false);
    }
}


void MainWindow::on_lineEdit_goal_textEdited(const QString &arg1)
{
    if (m_debounceTimerGoal) {
        m_debounceTimerGoal->start();
    }
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
    auto checkNewUI = content->findChild<QCheckBox*>("checkNewUI"); // NEW: new complete UI
    // NEW: Start Menu UI
    auto lblStartTitle = content->findChild<QLabel*>("labelAddStartMenuTitle");
    auto btnAddStart   = content->findChild<QPushButton*>("buttonAddStartMenu");
    auto lblStartHint  = content->findChild<QLabel*>("labelAddStartMenuHint");
    // NEW: checkbox to update shortcut after updates (optional in UI)
    auto checkUpdateShortcut = content->findChild<QCheckBox*>("checkUpdateStartShortcut");
    // NEW: date format
    auto comboDateFormat = content->findChild<QComboBox*>("comboDateFormat");
    auto labelDatePreview = content->findChild<QLabel*>("labelDatePreview");
    // NEW: duration settings
    auto spinDurGlo = content->findChild<QDoubleSpinBox*>("spinDurGlo");
    auto spinDurJp = content->findChild<QDoubleSpinBox*>("spinDurJp");

    // Initialize from settings
    if (radioGlo && radioJap) {
        if (AppSettings::region == "Jap") radioJap->setChecked(true);
        else radioGlo->setChecked(true);
    }
    auto checkHideNegativeTime = content->findChild<QCheckBox*>("checkHideNegativeTime");
    if (checkHideNegativeTime) checkHideNegativeTime->setChecked(AppSettings::hideNegativeTimes);
    auto checkForceZeroHour = content->findChild<QCheckBox*>("checkForceZeroHour");
    if (checkForceZeroHour) checkForceZeroHour->setChecked(AppSettings::forceZeroHour);
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
    // Initialize duration settings
    if (spinDurGlo) spinDurGlo->setValue(AppSettings::durationGlo);
    if (spinDurJp) spinDurJp->setValue(AppSettings::durationJp);
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

    // Save button logic for durations - capture the widgets found at the start
    if (buttonBox) {
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, [spinDurGlo, spinDurJp]() {
            if (spinDurGlo) AppSettings::durationGlo = spinDurGlo->value();
            if (spinDurJp) AppSettings::durationJp = spinDurJp->value();
            AppSettings::save();
        });
    }
    // NEW: init extra delay from saved settings
    if (spinExtraDelay) {
        spinExtraDelay->setRange(0, 15); // safety (also set in .ui)
        spinExtraDelay->setValue(AppSettings::autoRefreshExtraDelayMinutes);
    }
    // NEW: transparent controls
    if (checkTransparent) {
        checkTransparent->setChecked(AppSettings::transparentControls);
    }
    if (checkNewUI) {
        checkNewUI->setChecked(AppSettings::useNewUI);
    }

    // État: activer seulement si la version est à jour
    if (btnAddStart && lblStartHint) {
        const bool haveLatestInfo = !Updater::latestReleaseName.isEmpty();
        const bool isLatest = Updater::isCurrentLatest;
        btnAddStart->setEnabled(isLatest);
        if (!isLatest) {
            lblStartHint->setText(tr("Vous devez installer la dernière version pour ajouter Polar au menu Démarrer."));
            lblStartHint->setStyleSheet("color:#9aa0a6; font-size:11px;");
        } else {
            lblStartHint->clear();
        }
    }
    // Action "Ajouter"
    if (btnAddStart && lblStartHint) {
        QObject::connect(btnAddStart, &QPushButton::clicked, &dlg, [this, lblStartHint]() {
        #ifdef Q_OS_WIN
            const bool ok = createStartMenuShortcut(QStringLiteral("Polar"));
        #else
            const bool ok = false;
        #endif
            if (ok) {
                lblStartHint->setText(QString::fromUtf8("✔ ") + tr("Ajouté au menu Démarrer."));
                lblStartHint->setStyleSheet("color:#2ECC71; font-size:11px;");
            } else {
                lblStartHint->setText(QString::fromUtf8("✖ ") + tr("Erreur lors de l'ajout au menu Démarrer."));
                lblStartHint->setStyleSheet("color:#E74C3C; font-size:11px;");
            }
        });
    }

    // Initialize from settings (checkbox)
    if (checkUpdateShortcut) {
        checkUpdateShortcut->setChecked(AppSettings::updateStartShortcutOnUpgrade);
    }

    // NEW: init date format
    if (comboDateFormat) {
        comboDateFormat->setCurrentIndex(qBound(0, AppSettings::dateFormatIndex, comboDateFormat->count() - 1));
    }
    // NEW: update preview on change
    auto updateDatePreview = [labelDatePreview, comboDateFormat]() {
        if (!labelDatePreview || !comboDateFormat) return;
        const QDateTime now = QDateTime::currentDateTime();
        QString formatted;
        switch (comboDateFormat->currentIndex()) {
            case 1: formatted = now.toString("dd/MM/yyyy HH:mm"); break;
            case 2: formatted = now.toString("MM/dd/yyyy HH:mm"); break;
            case 3: formatted = now.toString("yyyy-MM-dd HH:mm"); break;
            default: formatted = QLocale().toString(now, QLocale::ShortFormat); break;
        }
        labelDatePreview->setText(QObject::tr("Aperçu : %1").arg(formatted));
    };
    if (comboDateFormat && labelDatePreview) {
        updateDatePreview();
        QObject::connect(comboDateFormat, QOverload<int>::of(&QComboBox::currentIndexChanged),
                         labelDatePreview, updateDatePreview);
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
        if (checkHideNegativeTime) AppSettings::hideNegativeTimes = checkHideNegativeTime->isChecked();
        if (checkForceZeroHour) AppSettings::forceZeroHour = checkForceZeroHour->isChecked();

        if (checkCustom) AppSettings::useCustomBackground = checkCustom->isChecked();
        if (editPath) AppSettings::backgroundPath = editPath->text();
        if (sliderOpacity) AppSettings::backgroundDimPercent = sliderOpacity->value();
        // NEW: save extra delay 0..15
        if (spinExtraDelay) AppSettings::autoRefreshExtraDelayMinutes = qBound(0, spinExtraDelay->value(), 15);
        // NEW: save transparent controls setting
        if (checkTransparent) AppSettings::transparentControls = checkTransparent->isChecked();
        if (checkNewUI) AppSettings::useNewUI = checkNewUI->isChecked();
        // NEW: save shortcut-update preference
        if (checkUpdateShortcut) AppSettings::updateStartShortcutOnUpgrade = checkUpdateShortcut->isChecked();
        // NEW: save date format
        if (comboDateFormat) AppSettings::dateFormatIndex = comboDateFormat->currentIndex();
        AppSettings::save();
        updateAfkSummaries();
        updateRankEstimation();
        if (ui->lineEdit_afk) doGoalEstimation();
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

    // Refresh dates display so relative times stay relevant
    updateTbDatesDisplay();
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
    const auto generation=++rankGeneration;
    const auto region=AppSettings::region;
    const int requestedEdition=AppSettings::selectedEdition;
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
    m_rankProjectedPts=-1; m_rankHistoryEds.clear(); m_rankHistoryPts.clear();
    int baseEd = tbEdition;
    if (baseEd <= 0) {
        lbl->setText(tr("Edition number unavailable"));
        lbl->setToolTip(tr("The API provides dates but no edition number. Historical rank extrapolation is disabled rather than guessing a tournament."));
        if (ui->label_win_pace_rank) ui->label_win_pace_rank->clear();
        hasRankWinPace=false;updateRankOverlayOnGraphs(true);return;
    }

    const bool isJP = (AppSettings::region == "Jap" || AppSettings::region == "JP");
    const int start = isJP ? 56 : 55;

    // Build up to 8 editions for a better trend projection
    QVector<int> eds;
    for (int i = 1; i <= 8; ++i) {
        if (baseEd - i >= start) eds.push_back(baseEd - i);
    }

    m_rankTarget = rank;
    m_rankHistoryEds.clear();
    m_rankHistoryPts.clear();

    QList<QUrl> urls;
    for(int ed:eds) {QUrlQuery q;q.addQueryItem("rank",QString::number(rank));urls.append(WtApi::endpoint(ed,"get-user",region,q));}
    WtApi::instance().getMany(urls,this,[this,eds,urls,rank,baseEd,lbl,generation,region,requestedEdition](const QHash<QUrl,WtApi::Result> &results) {
    if(generation!=rankGeneration || region!=AppSettings::region || requestedEdition!=AppSettings::selectedEdition) return;
    double rankAvgSeed = -1.0;
    
    for (int ed : eds) {
        qint64 lastPts = -1;
        const auto response=results.value(urls.at(eds.indexOf(ed)));
        const QJsonObject obj = response.error.isEmpty()?WtData::normalize(QJsonDocument::fromJson(response.bytes)):QJsonObject();
        if (!obj.isEmpty()) {
            if (obj.contains(QStringLiteral("points"))) {
                const QString ptsStr = obj.value(QStringLiteral("points")).toString().remove('[').remove(']');
                const QStringList vals = ptsStr.split(',', Qt::SkipEmptyParts);
                if (!vals.isEmpty()) {
                    bool okNum = false;
                    lastPts = vals.last().trimmed().toLongLong(&okNum);
                    if (!okNum) lastPts = -1;
                }
            }
            if (rankAvgSeed < 0 && obj.contains(QStringLiteral("points_wins"))) {
                const QString pwStr = obj.value(QStringLiteral("points_wins")).toString().remove('[').remove(']');
                const QStringList pwVals = pwStr.split(',', Qt::SkipEmptyParts);
                if (!pwVals.isEmpty()) {
                    bool okNum = false;
                    double pw = pwVals.last().trimmed().toDouble(&okNum);
                    if (okNum && pw > 0.0) rankAvgSeed = pw;
                }
            }
        }
        
        if (lastPts >= 0) {
            // Append to our history lists backwards so it's oldest first eventually
            m_rankHistoryEds.push_front(ed);
            m_rankHistoryPts.push_front(lastPts);
        }
    }

    // Mathematical Linear Regression
    qint64 estimatedPoints = -1;
    if (!m_rankHistoryPts.isEmpty()) {
        QVector<int> historyEditions;
        for(qint64 ed:m_rankHistoryEds) historyEditions.append(int(ed));
        const double projection=Performance::historicalProjection(historyEditions,m_rankHistoryPts,baseEd);
        if(std::isfinite(projection) && projection<double(std::numeric_limits<qint64>::max()))
            estimatedPoints=static_cast<qint64>(projection);

        m_rankProjectedEd = baseEd;
        m_rankProjectedPts = estimatedPoints;

        // Modern visual card (HTML/CSS)
        QString html = QString(
            "<div style='border:1px solid rgba(128, 128, 128, 80); border-radius:4px; padding:14px 6px 8px 6px; color:inherit; text-align:center;'>"
            "  <div style='font-size:24px; font-weight:bold; color:#1a73e8; margin-bottom:2px;'>%1</div>"
            "  <div style='margin-top:4px; font-size:11px;'>"
            "    <a href=\"show_rank_analysis\" style='color:#1a73e8; text-decoration:underline;'>%2 ➔</a>"
            "  </div>"
            "</div>"
        ).arg(formatMillionsCompact(estimatedPoints)).arg(tr("Statistiques détaillées"));
        
        lbl->setOpenExternalLinks(false);
        lbl->setText(html);
    } else {
        lbl->setText(tr("Impossible d'obtenir\nles données"));
    }

    // Compute wins/hour needed to reach estimated points (same as Points tab logic)
    if (ui->label_win_pace_rank) {
        // prerequisites
        if (estimatedPoints <= 0) {
            ui->label_win_pace_rank->clear();
            hasRankWinPace = false;
            updateRankOverlayOnGraphs(true);
            return;
        }
        bool isSimulation = (ui->checkBox_estimation && ui->checkBox_estimation->isChecked());

        // Current variables from functb
        if (functb::points == "-1") {
            if (!isSimulation) {
                ui->label_win_pace_rank->clear();
                hasRankWinPace = false;
                updateRankOverlayOnGraphs(true);
                // Also update the estimation label to show the error
                if (ui->label_estimation_rank) {
                    ui->label_estimation_rank->setStyleSheet("color: red; font-size: 14px;");
                    ui->label_estimation_rank->setText(tr("On ne peut pas deviner\nvos points actuels !\nGénérez un graphique\nou activez la Simulation."));
                }
                return;
            }
        }
        
        bool okP=true, okH=true;
        const qint64 curPoints = (functb::points == "-1") ? 0 : QString::fromStdString(functb::points).toLongLong(&okP);
        
        // Take the server response multiplier (points_wins) instead of 30 if available.
        // Fallback to functb::seed if possible, otherwise use a realistic tournament average (e.g. 950000).
        bool okS=true;
        int parsedSeed = QString::fromStdString(functb::seed).toInt(&okS);
        int seedValue;
        if (rankAvgSeed > 0.0) {
            seedValue = static_cast<int>(rankAvgSeed);
        } else if (functb::seed != "-1" && !functb::seed.empty() && okS) {
            seedValue = parsedSeed;
        } else {
            seedValue = 950000; // Realistic WT fallback
        }
        
        const double hours_left_total = remainingTournamentHours();
        
        // If simulation, we don't care about okP and okH since they're defaulting.
        if (!isSimulation && (!okP || !okH || seedValue == 0)) {
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
        
        double activeHours = hours_left_total - afkTotalHours;
        if (ui->checkBox_estimation && ui->checkBox_estimation->isChecked()) {
            // If estimation is independent of current state, we use the configured theoretical duration.
            if (AppSettings::region == "Jap") {
                activeHours = AppSettings::durationJp - afkTotalHours;
            } else {
                activeHours = AppSettings::durationGlo - afkTotalHours;
            }
        }

        if (activeHours <= 0.0) {
            // Impossible: show same red text as Points tab
            lbl->setStyleSheet("color: red;");
            lbl->setText(tr("Impossible"));
            ui->label_win_pace_rank->clear();
            hasRankWinPace = false;
            updateRankOverlayOnGraphs(true);
            return;
        }

        // Target delta: in simulation mode, we start from 0 points. Otherwise, we start from current points.
        qint64 targetDelta = 0;
        if (ui->checkBox_estimation && ui->checkBox_estimation->isChecked()) {
            targetDelta = estimatedPoints;
        } else {
            targetDelta = static_cast<qint64>(estimatedPoints) - static_cast<qint64>(curPoints);
        }

        if (targetDelta <= 0) {
            ui->label_win_pace_rank->setText(QStringLiteral("0.00"));
            ui->label_win_pace_rank->setStyleSheet("color: green; font-size: 47px;");
            lastRankWinPace = 0.0;
            hasRankWinPace = true;
            updateRankOverlayOnGraphs(false);
            return;
        }

        // NEW: use seedValue (points average per win) instead of edition deltas
        double pointsPerStep = static_cast<double>(seedValue);

        const double winsPerHour = (pointsPerStep > 0) ? ((static_cast<double>(targetDelta) / pointsPerStep) / activeHours) : 0.0;

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

        // Update info label for simulation mode
        if (ui->label_estimation_rank) {
            // Retrieve previously set HTML text
            QString currentText = ui->label_estimation_rank->text();
            
            if (ui->checkBox_estimation && ui->checkBox_estimation->isChecked()) {
                double baseDur = (AppSettings::region == "Jap") ? AppSettings::durationJp : AppSettings::durationGlo;
                double totalWins = (pointsPerStep > 0) ? (static_cast<double>(targetDelta) / pointsPerStep) : 0.0;
                
                currentText += QString("<div style='margin-top:6px; font-size:11px; opacity:0.8;'>");
                currentText += tr("Simulation: %1").arg(static_cast<int>(totalWins)) + " victoires";
                currentText += QString("</div>");
            }
            ui->label_estimation_rank->setText(currentText);
        }
    }
    });
}

// Ensure the checkbox trigger is present to refresh the calculation immediately
void MainWindow::on_checkBox_estimation_toggled(bool checked)
{
    updateRankEstimation();
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    // If user enables the overlay but we don't have a computed pace yet, compute it now
    if (checked && !hasWinPace) {
        if (ui && ui->lineEdit_afk) {
            doGoalEstimation();
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
    const double maxX = tbEndEpoch > tbStartEpoch ? (tbEndEpoch - tbStartEpoch) / 3600.0 : ((AppSettings::region == "Jap" || AppSettings::region == "JP") ? AppSettings::durationJp : AppSettings::durationGlo);

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
    const int edition=AppSettings::selectedEdition;
    const QString region=AppSettings::region;
    const auto generation=++metadataGeneration;
    ++graphGeneration;++rankGeneration;
    tbStartEpoch=tbEndEpoch=0;tbEdition=0;
    hasWinPace=hasRankWinPace=false;
    functb::points=functb::wins=functb::seed="-1";
    functb::hour_missing="-1";
    if(tbTitleLabel) tbTitleLabel->setText(edition==0?tr("Current WT"):tr("%1ème Tenkaichi Budokai").arg(edition));
    if(tbProgressBar) {tbProgressBar->setValue(0);tbProgressBar->setFormat("—");}
    ui->label_time_left->setText(tr("Loading…"));ui->wt_date->clear();
    ui->label_estimation_rank->clear();ui->label_win_pace_rank->clear();
    ui->label_7->clear();ui->label_win_pace->clear();
    // Never leave a chart from a different edition/region labeled as current.
    for(auto *view:{ui->graphiqueTest,Leaderboard::graphPlaceholder}) if(view && view->scene()) view->scene()->clear();
    if(Leaderboard::playerListPtr) Leaderboard::playerListPtr->clear();
    Leaderboard::snapshotRows.clear();Leaderboard::overlayNames.clear();Leaderboard::currentSelectedName.clear();
    WtApi::instance().get(WtApi::endpoint(edition,"metadata",region),this,
        [this,edition,region,generation](const QByteArray &bytes,const QString &error) {
            if(generation!=metadataGeneration || edition!=AppSettings::selectedEdition || region!=AppSettings::region) return;
            const auto m=WtData::metadata(QJsonDocument::fromJson(bytes));
            if(!error.isEmpty() || m.isEmpty()) {ui->label_time_left->setText(tr("Metadata unavailable"));return;}
            tbStartEpoch=m.value("start_at").toVariant().toLongLong();
            tbEndEpoch=m.value("end_at").toVariant().toLongLong();
            tbEdition=m.value("id").toInt(edition);
            if(tbTitleLabel && tbEdition>0) tbTitleLabel->setText(tr("%1ème Tenkaichi Budokai").arg(tbEdition));
            updateTbUiFromTimes();updateTbDatesDisplay();updateRankEstimation();
        },300);
}

void MainWindow::updateTbDatesDisplay()
{
    if (!ui || !ui->wt_date) return;
    if (tbStartEpoch <= 0 || tbEndEpoch <= 0) {
        ui->wt_date->clear();
        return;
    }

    const QDateTime startDt = QDateTime::fromSecsSinceEpoch(tbStartEpoch, Qt::LocalTime);
    const QDateTime endDt = QDateTime::fromSecsSinceEpoch(tbEndEpoch, Qt::LocalTime);

    // NEW: format according to setting
    QString startStr, endStr;
    switch (AppSettings::dateFormatIndex) {
        case 1:
            startStr = startDt.toString("dd/MM/yyyy HH:mm");
            endStr = endDt.toString("dd/MM/yyyy HH:mm");
            break;
        case 2:
            startStr = startDt.toString("MM/dd/yyyy HH:mm");
            endStr = endDt.toString("MM/dd/yyyy HH:mm");
            break;
        case 3:
            startStr = startDt.toString("yyyy-MM-dd HH:mm");
            endStr = endDt.toString("yyyy-MM-dd HH:mm");
            break;
        default: {
            const QLocale locale;
            startStr = locale.toString(startDt, QLocale::ShortFormat);
            endStr = locale.toString(endDt, QLocale::ShortFormat);
            break;
        }
    }

    auto formatRelative = [](const QDateTime& dt) -> QString {
        auto now = QDateTime::currentDateTime();
        qint64 secs = now.secsTo(dt);
        bool inPast = (secs < 0);
        secs = qAbs(secs);
        
        int days = secs / 86400;
        int hours = (secs % 86400) / 3600;
        
        if (days > 30) {
            int months = days / 30;
            int remDays = days % 30;
            QString res = tr("%1 mois").arg(months);
            if (remDays > 0) res += tr(", %1 jours").arg(remDays);
            return inPast ? tr("Il y a %1").arg(res) : tr("Dans %1").arg(res);
        } else if (days > 0) {
            QString res = tr("%1 jours").arg(days);
            if (hours > 0) res += tr(", %1 heures").arg(hours);
            return inPast ? tr("Il y a %1").arg(res) : tr("Dans %1").arg(res);
        } else if (hours > 0) {
            int mins = (secs % 3600) / 60;
            return inPast ? tr("Il y a %1 h %2 min").arg(hours).arg(mins) 
                          : tr("Dans %1 h %2 min").arg(hours).arg(mins);
        } else {
            int mins = secs / 60;
            return inPast ? tr("Il y a %1 min").arg(mins) : tr("Dans %1 min").arg(mins);
        }
    };

    QString startRel = formatRelative(startDt);
    QString endRel = formatRelative(endDt);

    QString html = QString(
        "<div style='text-align: center;'>"
        "  <span style='font-size:12px;'>" + tr("Du <b>%1</b> au <b>%2</b>") + "</span><br/>"
        "  <span style='font-size:10px; color:rgba(128, 128, 128, 200);'>"
        "    <i>" + tr("Début : %3 &nbsp;&nbsp;|&nbsp;&nbsp; Fin : %4") + "</i>"
        "  </span>"
        "</div>"
    ).arg(startStr).arg(endStr).arg(startRel).arg(endRel);

    ui->wt_date->setText(html);
}

// NEW: rebuild localized title/time using cached metadata (no refetch)
void MainWindow::refreshTbLocalizedTexts()
{
    if (tbTitleLabel && tbEdition > 0) {
        tbTitleLabel->setText(tr("%1ème Tenkaichi Budokai").arg(tbEdition));
    }
    updateTbUiFromTimes();
    updateTbDatesDisplay();
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
    
    // Apply UI DA dynamically to all specific group boxes
    QString osThemeStyles = "";
    if (AppSettings::useNewUI) {
        QString bgStyle = transparentControls ? "rgba(128, 128, 128, 20)" : "palette(window)";
        QString paneBg = transparentControls ? "transparent" : "palette(window)";
        QString btnStyle = transparentControls ? "rgba(128, 128, 128, 30)" : "palette(button)";
        QString hoverStyle = transparentControls ? "rgba(128, 128, 128, 50)" : "palette(light)";
        QString pressedStyle = transparentControls ? "rgba(128, 128, 128, 70)" : "palette(mid)";

        osThemeStyles = 
            "QGroupBox { background: " + paneBg + "; border: 1px solid rgba(128, 128, 128, 60); border-radius: 4px; margin-top: 18px; font-weight: bold; } "
            "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 5px; } "
            "QTabWidget::pane { background: " + paneBg + "; border: 1px solid rgba(128, 128, 128, 60); border-top-right-radius: 4px; border-bottom-left-radius: 4px; border-bottom-right-radius: 4px; top: -1px; } "
            "QTabWidget > QWidget { background: transparent; } "
            "QTabBar::tab { background: " + bgStyle + "; padding: 5px 12px; border: 1px solid rgba(128, 128, 128, 60); border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; } "
            "QTabBar::tab:selected { background: " + (transparentControls ? "transparent" : "palette(window)") + "; border-top: 2px solid #1a73e8; border-bottom: 3px solid " + (transparentControls ? "rgba(128, 128, 128, 0)" : "palette(window)") + "; margin-bottom: -2px; } "
            "QTabBar::tab:hover:!selected { background: " + hoverStyle + "; } "
            "QLineEdit, QComboBox { background: " + (transparentControls ? bgStyle : "palette(base)") + "; border: 1px solid rgba(128, 128, 128, 60); border-radius: 2px; padding: 4px; } "
            "QLineEdit[readOnly=\"false\"] { background: palette(base); border: 1px solid #1a73e8; } " // Better UX feedback for edit mode
            "QLineEdit:focus:!readOnly, QComboBox:focus { border: 1px solid #1a73e8; } "
            "QPushButton { background: " + btnStyle + "; border: 1px solid rgba(128, 128, 128, 60); border-radius: 2px; padding: 5px 12px; font-weight: bold; } "
            "QPushButton:hover { background: " + hoverStyle + "; border: 1px solid #1a73e8; } "
            "QPushButton:pressed { background: " + pressedStyle + "; } ";
    }

    if (ui->goal) ui->goal->setStyleSheet(osThemeStyles);
    QGroupBox *idBox = findChild<QGroupBox*>("groupBox_2");
    if (idBox) idBox->setStyleSheet(osThemeStyles);
    QGroupBox *grBox = findChild<QGroupBox*>("groupBox");
    if (grBox) grBox->setStyleSheet(osThemeStyles);
    QGroupBox *coBox = findChild<QGroupBox*>("groupBox_3");
    if (coBox) coBox->setStyleSheet(osThemeStyles);
    QGroupBox *edBox = findChild<QGroupBox*>("groupBox_4");
    if (edBox) edBox->setStyleSheet(osThemeStyles);
    
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
    const QString realId = QString::fromStdString(functb::identifier).trimmed();
    
    if (ui->lineEdit_id) {
        ui->lineEdit_id->setReadOnly(true);
        ui->lineEdit_id->clearFocus();
        ui->lineEdit_id->style()->unpolish(ui->lineEdit_id);
        ui->lineEdit_id->style()->polish(ui->lineEdit_id);
        ui->lineEdit_id->setText(realId);
        
        // Mode censure = Password dots, sinon Normal
        if (AppSettings::censorIdDisplay) {
            ui->lineEdit_id->setEchoMode(QLineEdit::Password);
            
            // Eye icon for toggling visibility
            bool hasEye = false;
            for(QAction* act : ui->lineEdit_id->actions()) {
                if (act->property("isEyeIcon").toBool()) {
                    hasEye = true;
                    act->setVisible(false); // only visible when editing
                    
                    QPixmap pix(20, 20); pix.fill(Qt::transparent);
                    QPainter p(&pix); p.setRenderHint(QPainter::Antialiasing);
                    p.setPen(QPen(Qt::darkGray, 1.5)); p.setBrush(Qt::NoBrush);
                    p.drawEllipse(QRectF(2, 6, 16, 8)); p.drawEllipse(QRectF(7, 7, 6, 6));
                    act->setIcon(QIcon(pix));
                    break;
                }
            }
            if(!hasEye) {
                QAction *eyeAction = ui->lineEdit_id->addAction(QIcon(), QLineEdit::TrailingPosition);
                eyeAction->setProperty("isEyeIcon", true);
                eyeAction->setVisible(false); // only visible when editing
                
                QPixmap pix(20, 20); pix.fill(Qt::transparent);
                QPainter p(&pix); p.setRenderHint(QPainter::Antialiasing);
                p.setPen(QPen(Qt::darkGray, 1.5)); p.setBrush(Qt::NoBrush);
                p.drawEllipse(QRectF(2, 6, 16, 8)); p.drawEllipse(QRectF(7, 7, 6, 6)); 
                eyeAction->setIcon(QIcon(pix));
                
                connect(eyeAction, &QAction::triggered, this, [this, eyeAction]() {
                    if (ui->lineEdit_id->echoMode() == QLineEdit::Password) {
                        ui->lineEdit_id->setEchoMode(QLineEdit::Normal);
                        QPixmap pix2(20, 20); pix2.fill(Qt::transparent); QPainter p2(&pix2);
                        p2.setRenderHint(QPainter::Antialiasing); p2.setPen(QPen(Qt::darkGray, 1.5)); p2.setBrush(Qt::NoBrush);
                        p2.drawEllipse(QRectF(2, 6, 16, 8)); p2.drawEllipse(QRectF(7, 7, 6, 6));
                        p2.setPen(QPen(Qt::red, 1.5)); p2.drawLine(2, 18, 18, 2);
                        eyeAction->setIcon(QIcon(pix2));
                    } else {
                        ui->lineEdit_id->setEchoMode(QLineEdit::Password);
                        QPixmap pix2(20, 20); pix2.fill(Qt::transparent); QPainter p2(&pix2);
                        p2.setRenderHint(QPainter::Antialiasing); p2.setPen(QPen(Qt::darkGray, 1.5)); p2.setBrush(Qt::NoBrush);
                        p2.drawEllipse(QRectF(2, 6, 16, 8)); p2.drawEllipse(QRectF(7, 7, 6, 6));
                        eyeAction->setIcon(QIcon(pix2));
                    }
                });
            }
        } else {
            ui->lineEdit_id->setEchoMode(QLineEdit::Normal);
            for(QAction* act : ui->lineEdit_id->actions()) {
                if (act->property("isEyeIcon").toBool()) {
                    ui->lineEdit_id->removeAction(act);
                    act->deleteLater();
                }
            }
        }
        
        // One-time connections
        if(ui->lineEdit_id->property("connected").isNull()) {
            connect(ui->lineEdit_id, &QLineEdit::returnPressed, this, [this]() {
                QString text = ui->lineEdit_id->text().trimmed();
                if(!text.isEmpty()) {
                    AppSettings::savedIdentifier = text;
                    AppSettings::save();
                    functb::identifier = text.toStdString();
                }
                updateIdLabelDisplay();
            });
            
            // To properly intercept Escape key and outside clicks natively without side effects
            ui->lineEdit_id->installEventFilter(this);
            ui->lineEdit_id->setProperty("connected", true);
        }
    }
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

// NEW: copy context-aware (Graphs or Classement or Joueur)
void MainWindow::copyAnyGraphToClipboard()
{
    if (!stackedWidget) return;
    const int idx = stackedWidget->currentIndex();
    if (idx == 1) {
        // Classement page
        copyClassementGraphToClipboard();
        return;
    }
    if (idx == 2) {
        // Joueur page
        onJoueurCopyClicked();
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
            doGoalEstimation();
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
            const double maxX = tbEndEpoch > tbStartEpoch ? (tbEndEpoch - tbStartEpoch) / 3600.0 : ((AppSettings::region == "Jap" || AppSettings::region == "JP") ? AppSettings::durationJp : AppSettings::durationGlo);
            const auto hAxes = chart->axes(Qt::Horizontal);
            if (!hAxes.isEmpty()) hAxes.first()->setRange(minX, maxX);
        }
        return;
    }

    // Clamp X to [0; 71.75]
    const double minX = 0.0;
    const double maxX = tbEndEpoch > tbStartEpoch ? (tbEndEpoch - tbStartEpoch) / 3600.0 : ((AppSettings::region == "Jap" || AppSettings::region == "JP") ? AppSettings::durationJp : AppSettings::durationGlo);
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
    const auto region=AppSettings::region;
    if(tbPicker) tbPicker->setEditions({0},0);
    WtApi::instance().get(QUrl("https://dokkan-wt.info/older_editions"),this,
        [this,region](const QByteArray &bytes,const QString &error) {
            if(region!=AppSettings::region) return;
            auto list=WtData::editions(bytes,region);list.prepend(0);
            if(AppSettings::selectedEdition>0 && !list.contains(AppSettings::selectedEdition)) list.append(AppSettings::selectedEdition);
            if(tbPicker) {
                tbPicker->setEditions(list,AppSettings::selectedEdition);
                if(!error.isEmpty()) tbPicker->setToolTip(tr("Archive list unavailable. Current remains accessible."));
            }
            populateJoueurEditionCombos();populateJoueurTop100Editions();
        },3600);
}

// Progress UI slots
void MainWindow::onUpdateDownloadStarted(qint64 totalBytes)
{
    if (!updateDlg) {
        updateDlg = new QProgressDialog(tr("Préparation du téléchargement..."), QString(), 0, 100, this);
        updateDlg->setWindowTitle(tr("Téléchargement de la mise à jour"));
        updateDlg->setCancelButton(nullptr);
        updateDlg->setWindowModality(Qt::ApplicationModal);
        updateDlg->setMinimumDuration(0);
        updateDlg->setAutoClose(false);
        updateDlg->setAutoReset(false);
    }
    const auto human = [](qint64 b)->QString {
        const double db = static_cast<double>(b);
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1024*1024) return QString::number(db/1024.0, 'f', 1) + " KB";
        if (b < 1024ll*1024ll*1024ll) return QString::number(db/1048576.0, 'f', 1) + " MB";
        return QString::number(db/1073741824.0, 'f', 2) + " GB";
    };
    const int maxKiB = totalBytes > 0 ? static_cast<int>(qMin<qint64>(totalBytes / 1024, INT_MAX)) : 0;
    updateDlg->setRange(0, qMax(1, maxKiB));
    updateDlg->setValue(0);
    updateDlg->setLabelText(tr("Téléchargement... 0 / %1").arg(human(totalBytes)));
    updateDlg->show();
}

void MainWindow::onUpdateDownloadProgress(qint64 receivedBytes, qint64 totalBytes, double speedBytesPerSec, qint64 etaSecs)
{
    if (!updateDlg) {
        onUpdateDownloadStarted(totalBytes);
        if (!updateDlg) return;
    }
    const auto human = [](qint64 b)->QString {
        const double db = static_cast<double>(b);
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1024*1024) return QString::number(db/1024.0, 'f', 1) + " KB";
        return QString::number(db/1048576.0, 'f', 1) + " MB";
    };
    const auto humanSpeed = [](double bps)->QString {
        if (bps < 1024.0) return QString::number(bps, 'f', 0) + " B/s";
        if (bps < 1024.0*1024.0) return QString::number(bps/1024.0, 'f', 1) + " KB/s";
        return QString::number(bps/1048576.0, 'f', 1) + " MB/s";
    };
    const auto humanEta = [](qint64 s)->QString {
        qint64 h = s / 3600; s %= 3600;
        qint64 m = s / 60;   qint64 sec = s % 60;
        if (h > 0) return QString("%1:%2:%3").arg(h).arg(m,2,10,QLatin1Char('0')).arg(sec,2,10,QLatin1Char('0'));
        return QString("%1:%2").arg(m).arg(sec,2,10,QLatin1Char('0'));
    };

    const int maxKiB = totalBytes > 0 ? static_cast<int>(qMin<qint64>(totalBytes / 1024, INT_MAX)) : 0;
    const int valKiB = static_cast<int>(qMin<qint64>(receivedBytes / 1024, maxKiB));
    if (updateDlg->maximum() != qMax(1, maxKiB)) updateDlg->setRange(0, qMax(1, maxKiB));
    updateDlg->setValue(qBound(0, valKiB, updateDlg->maximum()));
    updateDlg->setLabelText(tr("Téléchargement... %1 / %2 — %3 — ETA %4")
                            .arg(human(receivedBytes),
                                 human(totalBytes),
                                 humanSpeed(speedBytesPerSec),
                                 humanEta(etaSecs)));
}

void MainWindow::onUpdateDownloadFinished(const QString& filePath, bool ok, const QString& errorString)
{
    if (!updateDlg) return;
    if (ok) {
        updateDlg->setValue(updateDlg->maximum());
        updateDlg->setLabelText(tr("Téléchargement terminé."));
    } else {
        updateDlg->setLabelText(tr("Erreur de téléchargement: %1").arg(errorString));
    }
    // Let the dialog close; the app will quit right after (updater starts the new exe)
    QTimer::singleShot(300, updateDlg, [this](){
        if (updateDlg) { updateDlg->hide(); updateDlg->deleteLater(); updateDlg = nullptr; }
    });
}

// NEW (Windows): créer un .lnk dans le Start Menu (par-utilisateur)
#ifdef Q_OS_WIN
static bool createStartMenuShortcut(const QString& displayName)
{
    const QString target = QCoreApplication::applicationFilePath();
    const QString startMenuDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (target.isEmpty() || startMenuDir.isEmpty()) return false;
    const QString linkPath = QDir(startMenuDir).filePath(displayName + QStringLiteral(".lnk"));

    HRESULT hr = CoInitialize(nullptr);
    const bool didCoInit = SUCCEEDED(hr);
    IShellLinkW* psl = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&psl));
    if (FAILED(hr) || !psl) { if (didCoInit) CoUninitialize(); return false; }

    psl->SetPath(reinterpret_cast<LPCWSTR>(target.utf16()));
    psl->SetDescription(reinterpret_cast<LPCWSTR>(QStringLiteral("Polar").utf16()));
    psl->SetIconLocation(reinterpret_cast<LPCWSTR>(target.utf16()), 0);

    IPersistFile* ppf = nullptr;
    hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
    if (FAILED(hr) || !ppf) { psl->Release(); if (didCoInit) CoUninitialize(); return false; }

    // Save the shortcut
    hr = ppf->Save(reinterpret_cast<LPCWSTR>(QString(linkPath).replace('/', '\\').utf16()), TRUE);
    ppf->Release();
    psl->Release();
    if (didCoInit) CoUninitialize();
    return SUCCEEDED(hr);
}
#endif

// ============================================================================
// NEW: Joueur page implementation (multi-player comparison)
// ============================================================================

void MainWindow::setupJoueurPage()
{
    if (!pageJoueur) return;

    joueurGraphView = pageJoueur->findChild<QGraphicsView*>("view_graph");
    joueurIdEdit = pageJoueur->findChild<QLineEdit*>("lineEdit_playerId");
    joueurPlayersList = pageJoueur->findChild<QListWidget*>("list_players");
    joueurYDataCombo = pageJoueur->findChild<QComboBox*>("combo_ydata");
    joueurEditionStartCombo = pageJoueur->findChild<QComboBox*>("combo_editionStart");
    joueurEditionEndCombo = pageJoueur->findChild<QComboBox*>("combo_editionEnd");
    joueurStatusLabel = pageJoueur->findChild<QLabel*>("label_status");
    joueurCopyConfirmLabel = pageJoueur->findChild<QLabel*>("label_copy_confirm");
    joueurGenerateBtn = pageJoueur->findChild<QPushButton*>("button_generate");
    joueurCopyBtn = pageJoueur->findChild<QPushButton*>("button_copy");
    joueurAddPlayerBtn = pageJoueur->findChild<QPushButton*>("button_addPlayer");
    joueurRemovePlayerBtn = pageJoueur->findChild<QPushButton*>("button_removePlayer");
    joueurClearBtn = pageJoueur->findChild<QPushButton*>("button_clear");

    // NEW: top 100 widgets
    joueurTop100RegionCombo = pageJoueur->findChild<QComboBox*>("combo_top100_region");
    joueurTop100EditionCombo = pageJoueur->findChild<QComboBox*>("combo_top100_edition");
    joueurLoadTop100Btn = pageJoueur->findChild<QPushButton*>("button_loadTop100");
    joueurTop100PlayersCombo = pageJoueur->findChild<QComboBox*>("combo_top100_players");
    joueurAddFromTop100Btn = pageJoueur->findChild<QPushButton*>("button_addFromTop100");

    auto btnUseCurrentId = pageJoueur->findChild<QPushButton*>("button_useCurrentId");

    if (joueurGenerateBtn) {
        connect(joueurGenerateBtn, &QPushButton::clicked, this, &MainWindow::onJoueurGenerateClicked);
    }
    if (joueurCopyBtn) {
        connect(joueurCopyBtn, &QPushButton::clicked, this, &MainWindow::onJoueurCopyClicked);
    }
    if (btnUseCurrentId) {
        connect(btnUseCurrentId, &QPushButton::clicked, this, &MainWindow::onJoueurUseCurrentId);
    }
    if (joueurAddPlayerBtn) {
        connect(joueurAddPlayerBtn, &QPushButton::clicked, this, &MainWindow::onJoueurAddPlayer);
    }
    if (joueurRemovePlayerBtn) {
        connect(joueurRemovePlayerBtn, &QPushButton::clicked, this, &MainWindow::onJoueurRemovePlayer);
    }
    if (joueurClearBtn) {
        connect(joueurClearBtn, &QPushButton::clicked, this, &MainWindow::onJoueurClearAll);
    }
    if (joueurPlayersList) {
        connect(joueurPlayersList, &QListWidget::itemSelectionChanged, this, &MainWindow::onJoueurPlayerSelectionChanged);
    }

    // NEW: top 100 connections
    if (joueurTop100RegionCombo) {
        connect(joueurTop100RegionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::populateJoueurTop100Editions);
    }
    if (joueurLoadTop100Btn) {
        connect(joueurLoadTop100Btn, &QPushButton::clicked, this, &MainWindow::onJoueurLoadTop100);
    }
    if (joueurTop100PlayersCombo) {
        connect(joueurTop100PlayersCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) {
            if (joueurAddFromTop100Btn) {
                joueurAddFromTop100Btn->setEnabled(idx >= 0 && joueurTop100PlayersCombo->count() > 0);
            }
        });
    }
    if (joueurAddFromTop100Btn) {
        connect(joueurAddFromTop100Btn, &QPushButton::clicked, this, &MainWindow::onJoueurAddFromTop100);
    }

    // NEW: activer le bouton Ajouter quand du texte est saisi
    if (joueurIdEdit && joueurAddPlayerBtn) {
        connect(joueurIdEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            joueurAddPlayerBtn->setEnabled(!text.trimmed().isEmpty());
        });
        // État initial
        joueurAddPlayerBtn->setEnabled(!joueurIdEdit->text().trimmed().isEmpty());
    }

    populateJoueurEditionCombos();
    populateJoueurTop100Editions();
}

void MainWindow::populateJoueurEditionCombos()
{
    if(!joueurEditionStartCombo || !joueurEditionEndCombo) return;
    const auto region=AppSettings::region;
    joueurEditionStartCombo->clear();joueurEditionEndCombo->clear();
    joueurEditionStartCombo->addItem(tr("Current"),0);joueurEditionEndCombo->addItem(tr("Current"),0);
    WtApi::instance().get(QUrl("https://dokkan-wt.info/older_editions"),this,
        [this,region](const QByteArray &bytes,const QString &) {
            if(region!=AppSettings::region) return;
            for(auto *combo:{joueurEditionStartCombo,joueurEditionEndCombo}) {
                const int selected=combo->currentData().toInt();combo->clear();combo->addItem(tr("Current"),0);
                for(int ed:WtData::editions(bytes,region)) combo->addItem(QString::number(ed),ed);
                combo->setCurrentIndex(qMax(0,combo->findData(selected)));
            }
        },3600);
}

void MainWindow::populateJoueurEditions()
{
    populateJoueurEditionCombos();
}

void MainWindow::populateJoueurTop100Editions()
{
    if(!joueurTop100EditionCombo || !joueurTop100RegionCombo) return;
    const auto region=joueurTop100RegionCombo->currentText();
    joueurTop100EditionCombo->clear();joueurTop100EditionCombo->addItem(tr("Current"),0);
    if(joueurTop100PlayersCombo) {joueurTop100PlayersCombo->clear();joueurTop100PlayersCombo->setEnabled(false);}
    if(joueurAddFromTop100Btn) joueurAddFromTop100Btn->setEnabled(false);
    WtApi::instance().get(QUrl("https://dokkan-wt.info/older_editions"),this,
        [this,region](const QByteArray &bytes,const QString &) {
            if(region!=joueurTop100RegionCombo->currentText()) return;
            const int selected=joueurTop100EditionCombo->currentData().toInt();
            joueurTop100EditionCombo->clear();joueurTop100EditionCombo->addItem(tr("Current"),0);
            for(int ed:WtData::editions(bytes,region)) joueurTop100EditionCombo->addItem(QString::number(ed),ed);
            joueurTop100EditionCombo->setCurrentIndex(qMax(0,joueurTop100EditionCombo->findData(selected)));
        },3600);
}

void MainWindow::onJoueurLoadTop100()
{
    if (!joueurTop100RegionCombo || !joueurTop100EditionCombo || !joueurTop100PlayersCombo) return;

    joueurTop100PlayersCombo->clear();
    joueurTop100PlayersCombo->setEnabled(false);
    if (joueurAddFromTop100Btn) joueurAddFromTop100Btn->setEnabled(false);

    const QString region = joueurTop100RegionCombo->currentText();
    const int edition = joueurTop100EditionCombo->currentData().toInt();

    if (edition < 0) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            joueurStatusLabel->setText(tr("Édition invalide."));
        }
        return;
    }

    const auto generation=++top100Generation;
    WtApi::instance().get(WtApi::endpoint(edition,"get-top100",region),this,
        [this,region,edition,generation](const QByteArray &bytes,const QString &error) {
    if(generation!=top100Generation || region!=joueurTop100RegionCombo->currentText() || edition!=joueurTop100EditionCombo->currentData().toInt()) return;
    auto ladder=WtData::normalize(QJsonDocument::fromJson(bytes),"top");
    if(!error.isEmpty() || ladder.isEmpty()) ladder["error"]=error.isEmpty()?tr("Invalid response"):error;
    if (ladder.contains("error")) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            joueurStatusLabel->setText(tr("Erreur lors du chargement du classement."));
        }
        return;
    }

    if (!ladder.contains("top") || !ladder["top"].isArray()) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            joueurStatusLabel->setText(tr("Aucun joueur trouvé."));
        }
        return;
    }

    QJsonArray topArray = ladder["top"].toArray();

    // Sort by rank
    QList<QJsonObject> players;
    for (const QJsonValue& v : topArray) {
        if (v.isObject()) players.append(v.toObject());
    }
    std::sort(players.begin(), players.end(), [](const QJsonObject& a, const QJsonObject& b) {
        auto getRank = [](const QJsonObject& obj) -> int {
            QString ranksStr = obj.value("ranks").toString().remove("[").remove("]");
            QStringList vals = ranksStr.split(",", Qt::SkipEmptyParts);
            return vals.isEmpty() ? 9999 : vals.last().trimmed().toInt();
        };
        return getRank(a) < getRank(b);
    });

    // Populate combo
    for (const QJsonObject& player : players) {
        QString ranksStr = player.value("ranks").toString().remove("[").remove("]");
        QStringList ranksVals = ranksStr.split(",", Qt::SkipEmptyParts);
        int rank = ranksVals.isEmpty() ? 0 : ranksVals.last().trimmed().toInt();

        QString name = player.value("name").toString();
        // FIX: essayer d'abord "id", sinon fallback sur "name"
        QString id = player.value("id").toVariant().toString();
        if (id.isEmpty() || id == "0") {
            id = name; // fallback si pas d'ID dans le JSON
        }

        QString displayText = QString("#%1 - %2").arg(rank).arg(name);

        // Store both id and the region/edition info
        QVariantMap data;
        data["id"] = id;
        data["name"] = name;
        data["region"] = region;
        data["edition"] = edition;

        joueurTop100PlayersCombo->addItem(displayText, QVariant::fromValue(data));
    }

    joueurTop100PlayersCombo->setEnabled(joueurTop100PlayersCombo->count() > 0);
    if (joueurAddFromTop100Btn) {
        joueurAddFromTop100Btn->setEnabled(joueurTop100PlayersCombo->count() > 0);
    }

    if (joueurStatusLabel) {
        joueurStatusLabel->setStyleSheet("color: green;");
        joueurStatusLabel->setText(tr("%1 joueurs chargés.").arg(players.size()));
    }
    });
}

void MainWindow::onJoueurAddFromTop100()
{
    if (!joueurTop100PlayersCombo) return;

    const int idx = joueurTop100PlayersCombo->currentIndex();
    if (idx < 0) return;

    QVariant dataVar = joueurTop100PlayersCombo->currentData();
    if (!dataVar.isValid() || !dataVar.canConvert<QVariantMap>()) return;

    QVariantMap dataMap = dataVar.toMap();
    if (!dataMap.contains("id") || !dataMap.contains("name") ||
        !dataMap.contains("region") || !dataMap.contains("edition")) {
        return;
    }

    QString playerId = dataMap["id"].toString();
    const QString playerName = dataMap["name"].toString();
    const QString region = dataMap["region"].toString();
    const int edition = dataMap["edition"].toInt();

    if (playerId.isEmpty()) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            joueurStatusLabel->setText(tr("Joueur invalide."));
        }
        return;
    }

    // Vérifier si on n'a pas déjà trop d'entrées
    if (joueurEntries.size() >= 10) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: orange;");
            joueurStatusLabel->setText(tr("Maximum 10 joueurs/séries."));
        }
        return;
    }

    // Ajouter directement le joueur avec l'édition sélectionnée
    JoueurEntry entry;
    entry.playerId = playerId;
    entry.displayName = playerName + QString(" [%1]").arg(region);
    entry.editionStart = edition;
    entry.editionEnd = edition;
    entry.region = (region == "Jap" || region == "JP") ? "Jap" : "Glo"; // NEW: store region
    ++comparisonGeneration;
    joueurEntries.append(entry);

    refreshJoueurPlayersList();

    if (joueurStatusLabel) {
        joueurStatusLabel->setStyleSheet("color: green;");
        joueurStatusLabel->setText(tr("%1 ajouté.").arg(playerName));
    }
}


void MainWindow::refreshJoueurPlayersList()
{
    if (!joueurPlayersList) return;
    joueurPlayersList->clear();

    for (const auto& entry : joueurEntries) {
        QString text;
        if (entry.editionStart == entry.editionEnd) {
            text = QString("%1 [%2]").arg(entry.displayName).arg(entry.editionStart);
        } else {
            text = QString("%1 [%2-%3]").arg(entry.displayName).arg(entry.editionStart).arg(entry.editionEnd);
        }
        auto item = new QListWidgetItem(text, joueurPlayersList);
        item->setData(Qt::UserRole, entry.playerId);
    }

    if (joueurRemovePlayerBtn) {
        joueurRemovePlayerBtn->setEnabled(joueurPlayersList->count() > 0);
    }
}

void MainWindow::onJoueurUseCurrentId()
{
    if (joueurIdEdit && !AppSettings::savedIdentifier.isEmpty()) {
        joueurIdEdit->setText(AppSettings::savedIdentifier);
    }
}

void MainWindow::onJoueurAddPlayer()
{
    if (!joueurIdEdit || !joueurEditionStartCombo || !joueurEditionEndCombo) return;

    const QString playerId = joueurIdEdit->text().trimmed();
    if (playerId.isEmpty()) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            joueurStatusLabel->setText(tr("Veuillez entrer un identifiant."));
        }
        return;
    }

    const int edStart = joueurEditionStartCombo->currentData().toInt();
    const int edEnd = joueurEditionEndCombo->currentData().toInt();

    // Valider les éditions (start <= end en valeur, mais les combos sont décroissants)
    if ((edStart==0)!=(edEnd==0)) {
        if(joueurStatusLabel) joueurStatusLabel->setText(tr("Select Current on both sides, or two archive editions."));
        return;
    }
    const int edMin = qMin(edStart, edEnd);
    const int edMax = qMax(edStart, edEnd);

    // Vérifier si on n'a pas déjà trop d'entrées
    if (joueurEntries.size() >= 10) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: orange;");
            joueurStatusLabel->setText(tr("Maximum 10 joueurs/séries."));
        }
        return;
    }

    // Récupérer le nom du joueur depuis la première édition
    // IMPORTANT: si edMax == édition la plus récente, utiliser 0 pour l'API (tournoi en cours)
    const int edForApi = edMax;

    const auto region=AppSettings::region;
    const auto generation=++addPlayerGeneration;
    QUrlQuery query;query.addQueryItem("identifier",playerId);
    if(joueurAddPlayerBtn) joueurAddPlayerBtn->setEnabled(false);
    WtApi::instance().get(WtApi::endpoint(edForApi,"get-user",region,query),this,
        [this,region,playerId,edMin,edMax,generation](const QByteArray &bytes,const QString &error) {
    if(joueurAddPlayerBtn) joueurAddPlayerBtn->setEnabled(!joueurIdEdit->text().trimmed().isEmpty());
    if(generation!=addPlayerGeneration || region!=AppSettings::region || joueurEntries.size()>=10) return;
    auto data=WtData::normalize(QJsonDocument::fromJson(bytes),"users");
    if(!error.isEmpty()) data["error"]=error;
    // Vérifier si l'ID existe
    if (data.isEmpty() || data.contains("error")) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            QString errorMsg = data.contains("error")
                ? data["error"].toString()
                : tr("Identifiant introuvable.");
            joueurStatusLabel->setText(tr("Erreur : %1").arg(errorMsg));
        }
        return;
    }

    QString displayName = playerId;

    // Gérer le cas de plusieurs utilisateurs avec le même nom
    if (data.contains("users") && data["users"].isArray()) {
        QJsonArray jsonArray = data["users"].toArray();
        if (jsonArray.isEmpty()) {
            if (joueurStatusLabel) {
                joueurStatusLabel->setStyleSheet("color: red;");
                joueurStatusLabel->setText(tr("Aucun joueur trouvé avec cet identifiant."));
            }
            return;
        }

        // Helper : récupérer le dernier point (numérique) d'un objet
        auto lastPointsValue = [](const QJsonObject &obj)->double {
            QString ptsStr = obj.value("points").toString().remove("[").remove("]");
            QStringList ptsVals = ptsStr.split(',', Qt::SkipEmptyParts);
            if (ptsVals.isEmpty()) return 0.0;
            return ptsVals.last().trimmed().toDouble();
        };

        // Helper : formater le dernier point avec séparateur d'espaces tous les 3 chiffres
        auto formatThousands = [](const QString &s)->QString {
            QString str = s.trimmed();
            // Séparer partie entière et décimale (si présente)
            QString intPart = str;
            QString fracPart;
            int dot = str.indexOf('.');
            if (dot >= 0) {
                intPart = str.left(dot);
                fracPart = str.mid(dot);
            }
            // Gérer signe
            bool neg = false;
            if (intPart.startsWith('-')) { neg = true; intPart.remove(0, 1); }

            // Retirer espaces éventuels
            intPart.remove(' ');

            QString out;
            int cnt = 0;
            for (int i = intPart.length() - 1; i >= 0; --i) {
                out.prepend(intPart[i]);
                ++cnt;
                // Ajouter une virgule après chaque 3 chiffres sauf si c'est le dernier groupe
                if (cnt == 3 && i != 0) {
                    out.prepend(' ');
                    cnt = 0;
                }
            }
            if (neg) out.prepend('-');
            out += fracPart;
            return out;
        };

        // Convertir en liste d'objets et trier par points décroissant
        QList<QJsonObject> objs;
        objs.reserve(jsonArray.size());
        for (const QJsonValue &v : jsonArray) {
            if (v.isObject()) objs.append(v.toObject());
        }
        std::sort(objs.begin(), objs.end(), [&](const QJsonObject &a, const QJsonObject &b){
            return lastPointsValue(a) > lastPointsValue(b);
        });

        if (objs.size() == 1) {
            data = objs.first();
        } else {
            // Plusieurs utilisateurs : afficher une boîte de dialogue triée pour choisir
            QDialog dialog(this);
            dialog.setWindowTitle(tr("Choisir un joueur"));
            QVBoxLayout layout(&dialog);

            QLabel* infoLabel = new QLabel(tr("Plusieurs joueurs trouvés (triés par points). Sélectionnez-en un :"), &dialog);
            layout.addWidget(infoLabel);

            QListWidget listWidget(&dialog);
            for (const QJsonObject &obj : objs) {
                QString ranksStr = obj.value("ranks").toString().remove("[").remove("]");
                QStringList ranksVals = ranksStr.split(",", Qt::SkipEmptyParts);
                QString lastRank = ranksVals.isEmpty() ? QStringLiteral("?") : ranksVals.last().trimmed();

                QString ptsStr = obj.value("points").toString().remove("[").remove("]");
                QStringList ptsVals = ptsStr.split(",", Qt::SkipEmptyParts);
                QString lastPtsRaw = ptsVals.isEmpty() ? QStringLiteral("0") : ptsVals.last().trimmed();

                QString lastPtsFmt = formatThousands(lastPtsRaw);

                QString itemText = QString("%1 - %2 %3 - %4 %5")
                    .arg(obj.value("name").toString())
                    .arg(tr("Rank #"))
                    .arg(lastRank)
                    .arg(lastPtsFmt)
                    .arg(tr("pts"));

                QListWidgetItem* item = new QListWidgetItem(itemText, &listWidget);
                item->setData(Qt::UserRole, obj);
            }
            layout.addWidget(&listWidget);

            QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
            connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            layout.addWidget(&buttonBox);

            // Double-clic pour sélectionner directement
            connect(&listWidget, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);

            if (listWidget.count() > 0) listWidget.setCurrentRow(0);

            if (dialog.exec() != QDialog::Accepted || !listWidget.currentItem()) {
                return; // Annulé
            }

            data = listWidget.currentItem()->data(Qt::UserRole).toJsonObject();
        }
    }

    if (data.contains("name")) {
        displayName = data["name"].toString();
    }

    JoueurEntry entry;
    entry.playerId = data.value("id").toVariant().toString();
    if(entry.playerId.isEmpty()) entry.playerId=playerId;
    entry.region=region;
    entry.displayName = displayName;
    entry.editionStart = edMin;
    entry.editionEnd = edMax;
    ++comparisonGeneration;
    joueurEntries.append(entry);

    refreshJoueurPlayersList();
    joueurIdEdit->clear();

    if (joueurStatusLabel) {
        joueurStatusLabel->setStyleSheet("color: green;");
        joueurStatusLabel->setText(tr("%1 ajouté.").arg(displayName));
    }
    });
}

void MainWindow::onJoueurRemovePlayer()
{
    if (!joueurPlayersList) return;

    const int row = joueurPlayersList->currentRow();
    if (row >= 0 && row < joueurEntries.size()) {
        ++comparisonGeneration;
        joueurEntries.remove(row);
        refreshJoueurPlayersList();
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("");
            joueurStatusLabel->setText(tr("Joueur supprimé."));
        }
    }
}

void MainWindow::onJoueurClearAll()
{
    ++comparisonGeneration;
    joueurEntries.clear();
    refreshJoueurPlayersList();

    if (joueurGraphView && joueurGraphView->scene()) {
        joueurGraphView->scene()->clear();
    }

    if (joueurStatusLabel) {
        joueurStatusLabel->setStyleSheet("");
        joueurStatusLabel->setText(tr("Liste effacée."));
    }
}

void MainWindow::onJoueurPlayerSelectionChanged()
{
    if (joueurRemovePlayerBtn && joueurPlayersList) {
        joueurRemovePlayerBtn->setEnabled(joueurPlayersList->currentRow() >= 0);
    }
}

void MainWindow::onJoueurGenerateClicked()
{
    if (!joueurGraphView || !joueurYDataCombo) return;

    if (joueurEntries.isEmpty()) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            joueurStatusLabel->setText(tr("Ajoutez au moins un joueur."));
        }
        return;
    }

    const auto entries=joueurEntries;
    const auto region=AppSettings::region;
    const auto generation=++comparisonGeneration;
    QList<QUrl> urls;
    for(const auto &entry:entries) {
        QUrlQuery query;query.addQueryItem("identifier",entry.playerId);
        for(int ed=entry.editionStart;ed<=entry.editionEnd;++ed) urls.append(WtApi::endpoint(ed,"get-user",entry.region.isEmpty()?region:entry.region,query));
    }
    if(urls.size()>40) {if(joueurStatusLabel) joueurStatusLabel->setText(tr("Select at most 40 player/edition series."));return;}
    if(joueurGenerateBtn) joueurGenerateBtn->setEnabled(false);
    WtApi::instance().getMany(urls,this,[this,entries,region,generation](const QHash<QUrl,WtApi::Result> &results) {
    if(joueurGenerateBtn) joueurGenerateBtn->setEnabled(true);
    if(generation!=comparisonGeneration || region!=AppSettings::region) return;
    const QString ydata = joueurYDataCombo->currentText();
    const bool isWinsPace = (ydata == QStringLiteral("wins_pace"));

    // NEW: Extended color palette for multiple series (works with all themes)
    // These colors are chosen to be distinguishable and visually pleasing
    static const QVector<QColor> extendedColors = {
        QColor("#E6194B"), // Red
        QColor("#3CB44B"), // Green
        QColor("#FFE119"), // Yellow
        QColor("#4363D8"), // Blue
        QColor("#F58231"), // Orange
        QColor("#911EB4"), // Purple
        QColor("#42D4F4"), // Cyan
        QColor("#F032E6"), // Magenta
        QColor("#BFEF45"), // Lime
        QColor("#FABED4"), // Pink
        QColor("#469990"), // Teal
        QColor("#DCBEFF"), // Lavender
        QColor("#9A6324"), // Brown
        QColor("#FFFAC8"), // Beige
        QColor("#800000"), // Maroon
        QColor("#AAFFC3"), // Mint
        QColor("#808000"), // Olive
        QColor("#FFD8B1"), // Apricot
        QColor("#000075"), // Navy
        QColor("#A9A9A9"), // Grey
    };
    int colorIndex = 0;

    // Créer le graphique
    QChart* chart = new QChart();
    chart->setTitle(tr("Comparaison de %1 joueur(s)").arg(entries.size()));
    chart->setTheme(AppSettings::chartThemeEnum());
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setMargins(QMargins(12, 8, 8, 14));
    chart->setBackgroundPen(Qt::NoPen);

    double globalMinX = std::numeric_limits<double>::max();
    double globalMaxX = std::numeric_limits<double>::lowest();
    double globalMinY = std::numeric_limits<double>::max();
    double globalMaxY = std::numeric_limits<double>::lowest();
    int validSeries = 0;
    QList<double> allYValues; // Pour ajuster l'axe Y



    for (const auto& entry : entries) {
        for (int ed = entry.editionStart; ed <= entry.editionEnd; ++ed) {
            const int edForApi = ed;

            QUrlQuery query;query.addQueryItem("identifier",entry.playerId);
            const auto response=results.value(WtApi::endpoint(edForApi,"get-user",entry.region.isEmpty()?region:entry.region,query));
            if(!response.error.isEmpty()) continue;
            QJsonObject data=WtData::normalize(QJsonDocument::fromJson(response.bytes),"users");
            if(AppSettings::hideNegativeTimes) WtData::filterNegativeHours(data);
            if (data.isEmpty() || data.contains("error")) continue;

            if (data.contains("users") && data["users"].isArray()) {
                QJsonArray users = data["users"].toArray();
                if (users.isEmpty()) continue;
                data = users.first().toObject();
            }

            if (!data.contains("hour") || !data.contains(ydata)) continue;

            QString hoursStr = data["hour"].toString().remove("[").remove("]");
            QString valuesStr = data[ydata].toString().remove("[").remove("]");

            QStringList hourList = hoursStr.split(",", Qt::SkipEmptyParts);
            QStringList valueList = valuesStr.split(",", Qt::SkipEmptyParts);

            if (hourList.size() != valueList.size() || hourList.isEmpty()) continue;

            QLineSeries* series = new QLineSeries();
            QString seriesName;
            if (entry.editionStart == entry.editionEnd) {
                seriesName = QString("%1 [%2]").arg(entry.displayName).arg(ed);
            } else {
                seriesName = QString("%1 #%2").arg(entry.displayName).arg(ed);
            }
            series->setName(seriesName);

            // NEW: Apply custom color from extended palette
            QPen pen = series->pen();
            pen.setColor(extendedColors[colorIndex % extendedColors.size()]);
            pen.setWidth(2);
            series->setPen(pen);
            colorIndex++;

            for (int i = 0; i < hourList.size(); ++i) {
                bool okX, okY;
                double x = hourList[i].trimmed().toDouble(&okX);
                double y = valueList[i].trimmed().toDouble(&okY);
                if (okX && okY) {
                    series->append(x, y);
                    globalMinX = std::min(globalMinX, x);
                    globalMaxX = std::max(globalMaxX, x);
                    globalMinY = std::min(globalMinY, y);
                    globalMaxY = std::max(globalMaxY, y);
                    allYValues.append(y);
                }
            }

            if (series->count() > 0) {
                chart->addSeries(series);
                validSeries++;
            } else {
                delete series;
                colorIndex--; // NEW: Don't waste color if series was deleted
            }
        }


    }



    if (validSeries == 0) {
        if (joueurStatusLabel) {
            joueurStatusLabel->setStyleSheet("color: red;");
            joueurStatusLabel->setText(tr("Aucune donnée trouvée."));
        }
        delete chart;
        return;
    }

    // Créer l'axe X avec des catégories tous les 6h ou 12h (comme dans render.cpp)
    QCategoryAxis* axisX = new QCategoryAxis();
    axisX->setTitleText(tr("Heures"));
    axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    const double xRange = globalMaxX - globalMinX;
    const int step = (xRange > 48.0) ? 12 : 6;
    const int startH = static_cast<int>(std::floor(globalMinX / step)) * step;
    const int endH = static_cast<int>(std::ceil(globalMaxX / step)) * step;
    for (int h = startH; h <= endH; h += step) {
        if (h >= 0) {
            axisX->append(QString::number(h) + "h", static_cast<double>(h));
        }
    }
    axisX->setRange(globalMinX, globalMaxX);
    chart->addAxis(axisX, Qt::AlignBottom);

    // Créer l'axe Y avec des ticks pairs pour wins_pace, sinon numérique
    if (isWinsPace && !allYValues.isEmpty()) {
        // Axe avec ticks pairs (0, 2, 4, ...) comme dans render.cpp
        double maxY = *std::max_element(allYValues.begin(), allYValues.end());
        if (maxY < 0.0) maxY = 0.0;
        int topEven = static_cast<int>(std::ceil(maxY));
        if (topEven < 2) topEven = 2;
        if (topEven % 2 != 0) topEven++;

        QCategoryAxis* axisY = new QCategoryAxis();
        axisY->setTitleText(ydata);
        axisY->setRange(0.0, static_cast<double>(topEven));
        axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
        for (int v = 0; v <= topEven; v += 2) {
            axisY->append(QString::number(v), static_cast<double>(v));
        }
        chart->addAxis(axisY, Qt::AlignLeft);
    } else {
        // Axe numérique classique avec marge
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText(ydata);
        double margin = (globalMaxY - globalMinY) * 0.06;
        if (margin < 0.1) margin = 0.1;
        double minY = (globalMinY < 0) ? globalMinY : 0.0;
        axisY->setRange(minY, globalMaxY + margin);
        axisY->setTickCount(7);
        axisY->setLabelFormat("%.0f");
        chart->addAxis(axisY, Qt::AlignLeft);
    }

    // Attacher toutes les séries aux axes
    for (auto s : chart->series()) {
        s->attachAxis(chart->axes(Qt::Horizontal).first());
        s->attachAxis(chart->axes(Qt::Vertical).first());
    }

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // Afficher dans la vue (mêmes dimensions que classement.ui)
    if (!joueurGraphView->scene()) {
        joueurGraphView->setScene(new QGraphicsScene(joueurGraphView));
    }
    QGraphicsScene* scene = joueurGraphView->scene();
    scene->clear();
    scene->setBackgroundBrush(Qt::NoBrush);

    joueurGraphView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    joueurGraphView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    joueurGraphView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    joueurGraphView->setFrameShape(QFrame::NoFrame);
    joueurGraphView->setBackgroundBrush(Qt::NoBrush);
    joueurGraphView->setStyleSheet("border: 0;");
    if (joueurGraphView->viewport()) {
        joueurGraphView->viewport()->setAutoFillBackground(false);
        joueurGraphView->viewport()->setStyleSheet("background: transparent;");
    }

    QRectF vpRect(0, 0, joueurGraphView->viewport()->width(), joueurGraphView->viewport()->height());
    scene->setSceneRect(vpRect);

    auto chartView = new QChartView(chart);
    chartView->setFrameShape(QFrame::NoFrame);
    chartView->setLineWidth(0);
    chartView->setContentsMargins(0, 0, 0, 0);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("border: 0;");
    chartView->setAutoFillBackground(false);
    chartView->setBackgroundBrush(Qt::NoBrush);
    chartView->setAttribute(Qt::WA_TranslucentBackground);
    if (chartView->viewport()) {
        chartView->viewport()->setAutoFillBackground(false);
        chartView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    }

    QGraphicsProxyWidget* proxy = scene->addWidget(chartView);
    proxy->setPos(0, 0);
    proxy->setGeometry(vpRect);

    joueurGraphView->setRenderHint(QPainter::Antialiasing);

    if (joueurStatusLabel) {
        joueurStatusLabel->setStyleSheet("color: green;");
        joueurStatusLabel->setText(tr("Graphique généré avec %1 série(s).").arg(validSeries));
    }
    });
}


void MainWindow::onJoueurCopyClicked()
{
    if (!joueurGraphView || !joueurGraphView->scene()) {
        if (joueurCopyConfirmLabel) {
            joueurCopyConfirmLabel->setStyleSheet("color: red;");
            joueurCopyConfirmLabel->setText(tr("Aucun graphique à copier."));
        }
        return;
    }

    QImage img = Render::grabChartOnly(joueurGraphView);
    if (img.isNull()) {
        img = Render::grabChartImage(joueurGraphView);
    }
    if (img.isNull()) {
        if (joueurCopyConfirmLabel) {
            joueurCopyConfirmLabel->setStyleSheet("color: red;");
            joueurCopyConfirmLabel->setText(tr("Erreur lors de la copie."));
        }
        return;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setImage(img);

    if (joueurCopyConfirmLabel) {
        joueurCopyConfirmLabel->setStyleSheet("color: green;");
        joueurCopyConfirmLabel->setText(tr("Graphique copié !"));
        QTimer::singleShot(3000, this, [this]() {
            if (joueurCopyConfirmLabel) joueurCopyConfirmLabel->clear();
        });
    }
}

#include <QDialog>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>

void MainWindow::showRankAnalysisDialog(const QString &link) {
    if (m_rankHistoryPts.isEmpty()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Analyse du rang %1").arg(m_rankTarget));
    dlg.resize(420, 380);

    dlg.setStyleSheet("QDialog { background-color: #252526; color: #cccccc; } "
                      "QLabel { color: #cccccc; border: none; } "
                      "QTableWidget { background-color: #1e1e1e; color: #cccccc; gridline-color: #3c3c3c; border: 1px solid #3c3c3c; border-radius: 4px; } "
                      "QHeaderView::section { background-color: #2d2d2d; color: #9d9d9d; border: 1px solid #3c3c3c; border-top: none; border-left: none; font-weight: bold; padding: 4px; } "
                      "QScrollBar:vertical { border: none; background: #252526; width: 10px; margin: 0px 0px 0px 0px; } "
                      "QScrollBar::handle:vertical { background: #4a4a4a; min-height: 20px; border-radius: 5px; } "
                      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; height: 0px; } "
                      "QPushButton { background-color: #0e639c; color: white; border-radius: 3px; padding: 6px 15px; font-weight: bold; } "
                      "QPushButton:hover { background-color: #1177bb; } "
                      "QPushButton:pressed { background-color: #094771; }");

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *info = new QLabel(tr("Cette estimation utilise une régression linéaire sur les %1 derniers tournois.").arg(m_rankHistoryPts.size()), &dlg);
    info->setWordWrap(true);
    layout->addWidget(info);

    QTableWidget *table = new QTableWidget(m_rankHistoryPts.size() + 1, 2, &dlg);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setHorizontalHeaderLabels({tr("Édition"), tr("Points Requis")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    
    for (int i = 0; i < m_rankHistoryPts.size(); ++i) {
        QTableWidgetItem *edItem = new QTableWidgetItem(QString::number(m_rankHistoryEds[i]));
        QTableWidgetItem *ptItem = new QTableWidgetItem(formatMillionsCompact(m_rankHistoryPts[i]));
        edItem->setTextAlignment(Qt::AlignCenter);
        ptItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(i, 0, edItem);
        table->setItem(i, 1, ptItem);
    }
    
    QTableWidgetItem *edProjItem = new QTableWidgetItem(tr("%1").arg(m_rankProjectedEd));
    QTableWidgetItem *ptProjItem = new QTableWidgetItem(formatMillionsCompact(m_rankProjectedPts));
    edProjItem->setTextAlignment(Qt::AlignCenter);
    edProjItem->setForeground(QBrush(QColor("#3794ff")));
    ptProjItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ptProjItem->setForeground(QBrush(QColor("#3794ff")));
    QFont font = edProjItem->font(); font.setBold(true);
    edProjItem->setFont(font); ptProjItem->setFont(font);

    table->setItem(m_rankHistoryPts.size(), 0, edProjItem);
    table->setItem(m_rankHistoryPts.size(), 1, ptProjItem);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(table);

    QPushButton *btnClose = new QPushButton(tr("Fermer"), &dlg);
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(btnClose);

    dlg.exec();
}


double MainWindow::remainingTournamentHours() const {
    return tbEndEpoch>0?std::max(0.0,(tbEndEpoch-QDateTime::currentSecsSinceEpoch())/3600.0):0;
}
