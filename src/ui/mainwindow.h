#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonDocument>
#include <QTranslator>
#include "updater.h"
// Def var "globales"

#include <QStackedWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "ui_mainwindow.h"
#include <QtUiTools/QUiLoader>
#include <QFile>
#include <iostream>

// Tips rotation includes
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QPauseAnimation>
#include <QStringList>
#include <QRandomGenerator>
#include <QEvent>
#include <QTimer>
#include <QtCharts/QChart>
#include <QProgressDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QProgressBar;
class EditionPickerWidget; // NEW: forward declaration
class QProgressDialog;     // NEW
class QGraphicsView;
class QListWidget;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    double remainingTournamentHours() const;

protected:
    QMenu* menu1;
    QAction* menu1_action1;
    QAction* menu1_action2;
    void changeEvent(QEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

protected slots:
    void slotLanguageChanged(QAction* action);
    // Update download progress UI (Updater)
    void onUpdateDownloadStarted(qint64 totalBytes);
    void onUpdateDownloadProgress(qint64 receivedBytes, qint64 totalBytes, double speedBytesPerSec, qint64 etaSecs);
    void onUpdateDownloadFinished(const QString& filePath, bool ok, const QString& errorString);

private slots:

    // void fetchData(int pageNumber, QJsonDocument previousResponse);



    void formatNumberWithCommas(const QString &text, QString &outFormattedNumber);

    // void on_pushButton_3_clicked();

    // void on_pushButton_2_clicked();

    void on_bouton_graphique_clicked();

    void on_idButton_clicked();

    void on_lineEdit_goal_textEdited(const QString &arg1);

    void on_lineEdit_afk_textEdited(const QString &arg1);
    void doGoalEstimation();

    // Options dialog
    void showOptionsDialog(); // +

    void resizeEvent(QResizeEvent *event) override;

    // NEW: copy current Classement chart to clipboard
    void copyClassementGraphToClipboard(); // NEW
    // NEW: copy current page graph (Graphs or Classement)
    void copyAnyGraphToClipboard(); // NEW
    // NEW: checkbox toggled
    void onGoalOverlayToggled(bool checked); // NEW

    // NEW: rank overlay checkbox
    void on_checkBox_2_clicked();
    void on_checkBox_2_clicked(bool checked);

    void on_checkBox_clicked(bool checked);

    // NEW: Joueur page slots
    void onJoueurGenerateClicked();
    void onJoueurCopyClicked();
    void onJoueurUseCurrentId();
    void onJoueurAddPlayer();
    void onJoueurRemovePlayer();
    void onJoueurClearAll();
    void onJoueurPlayerSelectionChanged();

    void on_checkBox_estimation_toggled(bool checked);
    void updateAfkSummaries();

private:
    QTimer* m_debounceTimerRank = nullptr;
    QTimer* m_debounceTimerGoal = nullptr;
    QLabel* m_lblAfkSummary1 = nullptr;
    QLabel* m_lblAfkSummary2 = nullptr;

    // loads a language by the given language shortcur (e.g. de, en)
    void loadLanguage(const QString& rLanguage);
    void createLanguageMenu(void);
    void onUpdateAvailable(const QString &latestVersion, const QString &changelog, const QString &downloadUrl);

    // Tips rotation helpers
    void setupTipsRotation();
    void restartTipsCycle();

    // Easter-egg helpers
    void setupEasterEgg();
    void showEasterEgg();
    void hideEasterEgg();

    // Créer un menu
    QStackedWidget *stackedWidget;
    QLabel *labelDynamic;


    Ui::MainWindow *ui;
    QTranslator m_translator; // contains the translations for this application
    QTranslator m_translatorQt; // contains the translations for qt
    QString m_currLang; // contains the currently loaded language
    QString m_langPath; // Path of language files. This is always fixed to /languages.
    Updater *updater;

    // Tips rotation members
    QGraphicsOpacityEffect* tipsEffect = nullptr;
    QSequentialAnimationGroup* tipsGroup = nullptr;
    QStringList tipsPhrases;
    int lastTipIndex = -1;

    // Easter-egg members
    QLabel* easterEggLabel = nullptr;
    QGraphicsOpacityEffect* easterEggOpacity = nullptr; // laissé mais non essentiel
    QPropertyAnimation* easterEggAnim = nullptr;        // laissé mais non utilisé
    QPropertyAnimation* easterEggSlideAnim = nullptr;   // NEW: animation de position
    bool easterEggDone = false;
    bool easterEggActive = false;

    // NEW: apply background from settings
    void updateBackgroundPalette();
    // NEW: refresh displayed identifier according to settings
    void updateIdLabelDisplay();
    QString maskedIdentifier(const QString& id) const;

    // Auto-refresh
    QTimer* autoRefreshTimer = nullptr;     // NEW
    void scheduleNextAutoRefresh();         // NEW
    void doAutoRefreshIfClassement();       // NEW

    // NEW: copy-confirm UI
    QLabel* confirmCopyLabel = nullptr;               // NEW
    QGraphicsOpacityEffect* confirmCopyEffect = nullptr; // NEW
    QPropertyAnimation* confirmCopyFade = nullptr;    // NEW
    QTimer* confirmCopyHoldTimer = nullptr;           // NEW

    // NEW: goal overlay state for Graphs page
    double lastWinPace = std::numeric_limits<double>::quiet_NaN(); // NEW
    bool   hasWinPace = false;                                      // NEW

    // NEW: helpers for Graphs page overlay
    void updateGoalOverlayOnGraphs(bool allowAxisAdjust); // NEW
    // NEW: overlay for Rank estimation
    void updateRankOverlayOnGraphs(bool allowAxisAdjust);

    static QColor goalLineColorForTheme(QChart::ChartTheme t); // NEW

    // TB metadata/progress UI
    void fetchAndInitTbMetadata();     // NEW: fetch start/end/edition and init UI
    void updateTbUiFromTimes();        // NEW: recompute progress/time left from cached times
    static QString formatDhMin(qint64 secs); // NEW: translatable D/H/M string
    void refreshTbLocalizedTexts();    // NEW: rebuild title/time labels using cached data (no network)
    void updateTbDatesDisplay();       // NEW: format and display start/end dates
    void buildTbEditionCombo();        // NEW: populate tbPicker (3-slot) from region and latest edition
    // NEW: update "Score édition XX : ..." lines for rank estimation
    void updateRankEstimation();
    
public slots:
    void showRankAnalysisDialog(const QString &link);

private:
    qint64 m_rankProjectedEd = 0;
    qint64 m_rankProjectedPts = -1;
    int m_rankTarget = 0;
    QVector<qint64> m_rankHistoryEds;
    QVector<qint64> m_rankHistoryPts;

    // NEW: 853.3M formatter (no space)
    static QString formatMillionsCompact(qint64 v);

    // Cached widgets (autodetected)
    QLabel* tbTitleLabel = nullptr;         // “xxème Tenkaichi Budokai”
    QProgressBar* tbProgressBar = nullptr;  // main progress bar
    EditionPickerWidget* tbPicker = nullptr; // NEW: three-slot editions picker
    QWidget* tbPickerHost = nullptr;         // NEW: host placeholder to track size
    // We keep label_time_left from ui (already used elsewhere)


    // Cached times (epoch seconds)
    quint64 metadataGeneration = 0;
    quint64 graphGeneration = 0;
    quint64 rankGeneration = 0, top100Generation = 0, addPlayerGeneration = 0, comparisonGeneration = 0;
    qint64 tbStartEpoch = 0;
    qint64 tbEndEpoch = 0;
    int    tbEdition   = 0;                 // NEW: cached edition number
    QTimer* tbTimer = nullptr;              // periodic refresh for progress/time-left

    // NEW: Rank tab computed pace (wins/h) and validity
    double lastRankWinPace = std::numeric_limits<double>::quiet_NaN();
    bool   hasRankWinPace  = false;

    // NEW: updater progress dialog
    QProgressDialog* updateDlg = nullptr;


    // NEW: Joueur page widgets
    QWidget* pageJoueur = nullptr;
    QGraphicsView* joueurGraphView = nullptr;
    QLineEdit* joueurIdEdit = nullptr;
    QListWidget* joueurPlayersList = nullptr;
    QComboBox* joueurYDataCombo = nullptr;
    QComboBox* joueurEditionStartCombo = nullptr;
    QComboBox* joueurEditionEndCombo = nullptr;
    QLabel* joueurStatusLabel = nullptr;
    QLabel* joueurCopyConfirmLabel = nullptr;
    QPushButton* joueurGenerateBtn = nullptr;
    QPushButton* joueurCopyBtn = nullptr;
    QPushButton* joueurAddPlayerBtn = nullptr;
    QPushButton* joueurRemovePlayerBtn = nullptr;
    QPushButton* joueurClearBtn = nullptr;

    // NEW: top 100 selection widgets
    QComboBox* joueurTop100RegionCombo = nullptr;
    QComboBox* joueurTop100EditionCombo = nullptr;
    QPushButton* joueurLoadTop100Btn = nullptr;
    QComboBox* joueurTop100PlayersCombo = nullptr;
    QPushButton* joueurAddFromTop100Btn = nullptr;

    // Structure pour stocker les joueurs ajoutés
    struct JoueurEntry {
        QString playerId;
        QString displayName;
        int editionStart;
        int editionEnd;
        QString region; // NEW: region for top 100 players ("Jap", "Glo", or empty for default)
    };
    QVector<JoueurEntry> joueurEntries;
    int joueurLatestEdition = 0; // NEW: numéro de l'édition la plus récente (pour savoir quand utiliser 0)

    void setupJoueurPage();
    void populateJoueurEditionCombos();
    void refreshJoueurPlayersList();
    void populateJoueurEditions();
    // NEW: top 100 helpers
    void populateJoueurTop100Editions();
    void onJoueurLoadTop100();
    void onJoueurAddFromTop100();
};

#endif // MAINWINDOW_H
