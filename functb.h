#ifndef FUNCTB_H
#define FUNCTB_H
#include <QNetworkRequest>
#include "mainwindow.h"
#include <QTextEdit> // NEW

class functb
{
public:
    functb();
    static std::string ver_code;
    static std::string identifier; // identifiant qui permet d'avoir
    static QJsonObject pologet(int edition = 0);
    static QJsonObject pologettop(int edition = 0);       // NEW: edition-aware
    static std::string secret;
    static std::string access_token;
    static void connect(Ui::MainWindow *ui);
    static void getHeader(QNetworkRequest &request);
    static QString mac(const QString& url, int port, const QString& method, const QString& action, QString secret, QString access_token);
    static std::string points;
    static std::string wins;
    static std::string seed;
    static std::string hour_missing;
    static QJsonObject pologetmetadata(int edition = 0);  // NEW: edition-aware
    // NEW: set the QTextEdit to receive error logs
    static void setLogBox(QTextEdit* box);
    // NEW: fetch by rank (no identifier), returns object with "points" string
    static QJsonObject pologetrank(int edition, int rank);
};

#endif // FUNCTB_H
