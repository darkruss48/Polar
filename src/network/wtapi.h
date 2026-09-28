#pragma once
#include <QNetworkAccessManager>
#include <QHash>
#include <QElapsedTimer>
#include <QPointer>
#include <QUrlQuery>
#include <functional>

// One transport, bounded cache, context-bound callbacks. No widgets or global region state.
class WtApi : public QObject {
public:
    using Callback = std::function<void(const QByteArray &, const QString &)>;
    struct Result { QByteArray bytes; QString error; };
    using BatchCallback = std::function<void(const QHash<QUrl,Result> &)>;
    void getMany(const QList<QUrl> &urls, QObject *context, BatchCallback callback);
    explicit WtApi(QObject *parent = nullptr);
    static WtApi &instance();
    static QUrl endpoint(int edition, const QString &resource, const QString &region,
                         const QUrlQuery &query = {});
    void get(const QUrl &url, QObject *context, Callback callback, int ttlSeconds = 60);
private:
    struct CacheEntry { QByteArray bytes; qint64 inserted; };
    struct Listener { QPointer<QObject> context; Callback callback; };
    QNetworkAccessManager manager;
    QElapsedTimer clock;
    QHash<QUrl, CacheEntry> cache;
    QHash<QUrl, QList<Listener>> pending;
};
