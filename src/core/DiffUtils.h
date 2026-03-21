#pragma once
#include <QString>
#include <QStringList>

namespace CodeHex {

/**
 * @brief Simple utility to generate unified diffs.
 */
class DiffUtils {
public:
    struct DiffLine {
        enum Type { Unchanged, Added, Removed };
        Type type;
        QString text;
    };

    static QList<DiffLine> generateDiff(const QString& oldText, const QString& newText);
    static QString toUnifiedString(const QList<DiffLine>& diff);
};

} // namespace CodeHex
