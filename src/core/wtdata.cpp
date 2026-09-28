#include "wtdata.h"
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <cmath>

QVector<double> WtData::numbers(const QJsonValue &value)
{
    const QJsonArray a = value.isArray() ? value.toArray()
        : QJsonDocument::fromJson(value.toString().toUtf8()).array();
    QVector<double> result;
    for (const auto &v : a) {
        bool ok = v.isDouble();
        const double n = ok ? v.toDouble() : v.toString().toDouble(&ok);
        if (!ok || !std::isfinite(n)) return {}; // Never shift parallel samples.
        result.append(n);
    }
    return result;
}

QJsonObject WtData::normalize(const QJsonDocument &doc, const QString &arrayKey)
{
    QJsonObject obj = doc.object();
    if (doc.isArray() && !arrayKey.isEmpty()) obj[arrayKey] = doc.array();
    // Preserve legacy consumers while accepting native JSON arrays from the API.
    const QSet<QString> seriesKeys = {"hour", "points", "wins", "ranks", "wins_pace",
                                     "points_pace", "points_wins", "max_points", "max_wins"};
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (seriesKeys.contains(it.key()) && it->isArray()) {
            *it = QString::fromUtf8(QJsonDocument(it->toArray()).toJson(QJsonDocument::Compact));
        } else if (it->isObject()) {
            *it = normalize(QJsonDocument(it->toObject()));
        } else if (it->isArray()) {
            auto array = it->toArray();
            for (int i=0; i<array.size(); ++i)
                if (array[i].isObject()) array[i] = normalize(QJsonDocument(array[i].toObject()));
            *it = array;
        }
    }
    return obj;
}

QJsonObject WtData::metadata(const QJsonDocument &doc)
{
    auto obj = doc.object();
    if (obj.value("metadata").isObject()) obj = obj.value("metadata").toObject();
    // The GLB endpoint legitimately has dates but no id. Do not infer max(archives)+1.
    if (obj.value("id").isString()) obj["id"] = obj.value("id").toString().toInt();
    const qint64 start = obj.value("start_at").toVariant().toLongLong();
    const qint64 end = obj.value("end_at").toVariant().toLongLong();
    if (start <= 0 || end <= start || obj.contains("error")) return {};
    return obj;
}

QVector<int> WtData::editions(const QByteArray &html, const QString &region)
{
    const QString code = (region == "JP" || region == "Jap") ? "JP" : "GLB";
    // Public archive links observed on /older_editions, not an assumed API endpoint.
    const QRegularExpression pattern("href=[\"']/edition/" + code + "/([0-9]+)\\.db[\"']");
    auto matches = pattern.globalMatch(QString::fromUtf8(html));
    QSet<int> found;
    while (matches.hasNext()) {
        const int id = matches.next().captured(1).toInt();
        if (id > 0) found.insert(id);
    }
    QVector<int> result(found.begin(), found.end());
    std::sort(result.begin(), result.end(), std::greater<int>());
    return result;
}

void WtData::filterNegativeHours(QJsonObject &obj)
{
    const auto hours = numbers(obj.value("hour"));
    if (!hours.isEmpty()) {
        for (const auto &key : {"hour", "points", "wins", "ranks", "wins_pace", "points_pace", "points_wins", "max_points", "max_wins"}) {
            const auto values = numbers(obj.value(key));
            if (values.size() != hours.size()) continue;
            QJsonArray filtered;
            for (int i=0; i<hours.size(); ++i) if (hours[i] >= 0) filtered.append(values[i]);
            obj[key] = QString::fromUtf8(QJsonDocument(filtered).toJson(QJsonDocument::Compact));
        }
    }
    for (auto it=obj.begin(); it!=obj.end(); ++it) {
        if (it->isObject()) { auto child=it->toObject(); filterNegativeHours(child); *it=child; }
        else if (it->isArray()) {
            auto a=it->toArray();
            for (int i=0;i<a.size();++i) if (a[i].isObject()) {
                auto child=a[i].toObject(); filterNegativeHours(child); a[i]=child;
            }
            *it=a;
        }
    }
}
