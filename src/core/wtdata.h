#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>

namespace WtData {
QJsonObject normalize(const QJsonDocument &doc, const QString &arrayKey = {});
QJsonObject metadata(const QJsonDocument &doc);
QVector<int> editions(const QByteArray &html, const QString &region);
QVector<double> numbers(const QJsonValue &value);
void filterNegativeHours(QJsonObject &object);
}
