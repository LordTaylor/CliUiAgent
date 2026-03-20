#include "Session.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace CodeHex {

void Session::appendMessage(const Message& msg) {
    messages.append(msg);
    updatedAt = QDateTime::currentDateTimeUtc();
}

void Session::updateTokens(int inputDelta, int outputDelta) {
    tokens.input += inputDelta;
    tokens.output += outputDelta;
    tokens.total = tokens.input + tokens.output;
    updatedAt = QDateTime::currentDateTime();
}

void Session::clear() {
    messages.clear();
    tokens = {0,0,0};
    updatedAt = QDateTime::currentDateTime();
}

bool Session::save() const {
    if (filePath.isEmpty()) return false;
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    return true;
}

Session Session::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    auto doc = QJsonDocument::fromJson(f.readAll());
    return fromJson(doc.object(), path);
}

Session Session::createNew(const QString& profileName, const QString& modelName) {
    Session s;
    s.id = QUuid::createUuid();
    s.profileName = profileName;
    s.modelName = modelName;
    s.title = "New Chat";
    s.createdAt = QDateTime::currentDateTimeUtc();
    s.updatedAt = s.createdAt;
    return s;
}

QJsonObject Session::toJson() const {
    QJsonArray msgArr;
    for (const auto& m : messages) msgArr.append(m.toJson());

    QJsonObject tokObj;
    tokObj["input"] = tokens.input;
    tokObj["output"] = tokens.output;
    tokObj["total"] = tokens.total;

    return {
        {"id", id.toString(QUuid::WithoutBraces)},
        {"title", title},
        {"model", modelName},
        {"profile", profileName},
        {"createdAt", createdAt.toUTC().toString(Qt::ISODate)},
        {"updatedAt", updatedAt.toUTC().toString(Qt::ISODate)},
        {"tokens", tokObj},
        {"messages", msgArr},
    };
}

Session Session::fromJson(const QJsonObject& obj, const QString& filePath) {
    Session s;
    s.id = QUuid(obj["id"].toString());
    s.title = obj["title"].toString();
    s.modelName = obj["model"].toString();
    s.profileName = obj["profile"].toString();
    s.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
    s.updatedAt = QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate);
    s.filePath = filePath;

    const auto tok = obj["tokens"].toObject();
    s.tokens.input = tok["input"].toInt();
    s.tokens.output = tok["output"].toInt();
    s.tokens.total = tok["total"].toInt();

    for (const auto& v : obj["messages"].toArray())
        s.messages.append(Message::fromJson(v.toObject()));

    return s;
}

}  // namespace CodeHex
