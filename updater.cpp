#include "updater.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopServices>
#include <iostream>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>

Updater::Updater(QObject *parent) : QObject(parent) {}

std::string Updater::polar_version = "v1.4.4";

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
                m_latestAssetUrl.clear();
                m_latestAssetName.clear();
                if (jsonObj.contains("assets") && jsonObj["assets"].isArray()) {
                    const QJsonArray assets = jsonObj["assets"].toArray();
                    for (const QJsonValue& v : assets) {
                        const QJsonObject a = v.toObject();
                        const QString name = a.value("name").toString();
                        const QString url  = a.value("browser_download_url").toString();
                        const QString ctype = a.value("content_type").toString();
                        if (name.endsWith(".exe", Qt::CaseInsensitive) ||
                            ctype.contains("msdownload", Qt::CaseInsensitive)) {
                            m_latestAssetUrl = url;
                            m_latestAssetName = name;
                            break;
                        }
                    }
                }

                if (title.toStdString() != polar_version) {
                    emit updateAvailable(title, changelog, downloadUrl);
                }
            }
        }
        reply->deleteLater();
    });
}

void Updater::startDownloadLatestAsset()
{
    // Fallback: open release page in browser if no asset URL
    if (m_latestAssetUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl("https://github.com/darkruss48/polar/releases/latest"));
        return;
    }

    // Reset timing
    m_dlTimer.invalidate();
    m_lastBytes = 0;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QUrl assetUrl(m_latestAssetUrl);
    QNetworkRequest req(assetUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("PolarApp"));

    QNetworkReply* rep = manager->get(req);

    // Wire progress: compute global average speed and ETA
    connect(rep, &QNetworkReply::downloadProgress, this, [this](qint64 rec, qint64 tot){
        if (!m_dlTimer.isValid()) {
            m_dlTimer.start();
            emit downloadStarted(tot);
        }
        const qint64 ms = qMax<qint64>(1, m_dlTimer.elapsed());
        const double secs = static_cast<double>(ms) / 1000.0;
        const double speed = (secs > 0.0) ? static_cast<double>(rec) / secs : 0.0; // B/s
        const qint64 eta = (speed > 0.0 && tot > rec) ? static_cast<qint64>((tot - rec) / speed) : 0;
        emit downloadProgress(rec, tot, speed, eta);
        m_lastBytes = rec;
    });

    connect(rep, &QNetworkReply::finished, this, [this, rep]() {
        if (rep->error() != QNetworkReply::NoError) {
            const QString err = rep->errorString();
            emit downloadFinished(QString(), false, err);
            // Fallback to opening the asset URL in browser
            QDesktopServices::openUrl(rep->url());
            rep->deleteLater();
            return;
        }
        const QByteArray bin = rep->readAll();
        rep->deleteLater();

        // Save beside current application
        const QString appDir = QCoreApplication::applicationDirPath();
        QString fileName = m_latestAssetName.isEmpty()
            ? QStringLiteral("Polar_v%1_x64.exe").arg(polar_version.c_str())
            : m_latestAssetName;
        // Ensure unique filename if it already exists
        QString fullPath = QDir(appDir).filePath(fileName);
        if (QFile::exists(fullPath)) {
            QFileInfo fi(fullPath);
            const QString base = fi.completeBaseName();
            const QString ext  = fi.suffix().isEmpty() ? QStringLiteral("exe") : fi.suffix();
            int idx = 1;
            do {
                fileName = QString("%1 (%2).%3").arg(base).arg(idx).arg(ext);
                fullPath = QDir(appDir).filePath(fileName);
                ++idx;
            } while (QFile::exists(fullPath) && idx < 100);
        }

        QFile f(fullPath);
        if (!f.open(QIODevice::WriteOnly)) {
            const QString err = tr("Impossible d'écrire le fichier dans le dossier de l'application.");
            emit downloadFinished(QString(), false, err);
            QDesktopServices::openUrl(QUrl(m_latestAssetUrl));
            return;
        }
        f.write(bin);
        f.close();

        emit downloadFinished(fullPath, true, QString());

        // Launch the downloaded installer/app, then quit this instance
        QProcess::startDetached(fullPath, QStringList(), QFileInfo(fullPath).absolutePath());
        QCoreApplication::quit();
    });
}
