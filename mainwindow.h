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

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QMenu* menu1;
    QAction* menu1_action1;
    QAction* menu1_action2;
    void changeEvent(QEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

protected slots:
    void slotLanguageChanged(QAction* action);

private slots:

    // void fetchData(int pageNumber, QJsonDocument previousResponse);

    void on_pushButton_clicked();

    void formatNumberWithCommas(const QString &text, QString &outFormattedNumber);

    // void on_pushButton_3_clicked();

    // void on_pushButton_2_clicked();

    void on_bouton_graphique_clicked();

    void on_idButton_clicked();

    void on_lineEdit_goal_textEdited(const QString &arg1);

    void on_lineEdit_afk_textEdited(const QString &arg1);

    // Options dialog
    void showOptionsDialog(); // +

    void resizeEvent(QResizeEvent *event) override;

    // NEW: copy current Classement chart to clipboard
    void copyClassementGraphToClipboard(); // NEW
    // NEW: copy current page graph (Graphs or Classement)
    void copyAnyGraphToClipboard(); // NEW
    // NEW: checkbox toggled
    void onGoalOverlayToggled(bool checked); // NEW

    void on_checkBox_clicked();

    void on_checkBox_clicked(bool checked);

private:
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
    static QColor goalLineColorForTheme(QChart::ChartTheme t); // NEW
};
#endif // MAINWINDOW_H
