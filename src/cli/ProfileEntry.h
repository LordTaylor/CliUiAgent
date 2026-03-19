#pragma once
#include <QList>
#include <QString>

namespace CodeHex {

// Lightweight descriptor for a profile loaded from ~/.codehex/profiles/*.json.
// Shared between Application (discovery) and MainWindow (combo box).
struct ProfileEntry {
    QString name;        // internal id — matches JSON "name" field
    QString displayName; // shown in UI combo box
    QString filePath;    // absolute path to the .json file
};

using ProfileList = QList<ProfileEntry>;

}  // namespace CodeHex
