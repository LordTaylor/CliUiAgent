#pragma once
#include <QJsonObject>
#include <QString>

// Thin utility wrappers around QJsonDocument for file I/O.
// Complex serialization logic lives in each model's toJson/fromJson.
namespace CodeHex::JsonSerializer {

QJsonObject readFile(const QString& path);
bool writeFile(const QString& path, const QJsonObject& obj);

}  // namespace CodeHex::JsonSerializer
