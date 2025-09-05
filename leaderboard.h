#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QSet>
#include <QVector> // NEW
#include "mainwindow.h"

class Leaderboard : public QWidget
{
    Q_OBJECT
public:
    static void onRefreshClicked(MainWindow * this_, QListWidget *playerList);
    static void affichergraphiqueettexte(MainWindow * this_, QJsonObject user, bool preserveOverlays = false);
    static void autoRefresh(MainWindow* this_); // NEW
    static QGraphicsView *graphPlaceholder;

    // NEW: keep access to the list widget
    static QListWidget* playerListPtr; // NEW

    // Old single-column placeholders (kept for safety)
    static QLabel *dataPlaceholder;
    static QLabel *avgPlaceholder;
    static QLabel *gapPlaceholder;

    // NEW: two-column placeholders
    static QLabel *dataLeftPlaceholder;
    static QLabel *dataRightPlaceholder;
    static QLabel *avgLeftPlaceholder;
    static QLabel *avgRightPlaceholder;
    static QLabel *gapLeftPlaceholder;
    static QLabel *gapRightPlaceholder;     // NEW

    // NEW: overlay state
    static QString baseSeriesName;
    static QSet<QString> overlayNames;
    static QString currentSelectedName; // NEW

    // NEW: snapshot rows
    struct SimpleRow {
        int rank;
        QString name;
        qint64 lastPoints;
        QStringList pointsList;
    };
    static QVector<SimpleRow> snapshotRows;

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
