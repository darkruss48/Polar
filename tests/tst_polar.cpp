#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QPointer>
#include <QTimer>
#include <QTranslator>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include "wtdata.h"
#include "wtapi.h"
#include "performanceanalysis.h"
#include "editionpicker.h"
#include "raceanalysisdialog.h"
#include "render.h"
#include "appsettings.h"
#include "ui_mainwindow.h"

class PolarTests : public QObject {
    Q_OBJECT
private slots:
    void currentMetadataWithoutId() {
        const auto m=WtData::metadata(QJsonDocument::fromJson(R"({"start_at":1786689000,"end_at":1786946399})"));
        QVERIFY(!m.isEmpty());QVERIFY(!m.contains("id"));
        QVERIFY(WtData::metadata(QJsonDocument::fromJson(R"({"error":"offline"})")).isEmpty());
        QVERIFY(WtData::metadata(QJsonDocument::fromJson(R"({"start_at":10,"end_at":9})")).isEmpty());
    }
    void legacyMetadataAndRegionCatalog() {
        QCOMPARE(WtData::metadata(QJsonDocument::fromJson(R"({"metadata":{"id":"61","start_at":100,"end_at":200}})")).value("id").toInt(),61);
        QByteArray html=R"(<a href="/edition/GLB/62.db"><a href='/edition/JP/61.db'><a href="/edition/GLB/55.db"><a href="/edition/GLB/62.db">)";
        QCOMPARE(WtData::editions(html,"Glo"),QVector<int>({62,55}));
        QCOMPARE(WtData::editions(html,"JP"),QVector<int>({61}));
        QVERIFY(WtData::editions("<h1>Offline</h1>","Glo").isEmpty());
    }
    void nativeAndStringSamples() {
        const auto doc=QJsonDocument::fromJson(R"([{"hour":[-0.25,0,0.5],"points":[0,3000000000,3010000000],"name":"[a,b,c]"}])");
        auto o=WtData::normalize(doc,"top");WtData::filterNegativeHours(o);
        const auto p=o.value("top").toArray().first().toObject();
        QCOMPARE(WtData::numbers(p.value("points")),QVector<double>({3000000000.,3010000000.}));
        QCOMPARE(p.value("name").toString(),QString("[a,b,c]"));
        QVERIFY(WtData::numbers("[0,null,2]").isEmpty());
    }
    void irregularIntervals() {
        auto s=Performance::summarize({{0,100},{0.2,120},{0.7,120},{1,180}},1);
        QVERIFY(s.valid);QCOMPARE(s.activeHours,0.5);QCOMPARE(s.idleHours,0.5);
        QCOMPARE(s.recentRate,80.0);QCOMPARE(s.activeRate,160.0);QCOMPARE(s.observedRate,80.0);
        QCOMPARE(Performance::project(s,2,0.5),300.0);
    }
    void missingIntervalsAreNotAfk() {
        auto s=Performance::summarize({{0,100},{0.25,100},{2,100},{2.5,150}},2);
        QCOMPARE(s.idleHours,0.25);QCOMPARE(s.unknownHours,1.75);
        QVERIFY(std::isnan(s.recentRate));QVERIFY(std::isnan(s.observedRate));
    }
    void invalidAndResetSamples() {
        QVERIFY(!Performance::summarize({{0,0},{0,10}}).valid);
        QVERIFY(!Performance::summarize({{1,20},{0,10}}).valid);
        QVERIFY(!Performance::summarize({{0,0}}).valid);
        auto s=Performance::summarize({{0,100},{0.5,200},{1,50}});
        QVERIFY(std::isnan(s.recentRate));QVERIFY(std::isnan(s.observedRate));
        QVERIFY(Performance::samples({{"hour","[0,1]"},{"points","[1]"}}).isEmpty());
    }
    void pauseAndCatchDeadline() {
        const auto s=Performance::summarize({{0,3e9},{0.5,3.01e9},{1,3.02e9}});
        QCOMPARE(Performance::project(s,1,2),3.02e9);
        QCOMPARE(Performance::catchHours(100,80,30,3),2.0);
        QVERIFY(std::isnan(Performance::catchHours(100,80,30,1)));
        QVERIFY(std::isnan(Performance::catchHours(100,30,80,3)));
        QVERIFY(std::isnan(Performance::project(s,-1)));
    }
    void historyUsesRealEditionsAndNoCurrentLeakage() {
        QCOMPARE(Performance::historicalProjection({55,57,60},{100,300,600},62),800.0);
        QCOMPARE(Performance::historicalProjection({55,57,60,62},{100,300,600,1},62),800.0);
        QVERIFY(std::isnan(Performance::historicalProjection({62},{1},62)));
    }
    void endpointEscapesIdentifier() {
        QUrlQuery q;q.addQueryItem("identifier","a&rank=1");
        const auto url=WtApi::endpoint(0,"get-user","JP",q);
        QCOMPARE(QUrlQuery(url).queryItemValue("identifier"),QString("a&rank=1"));
        QVERIFY(!QUrlQuery(url).hasQueryItem("rank"));
        QCOMPARE(QUrlQuery(url).queryItemValue("region"),QString("JP"));
        QCOMPARE(url.path(),QString("/api/0/get-user"));
    }
    void asyncCacheCoalescingAndContextLifetime() {
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost));int requests=0;
        connect(&server,&QTcpServer::newConnection,&server,[&] {
            while(server.hasPendingConnections()) {
                auto *socket=server.nextPendingConnection();
                connect(socket,&QTcpSocket::readyRead,socket,[&,socket] {
                    if(socket->property("answered").toBool()) return;
                    socket->readAll();socket->setProperty("answered",true);++requests;
                    socket->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}");socket->disconnectFromHost();
                });
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            }
        });
        WtApi api;QUrl url(QString("http://127.0.0.1:%1/test").arg(server.serverPort()));
        int completed=0;auto callback=[&](const QByteArray &b,const QString &e){QVERIFY(e.isEmpty());QCOMPARE(b,QByteArray("{}"));++completed;};
        auto *context=new QObject;
        api.get(url,this,callback);api.get(url,this,callback);
        api.get(url,context,[&](const auto &,const auto &){QFAIL("Destroyed context called");});delete context;
        QTRY_COMPARE(completed,2);QCOMPARE(requests,1);
        api.get(url,this,callback);QTRY_COMPARE(completed,3);QCOMPARE(requests,1);
        api.get(url,this,callback,0);QTRY_COMPARE(completed,4);QCOMPARE(requests,2);
    }
    void batchConcurrencyAndHttpErrors() {
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost));
        int active=0,peak=0,requests=0;
        connect(&server,&QTcpServer::newConnection,&server,[&] {
            while(server.hasPendingConnections()) {
                auto *socket=server.nextPendingConnection();
                connect(socket,&QTcpSocket::readyRead,socket,[&,socket] {
                    if(socket->property("answered").toBool()) return;
                    socket->setProperty("answered",true);const auto req=socket->readAll();
                    ++requests;++active;peak=qMax(peak,active);
                    QTimer::singleShot(15,socket,[&,socket,req] {
                        --active;
                        socket->write(req.contains("/fail")?"HTTP/1.1 503 Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n":"HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}");
                        socket->disconnectFromHost();
                    });
                });
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            }
        });
        WtApi api;QList<QUrl> urls;
        for(int i=0;i<9;++i) urls.append(QUrl(QString("http://127.0.0.1:%1/%2").arg(server.serverPort()).arg(i)));
        urls.append(QUrl(QString("http://127.0.0.1:%1/fail").arg(server.serverPort())));
        bool done=false;
        api.getMany(urls,this,[&](const QHash<QUrl,WtApi::Result> &results){
            QCOMPARE(results.size(),10);QVERIFY(!results.value(urls.last()).error.isEmpty());done=true;
        });
        QTRY_VERIFY(done);QCOMPARE(requests,10);QVERIFY(peak<=4);QVERIFY(peak>1);
    }
    void pickerKeepsCurrentAndSupportsKeyboardAndDrag() {
        EditionPickerWidget picker;picker.resize(300,60);picker.show();
        int value=-1,changes=0;picker.setOnChanged([&](int v){value=v;++changes;});
        picker.setEditions({0,62,60},0);
        QTest::keyClick(&picker,Qt::Key_Right);QCOMPARE(value,62);
        QTest::keyClick(&picker,Qt::Key_Home);QCOMPARE(value,0);
        QTest::mousePress(&picker,Qt::LeftButton,Qt::NoModifier,{220,30});
        QTest::mouseMove(&picker,{80,30});QTest::mouseRelease(&picker,Qt::LeftButton,Qt::NoModifier,{80,30});QCOMPARE(value,62);
        picker.setEditions({},0);QTest::keyClick(&picker,Qt::Key_Right);QCOMPARE(changes,3);
        if(!qEnvironmentVariable("POLAR_SCREENSHOT_DIR").isEmpty()) picker.grab().save(qEnvironmentVariable("POLAR_SCREENSHOT_DIR")+"/edition-picker.png");
    }
    void chartReplacementThenResize() {
        QMainWindow window;Ui::MainWindow ui;ui.setupUi(&window);window.show();
        for(int i=0;i<20;++i) {
            Render::createLineChartInGraphicsView(&ui,"[0,0.25,0.5]","[0,12,14]","wins_pace");
            window.resize(900+i,700+i);QCoreApplication::processEvents();
        }
        QVERIFY(Render::chartFromView(ui.graphiqueTest));
        // Old event filters must die with each replaced proxy; no dangling resize callback.
        QCOMPARE(ui.graphiqueTest->findChildren<QGraphicsProxyWidget*>().size(),0); // scene owns proxy, not view QObject tree
    }
    void raceScenariosAndStaleData() {
        QJsonArray data;
        for(int i=1;i<=20;++i) data.append(QJsonObject{{"id",QString::number(i)},{"name",QString("Rival %1").arg(i)},
            {"ranks",QString("[%1,%1,%1,%1,%1]").arg(i)}, {"hour","[0,0.5,1,1.5,2]"},
            {"points",QString("[%1,%2,%3,%4,%5]").arg(200000000-i*1000000).arg(205000000-i*1000000).arg(210000000-i*1000000).arg(215000000-i*1000000).arg(220000000-i*1000000)}});
        const auto now=QDateTime::currentSecsSinceEpoch();
        QJsonObject meta{{"start_at",now-7200},{"end_at",now+3600*48}};
        RaceAnalysisDialog dialog(data,meta);dialog.show();QTest::qWait(50);
        const auto table=dialog.findChild<QTableWidget*>();QVERIFY(table);QCOMPARE(table->rowCount(),20);
        QVERIFY(table->item(0,5)->text().contains("M"));
        const auto before=table->item(0,5)->text();dialog.findChild<QDoubleSpinBox*>()->setValue(1);
        QVERIFY(before!=table->item(0,5)->text());
        if(!qEnvironmentVariable("POLAR_SCREENSHOT_DIR").isEmpty()) dialog.grab().save(qEnvironmentVariable("POLAR_SCREENSHOT_DIR")+"/race-analysis.png");
        meta["start_at"]=now-36000;
        RaceAnalysisDialog stale(data,meta);QCOMPARE(stale.findChild<QTableWidget*>()->item(0,5)->text(),QString("Unavailable"));
    }
    void mainWindowStartupAndResources() {
        MainWindow window;window.show();QTest::qWait(80);
        QVERIFY(window.findChild<QWidget*>("tbEditionPicker"));
        QVERIFY(!QPixmap(":/images/chart.png").isNull());
        QTranslator translator;QVERIFY(translator.load(":/i18n/Polar_fr_FR.qm"));
        if(!qEnvironmentVariable("POLAR_SCREENSHOT_DIR").isEmpty()) window.grab().save(qEnvironmentVariable("POLAR_SCREENSHOT_DIR")+"/main-window.png");
    }

};
QTEST_MAIN(PolarTests)
#include "tst_polar.moc"
