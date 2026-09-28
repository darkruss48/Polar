#pragma once
#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
class QComboBox;
class QDoubleSpinBox;
class QTableWidget;
class QLabel;
class RaceAnalysisDialog : public QDialog {
public:
    RaceAnalysisDialog(const QJsonArray &players, const QJsonObject &metadata, QWidget *parent=nullptr);
private:
    QJsonArray players;
    QJsonObject metadata;
    QComboBox *windowBox;
    QComboBox *focusBox;
    QDoubleSpinBox *pauseBox;
    QTableWidget *table;
    QLabel *status;
    void refresh();
};
