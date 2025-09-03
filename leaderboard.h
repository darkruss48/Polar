#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QSet>              // NEW
#include "mainwindow.h"

class Leaderboard : public QWidget
{
    Q_OBJECT
public:
    static void onRefreshClicked(MainWindow * this_, QListWidget *playerList);
    static void affichergraphiqueettexte(MainWindow * this_, QJsonObject user);
    static QGraphicsView *graphPlaceholder;
    static QLabel *dataPlaceholder;
    static QLabel *avgPlaceholder;
    // NEW: overlay state
    static QString baseSeriesName;
    static QSet<QString> overlayNames;

    explicit Leaderboard(QWidget *parent = nullptr);
    ~Leaderboard();

protected:
    void changeEvent(QEvent *event) override;
    void retranslateUi();

private slots:
    void onPlayerDoubleClicked(QListWidgetItem *item);

private:
    QVBoxLayout *mainLayout;
    QHBoxLayout *contentLayout;
    QListWidget *playerList;
    QPushButton *refreshButton;
    // QGraphicsView *graphPlaceholder;
    // QLabel *dataPlaceholder;
};

#endif // LEADERBOARD_H
