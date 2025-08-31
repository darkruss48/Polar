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

// Add includes for tips rotation
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QPauseAnimation>
#include <QStringList>
#include <QRandomGenerator>

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

private:
    // loads a language by the given language shortcur (e.g. de, en)
    void loadLanguage(const QString& rLanguage);
    void createLanguageMenu(void);
    void onUpdateAvailable(const QString &latestVersion, const QString &changelog, const QString &downloadUrl);

    // Tips rotation helpers
    void setupTipsRotation();
    void restartTipsCycle();

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
};
#endif // MAINWINDOW_H
