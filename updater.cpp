#include "updater.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopServices>
#include <iostream>


Updater::Updater(QObject *parent) : QObject(parent) {}

std::string Updater::polar_version = "v1.3.0";

void Updater::checkForUpdate()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QUrl url("https://api.github.com/repos/darkruss48/polar/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "PolarApp");

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response_data);
            if (!jsonDoc.isNull()) {
                QJsonObject jsonObj = jsonDoc.object();
                QString latestVersion = jsonObj["tag_name"].toString();
                QString changelog = jsonObj["body"].toString();
                QString downloadUrl = jsonObj["html_url"].toString();
                // titre du release fetch
                QString title = jsonObj["name"].toString();
                std::cout << "titre : " << title.toStdString() << std::endl;
                std::cout << "polar_version : " << polar_version << std::endl;

                if (title.toStdString() != polar_version) {
                    emit updateAvailable(title, changelog, downloadUrl);
                }
            }
        }
        reply->deleteLater();
    });
}
