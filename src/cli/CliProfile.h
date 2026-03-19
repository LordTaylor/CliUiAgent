#pragma once
#include <QStringList>
#include <QString>

namespace CodeHex {

class CliProfile {
public:
    virtual ~CliProfile() = default;

    virtual QString name() const = 0;
    virtual QString displayName() const = 0;
    virtual QString executable() const = 0;
    virtual QStringList buildArguments(const QString& prompt,
                                       const QString& workDir) const = 0;
    // Parse a raw stdout line/chunk and return the text token (empty = skip)
    virtual QString parseStreamChunk(const QByteArray& raw) const = 0;
    // Default model identifier shown in UI
    virtual QString defaultModel() const = 0;
};

}  // namespace CodeHex
