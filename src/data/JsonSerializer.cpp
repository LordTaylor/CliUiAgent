#include "JsonSerializer.h"
#include <QFile>
#include <QJsonDocument>

namespace CodeHex::JsonSerializer {

QJsonObject readFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

bool writeFile(const QString& path, const QJsonObject& obj) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

}  // namespace CodeHex::JsonSerializer
