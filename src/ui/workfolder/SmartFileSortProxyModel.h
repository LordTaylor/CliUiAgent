#pragma once
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QObject>
#include <QString>
#include "../../core/FileHotnessProvider.h"

namespace CodeHex {

/**
 * @brief Logic for sorting files by "Hotness" (Git history).
 */
class SmartFileSortProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SmartFileSortProxyModel(FileHotnessProvider* provider, QObject* parent = nullptr) 
        : QSortFilterProxyModel(parent), m_provider(provider) {
        setSortCaseSensitivity(Qt::CaseInsensitive);
    }

    void setSortByHotness(bool enabled) {
        if (m_sortByHotness == enabled) return;
        m_sortByHotness = enabled;
        invalidate(); // Re-sort everything
    }

    bool sortByHotness() const { return m_sortByHotness; }

protected:
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override {
        if (!m_sortByHotness) {
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }

        auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (!fsModel) return QSortFilterProxyModel::lessThan(source_left, source_right);

        QString pathLeft = fsModel->filePath(source_left);
        QString pathRight = fsModel->filePath(source_right);
        bool isDirLeft = fsModel->isDir(source_left);
        bool isDirRight = fsModel->isDir(source_right);

        // Always show directories before files
        if (isDirLeft && !isDirRight) return true;
        if (!isDirLeft && isDirRight) return false;

        // If both are files, sort by hotness
        if (!isDirLeft && !isDirRight) {
            int hotLeft = m_provider->getHotness(pathLeft);
            int hotRight = m_provider->getHotness(pathRight);

            if (hotLeft != hotRight) {
                // Higher hotness comes first
                return hotLeft < hotRight; // Note: QSortFilterProxyModel inversion depends on sort order (Asc/Desc)
            }
        }

        // Fallback to alphabetical
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

private:
    FileHotnessProvider* m_provider;
    bool m_sortByHotness = false;
};

} // namespace CodeHex
