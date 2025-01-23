#ifndef FUNCTB_H
#define FUNCTB_H
#include <QNetworkRequest>
#include "mainwindow.h"

class functb
{
public:
    functb();
    static std::string ver_code;
    static std::string identifier; // identifiant qui permet d'avoir
    static QJsonObject pologet();
    static QJsonObject pologettop();
    static std::string secret;
    static std::string access_token;
    static void connect(Ui::MainWindow *ui);
    static void getHeader(QNetworkRequest &request);
    static QString mac(const QString& url, int port, const QString& method, const QString& action, QString secret, QString access_token);
};

#endif // FUNCTB_H
