#include "performanceanalysis.h"
#include "wtdata.h"
#include <algorithm>
#include <cmath>

QVector<Performance::Sample> Performance::samples(const QJsonObject &player) {
    const auto hours=WtData::numbers(player.value("hour"));
    const auto points=WtData::numbers(player.value("points"));
    if(hours.size()!=points.size()) return {};
    QVector<Sample> result;
    for(int i=0;i<hours.size();++i) {
        if(hours[i]<0) continue;
        if(points[i]<0 || (!result.isEmpty() && hours[i]<=result.last().hour)) return {};
        result.append({hours[i],points[i]});
    }
    return result;
}
Performance::Summary Performance::summarize(const QVector<Sample> &s,double windowHours,double maxGapHours) {
    Summary out;
    if(s.size()<2 || windowHours<=0 || maxGapHours<=0) return out;
    for(int i=0;i<s.size();++i)
        if(!std::isfinite(s[i].hour) || !std::isfinite(s[i].points) || s[i].points<0 ||
           (i && s[i].hour<=s[i-1].hour)) return out;
    out.lastHour=s.last().hour; out.points=s.last().points;
    double gained=0, recentGain=0, recentCovered=0;
    const double windowStart=std::max(s.first().hour,out.lastHour-windowHours);
    for(int i=1;i<s.size();++i) {
        const double dt=s[i].hour-s[i-1].hour, dp=s[i].points-s[i-1].points;
        if(dt>maxGapHours || dp<0) {
            out.unknownHours+=dt; out.trailingIdleHours=0; continue;
        }
        if(dp==0) {out.idleHours+=dt;out.trailingIdleHours+=dt;}
        else {out.activeHours+=dt;out.trailingIdleHours=0;gained+=dp;}
        const double overlap=std::max(0.0,s[i].hour-std::max(windowStart,s[i-1].hour));
        recentCovered+=overlap; recentGain+=dp*overlap/dt;
    }
    const double known=out.activeHours+out.idleHours;
    out.coverage=known/(s.last().hour-s.first().hour);
    if(out.activeHours>0) out.activeRate=gained/out.activeHours;
    if(known>0 && out.coverage>=0.75) out.observedRate=gained/known;
    const double recentSpan=out.lastHour-windowStart;
    // Require at least half an hour, adequate coverage, and a trustworthy last interval.
    const double lastDt=s.last().hour-s[s.size()-2].hour;
    const bool lastValid=lastDt<=maxGapHours && s.last().points>=s[s.size()-2].points;
    if(recentSpan>=0.5 && recentCovered>=0.75*recentSpan && lastValid)
        out.recentRate=recentGain/recentCovered;
    if(!lastValid) out.observedRate=unavailable;
    out.valid=known>0;
    return out;
}
double Performance::project(const Summary &s,double remaining,double pause,bool habit) {
    if(!s.valid || !std::isfinite(remaining) || !std::isfinite(pause) || remaining<0 || pause<0) return unavailable;
    if(remaining==0) return s.points;
    const double rate=habit?s.observedRate:s.recentRate;
    if(!std::isfinite(rate)) return unavailable;
    return s.points+rate*std::max(0.0,remaining-pause);
}
double Performance::catchHours(double gap,double pursuer,double leader,double remaining) {
    if(!std::isfinite(gap)||!std::isfinite(pursuer)||!std::isfinite(leader)||!std::isfinite(remaining)||
       gap<=0||pursuer<0||leader<0||pursuer<=leader||remaining<=0) return unavailable;
    const double eta=gap/(pursuer-leader);
    return eta<=remaining?eta:unavailable;
}
double Performance::historicalProjection(const QVector<int> &editions,const QVector<qint64> &scores,int target) {
    if(editions.size()!=scores.size() || editions.isEmpty()) return unavailable;
    double sx=0,sy=0,sxx=0,sxy=0;int n=0;
    for(int i=0;i<editions.size();++i) {
        if(editions[i]>=target || scores[i]<0) continue;
        const double x=editions[i]-target,y=scores[i];
        sx+=x;sy+=y;sxx+=x*x;sxy+=x*y;++n;
    }
    if(!n) return unavailable;
    const double denominator=n*sxx-sx*sx;
    if(n==1 || denominator==0) return sy/n;
    const double slope=(n*sxy-sx*sy)/denominator;
    return std::max(0.0,(sy-slope*sx)/n);
}
