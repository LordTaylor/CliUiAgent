#pragma once
#include <QTextDocument>
#include <QList>
#include <memory>

namespace CodeHex {

struct PrecomputedLayout {
    struct BlockLayout {
        std::shared_ptr<QTextDocument> doc;
        int height = 0;
        int width = 0;
    };

    QList<BlockLayout> blocks;
    int totalHeight = 0;
    int lastViewWidth = 0;
};

} // namespace CodeHex
