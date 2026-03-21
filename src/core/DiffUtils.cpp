#include "DiffUtils.h"
#include <QStringList>

namespace CodeHex {

QList<DiffUtils::DiffLine> DiffUtils::generateDiff(const QString& oldText, const QString& newText) {
    QList<DiffLine> result;
    QStringList oldLines = oldText.split('\n');
    QStringList newLines = newText.split('\n');

    // This is a trivial diff (not a full Myers/LCS)
    // For now, it just shows what changed at a line level if they were identical otherwise
    // In a real app we'd use a library, but let's do a basic one.
    
    int i = 0, j = 0;
    while (i < oldLines.size() || j < newLines.size()) {
        if (i < oldLines.size() && j < newLines.size() && oldLines[i] == newLines[j]) {
            result.append({DiffLine::Unchanged, oldLines[i]});
            i++; j++;
        } else if (i < oldLines.size() && (j >= newLines.size() || oldLines[i] != newLines[j])) {
            result.append({DiffLine::Removed, oldLines[i]});
            i++;
        } else if (j < newLines.size()) {
            result.append({DiffLine::Added, newLines[j]});
            j++;
        }
    }
    
    return result;
}

QString DiffUtils::toUnifiedString(const QList<DiffLine>& diff) {
    QString res;
    for (const auto& line : diff) {
        if (line.type == DiffLine::Added) res += "+ " + line.text + "\n";
        else if (line.type == DiffLine::Removed) res += "- " + line.text + "\n";
        else res += "  " + line.text + "\n";
    }
    return res;
}

} // namespace CodeHex
