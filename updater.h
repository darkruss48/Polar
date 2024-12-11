#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr);
    void checkForUpdate();
    static std::string polar_version;

signals:
    void updateAvailable(const QString &latestVersion, const QString &changelog, const QString &downloadUrl);

private:
    QString currentVersion;
};

#endif // UPDATER_H
