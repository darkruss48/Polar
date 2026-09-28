#include "wtapi.h"
#include <QCoreApplication>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

WtApi::WtApi(QObject *parent) : QObject(parent), manager(this) { clock.start(); }
WtApi &WtApi::instance() {
    static WtApi *api = new WtApi(QCoreApplication::instance());
    return *api;
}
QUrl WtApi::endpoint(int edition, const QString &resource, const QString &region, const QUrlQuery &query) {
    QUrl url(QStringLiteral("https://dokkan-wt.info/api/%1/%2").arg(qMax(0,edition)).arg(resource));
    auto q=query;
    if (region=="JP" || region=="Jap") q.addQueryItem("region", "JP");
    url.setQuery(q);
    return url;
}
void WtApi::get(const QUrl &url, QObject *context, Callback callback, int ttlSeconds) {
    if (!context) return;
    const auto it=cache.constFind(url);
    if (ttlSeconds>0 && it!=cache.cend() && clock.elapsed()-it->inserted < ttlSeconds*1000LL) {
        const auto bytes=it->bytes;
        QTimer::singleShot(0, context, [callback,bytes]{callback(bytes,{});});
        return;
    }
    const bool inFlight=pending.contains(url);
    pending[url].append({context,std::move(callback)});
    if (inFlight) return;
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    request.setHeader(QNetworkRequest::UserAgentHeader,"Polar/1.5");
    auto *reply=manager.get(request);
    auto *deadline=new QTimer(reply);
    deadline->setSingleShot(true);
    connect(deadline,&QTimer::timeout,reply,&QNetworkReply::abort);
    deadline->start(20000); // Total deadline, including trickling responses.
    connect(reply,&QNetworkReply::finished,this,[this,reply,url,ttlSeconds] {
        const auto bytes=reply->readAll();
        const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString error;
        if (reply->error()!=QNetworkReply::NoError) error=reply->errorString();
        else if (status<200 || status>=300) error=QStringLiteral("HTTP %1").arg(status);
        if(error.isEmpty() && url.path().startsWith("/api/")) {
            const auto doc=QJsonDocument::fromJson(bytes);
            if(doc.isNull()) error=tr("Invalid JSON response");
            else if(doc.isObject() && doc.object().contains("error")) error=doc.object().value("error").toString(tr("API error"));
        }
        if (error.isEmpty() && ttlSeconds>0 && bytes.size()<=4*1024*1024) {
            qint64 cacheBytes=0;
            for(const auto &entry:cache) cacheBytes+=entry.bytes.size();
            if (cache.size()>=64 || cacheBytes+bytes.size()>16*1024*1024) cache.clear();
            cache.insert(url,{bytes,clock.elapsed()});
        }
        auto listeners=pending.take(url);
        reply->deleteLater();
        for (const auto &listener:listeners)
            if (listener.context) listener.callback(bytes,error);
    });
}

namespace {
class BatchRequest : public QObject {
public:
    WtApi *api;
    QList<QUrl> urls;
    WtApi::BatchCallback callback;
    QHash<QUrl,WtApi::Result> results;
    int next=0, running=0;
    BatchRequest(WtApi *api,const QList<QUrl> &urls,QObject *context,WtApi::BatchCallback callback)
        :QObject(context),api(api),urls(urls),callback(std::move(callback)) {}
    void pump() {
        if(next==urls.size() && running==0) {callback(results);deleteLater();return;}
        while(running<4 && next<urls.size()) {
            const auto url=urls[next++];++running;
            api->get(url,this,[this,url](const QByteArray &bytes,const QString &error) {
                results.insert(url,{bytes,error});--running;pump();
            });
        }
    }
};
}
void WtApi::getMany(const QList<QUrl> &urls,QObject *context,BatchCallback callback) {
    if(!context) return;
    auto *batch=new BatchRequest(this,urls,context,std::move(callback));
    QTimer::singleShot(0,batch,[batch]{batch->pump();});
}
