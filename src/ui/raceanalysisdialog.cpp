#include "raceanalysisdialog.h"
#include "performanceanalysis.h"
#include "wtdata.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace {
QString millions(double v) {return std::isfinite(v)?QString::number(v/1e6,'f',1)+"M":QStringLiteral("—");}
QString hours(double v) {return std::isfinite(v)?QString::number(v,'f',2)+" h":QStringLiteral("—");}
int rank(const QJsonObject &o) {const auto v=WtData::numbers(o.value("ranks"));return v.isEmpty()?100000:int(v.last());}
}
RaceAnalysisDialog::RaceAnalysisDialog(const QJsonArray &data,const QJsonObject &meta,QWidget *parent)
    : QDialog(parent),metadata(meta) {
    setWindowTitle(tr("Race analysis · Top 20"));resize(1140,650);
    QVector<QJsonObject> sorted;
    for(const auto &v:data) if(rank(v.toObject())>0 && rank(v.toObject())<=20) sorted.append(v.toObject());
    std::sort(sorted.begin(),sorted.end(),[](const auto &a,const auto &b){return rank(a)<rank(b);});
    auto *layout=new QVBoxLayout(this);auto *controls=new QHBoxLayout;
    focusBox=new QComboBox(this);windowBox=new QComboBox(this);pauseBox=new QDoubleSpinBox(this);
    for(const auto &o:sorted) {players.append(o);focusBox->addItem(QString("#%1 %2").arg(rank(o)).arg(o.value("name").toString()));}
    for(int h:{1,2,6}) windowBox->addItem(QString::number(h)+" h",h);
    windowBox->setCurrentIndex(1);pauseBox->setRange(0,24);pauseBox->setSingleStep(0.25);pauseBox->setSuffix(" h");
    focusBox->setToolTip(tr("Reference player for gaps and catch-up estimates. Player identity is kept by row, never by name."));
    windowBox->setToolTip(tr("Recent scoring window. Actual timestamps are used; gaps over 45 minutes and score resets are unknown."));
    pauseBox->setToolTip(tr("Additional future pause for the reference player only. Habit pace already includes observed idle periods."));
    controls->addWidget(new QLabel(tr("Player"),this));controls->addWidget(focusBox,1);
    controls->addWidget(new QLabel(tr("Window"),this));controls->addWidget(windowBox);
    controls->addWidget(new QLabel(tr("Pause"),this));controls->addWidget(pauseBox);layout->addLayout(controls);
    table=new QTableWidget(players.size(),8,this);
    table->setHorizontalHeaderLabels({tr("Player"),tr("Points"),tr("Recent / h"),tr("Active"),tr("Idle streak"),tr("Finish scenarios"),tr("Gap"),tr("Catch-up")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->hide();table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);
    table->setColumnWidth(0,210);
    table->horizontalHeader()->setSectionResizeMode(5,QHeaderView::Stretch);
    layout->addWidget(table,1);status=new QLabel(this);layout->addWidget(status);
    auto *ageTimer=new QTimer(this);
    connect(ageTimer,&QTimer::timeout,this,[this]{refresh();});ageTimer->start(60000);
    connect(focusBox,&QComboBox::currentIndexChanged,this,[this]{refresh();});
    connect(windowBox,&QComboBox::currentIndexChanged,this,[this]{refresh();});
    connect(pauseBox,&QDoubleSpinBox::valueChanged,this,[this]{refresh();});refresh();
}
void RaceAnalysisDialog::refresh() {
    if(players.isEmpty()) {status->setText(tr("No Top 20 data"));return;}
    const double duration=(metadata.value("end_at").toVariant().toLongLong()-metadata.value("start_at").toVariant().toLongLong())/3600.0;
    const bool datesValid=metadata.value("start_at").toVariant().toLongLong()>0 && duration>0;
    const bool ended=datesValid && metadata.value("end_at").toVariant().toLongLong()<=QDateTime::currentSecsSinceEpoch();
    QVector<Performance::Summary> stats;
    for(const auto &p:players) stats.append(Performance::summarize(Performance::samples(p.toObject()),windowBox->currentData().toDouble()));
    const int selected=qMax(0,focusBox->currentIndex());const auto &focus=stats[selected];
    const double referenceRemaining=datesValid?std::max(0.0,duration-focus.lastHour):0;
    const double age=datesValid?(QDateTime::currentSecsSinceEpoch()-metadata.value("start_at").toVariant().toLongLong())/3600.0-focus.lastHour:0;
    // A live forecast older than an hour is not actionable. Historical rows show recorded results.
    const bool fresh=ended || (datesValid && age>=-0.05 && age<=1.0);
    for(int i=0;i<players.size();++i) {
        const auto o=players[i].toObject();const auto &s=stats[i];
        const double remaining=datesValid?std::max(0.0,duration-s.lastHour):0;
        const double rowAge=datesValid?(QDateTime::currentSecsSinceEpoch()-metadata.value("start_at").toVariant().toLongLong())/3600.0-s.lastHour:0;
        const bool rowFresh=ended || (datesValid && rowAge>=-0.05 && rowAge<=1.0);
        const double pause=i==selected?pauseBox->value():0;
        const double recent=Performance::project(s,remaining,pause);
        const double habit=Performance::project(s,remaining,pause,true);
        QString forecast=tr("Unavailable");
        if(ended) forecast=millions(s.valid?s.points:Performance::unavailable);
        else if(fresh && rowFresh && datesValid && std::isfinite(recent) && std::isfinite(habit))
            forecast=millions(std::min(recent,habit))+" – "+millions(std::max(recent,habit));
        const bool aligned=s.valid && focus.valid && std::abs(s.lastHour-focus.lastHour)<=0.05;
        const double gap=aligned?s.points-focus.points:Performance::unavailable;
        // ETA assumes no planned pause: don't quietly mix a pause scenario with constant-pace ETA.
        double eta=Performance::unavailable;
        if(aligned && fresh && rowFresh && !ended && pauseBox->value()==0)
            eta=gap>0?Performance::catchHours(gap,focus.recentRate,s.recentRate,referenceRemaining)
                     :Performance::catchHours(-gap,s.recentRate,focus.recentRate,referenceRemaining);
        const double known=s.activeHours+s.idleHours;
        const QStringList values={QString("#%1 %2").arg(rank(o)).arg(o.value("name").toString()),millions(s.valid?s.points:Performance::unavailable),
            millions(s.recentRate),known>0?QString::number(100*s.activeHours/known,'f',0)+"%":QStringLiteral("—"),
            s.valid?hours(s.trailingIdleHours):QStringLiteral("—"),forecast,i==selected?QStringLiteral("—"):millions(gap),hours(eta)};
        for(int c=0;c<values.size();++c) {
            auto *item=new QTableWidgetItem(values[c]);
            item->setToolTip(tr("Coverage: %1%. Unknown: %2 h. Last sample: %3 h.\nFinish range compares recent pace with observed active/idle habits; it is not a confidence interval.\nCatch-up uses constant recent paces, aligned samples and the tournament deadline. A dash means unavailable or no catch before the end.")
                .arg(s.coverage*100,0,'f',0).arg(s.unknownHours,0,'f',2).arg(s.lastHour,0,'f',2));
            if(i==selected) {auto f=item->font();f.setBold(true);item->setFont(f);}
            table->setItem(i,c,item);
        }
    }
    status->setText(ended?tr("Tournament ended · recorded scores") : !datesValid?tr("Tournament dates unavailable") : !fresh?tr("Stale data · projections unavailable") : tr("Snapshot · %1 min old").arg(qMax(0,int(age*60))));
}
