#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QElapsedTimer>
#include <QString> // NEW

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr);
    void checkForUpdate();
    static std::string polar_version;
    // NEW: état de version (rempli après checkForUpdate)
    static bool isCurrentLatest;
    static QString latestReleaseName;
    // Download the latest .exe asset, launch it, then quit current app
    void startDownloadLatestAsset();

signals:
    void updateAvailable(const QString &latestVersion, const QString &changelog, const QString &downloadUrl);
    // Progress UI
    void downloadStarted(qint64 totalBytes);
    void downloadProgress(qint64 receivedBytes, qint64 totalBytes, double speedBytesPerSec, qint64 etaSecs);
    void downloadFinished(const QString& filePath, bool ok, const QString& errorString);

private:
    QString currentVersion;
    // Remember the chosen asset from the last API response
    QString m_latestAssetUrl;
    QString m_latestAssetName;
    // Timing for speed/ETA
    QElapsedTimer m_dlTimer;
    qint64 m_lastBytes = 0;
};

#endif // UPDATER_H
