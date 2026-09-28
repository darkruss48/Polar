#pragma once
#include <QJsonObject>
#include <QVector>
#include <limits>

namespace Performance {
constexpr double unavailable = std::numeric_limits<double>::quiet_NaN();
struct Sample { double hour; double points; };
struct Summary {
    bool valid = false;
    double lastHour = 0, points = 0;
    double recentRate = unavailable, activeRate = unavailable, observedRate = unavailable;
    double activeHours = 0, idleHours = 0, unknownHours = 0, coverage = 0;
    double trailingIdleHours = 0;
};
QVector<Sample> samples(const QJsonObject &player);
Summary summarize(const QVector<Sample> &samples, double windowHours = 2.0, double maxGapHours = 0.75);
double project(const Summary &summary, double remainingHours, double plannedPause = 0, bool observedHabit = false);
double catchHours(double gap, double pursuerRate, double leaderRate, double remainingHours);
double historicalProjection(const QVector<int> &editions, const QVector<qint64> &scores, int targetEdition);
}
