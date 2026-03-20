#include "HelpDialog.h"
#include <QFile>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>

namespace CodeHex {

// Table of contents: { display label, resource page key }
static const QList<QPair<QString, QString>> TOC = {
    {"Getting Started",        "getting-started"},
    {"Interface Guide",        "ui-guide"},
    {"Sessions",               "sessions"},
    {"CLI Profiles & Models",  "cli-profiles"},
    {"Scripting (Lua/Python)", "scripting"},
    {"Voice & Attachments",    "voice-and-attachments"},
    {"Keyboard Shortcuts",     "keyboard-shortcuts"},
};

HelpDialog::HelpDialog(const QString& startPage, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("CodeHex — Help");
    resize(960, 680);
    setupUi();
    openPage(startPage);
}

void HelpDialog::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header bar ────────────────────────────────────────────────
    auto* header = new QWidget(this);
    header->setObjectName("helpHeader");
    header->setFixedHeight(40);
    auto* hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(8, 4, 8, 4);
    hLayout->setSpacing(4);

    m_backBtn = new QPushButton("◀", header);
    m_backBtn->setObjectName("helpNavBtn");
    m_backBtn->setFixedSize(28, 28);
    m_backBtn->setEnabled(false);
    m_fwdBtn = new QPushButton("▶", header);
    m_fwdBtn->setObjectName("helpNavBtn");
    m_fwdBtn->setFixedSize(28, 28);
    m_fwdBtn->setEnabled(false);

    m_titleLabel = new QLabel("Help", header);
    m_titleLabel->setObjectName("helpTitle");

    hLayout->addWidget(m_backBtn);
    hLayout->addWidget(m_fwdBtn);
    hLayout->addSpacing(8);
    hLayout->addWidget(m_titleLabel, 1);
    root->addWidget(header);

    // ── Splitter: TOC list | browser ──────────────────────────────
    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_toc = new QListWidget(m_splitter);
    m_toc->setObjectName("helpToc");
    m_toc->setFixedWidth(200);
    m_toc->setFrameShape(QFrame::NoFrame);

    for (const auto& entry : TOC) {
        auto* item = new QListWidgetItem(entry.first, m_toc);
        item->setData(Qt::UserRole, entry.second);
    }

    m_browser = new QTextBrowser(m_splitter);
    m_browser->setObjectName("helpBrowser");
    m_browser->setFrameShape(QFrame::NoFrame);
    m_browser->setOpenLinks(false);   // we handle link clicks ourselves

    m_splitter->addWidget(m_toc);
    m_splitter->addWidget(m_browser);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    root->addWidget(m_splitter, 1);

    // ── Connections ───────────────────────────────────────────────
    connect(m_toc, &QListWidget::itemClicked, this, &HelpDialog::onPageSelected);
    connect(m_backBtn,  &QPushButton::clicked, this, &HelpDialog::onBack);
    connect(m_fwdBtn,   &QPushButton::clicked, this, &HelpDialog::onForward);

    // Handle internal links like [[wiki-links]] and [text](page.md)
    connect(m_browser, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        QString s = url.toString();
        // Strip .md extension and path prefix if any
        s.remove(QRegularExpression("\\.md$"));
        s = s.split('/').last();
        openPage(s);
    });
}

void HelpDialog::openPage(const QString& page) {
    const QString path = ":/help/en/" + page + ".md";
    loadPage(path);

    // Truncate forward history
    if (m_historyPos < m_history.size() - 1)
        m_history = m_history.mid(0, m_historyPos + 1);
    m_history.append(page);
    m_historyPos = m_history.size() - 1;
    updateNavButtons();

    // Sync TOC selection
    for (int i = 0; i < m_toc->count(); ++i) {
        if (m_toc->item(i)->data(Qt::UserRole).toString() == page) {
            m_toc->setCurrentRow(i);
            break;
        }
    }
}

void HelpDialog::loadPage(const QString& resourcePath) {
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly)) {
        m_browser->setPlainText(
            QString("Page not found: %1\n\nMake sure the app was built with help resources.")
                .arg(resourcePath));
        return;
    }

    QString md = QString::fromUtf8(f.readAll());

    // Strip Obsidian/wikilink-style breadcrumb line that starts with "> [[index"
    md.remove(QRegularExpression(R"(^> \[\[.*?\]\].*\n)", QRegularExpression::MultilineOption));
    // Strip language switcher lines  🇵🇱 ...
    md.remove(QRegularExpression(R"(> 🇵🇱.*\n)"));

    QTextDocument doc;
    doc.setMarkdown(md);

    // Extract page title from first H1
    const QString title = doc.metaInformation(QTextDocument::DocumentTitle);
    m_titleLabel->setText(title.isEmpty() ? "Help" : title);

    m_browser->setDocument(nullptr);
    m_browser->setMarkdown(md);
    m_browser->scrollToAnchor({});
    m_browser->verticalScrollBar()->setValue(0);
}

void HelpDialog::onPageSelected(QListWidgetItem* item) {
    openPage(item->data(Qt::UserRole).toString());
}

void HelpDialog::onBack() {
    if (m_historyPos <= 0) return;
    --m_historyPos;
    loadPage(":/help/en/" + m_history[m_historyPos] + ".md");
    updateNavButtons();

    for (int i = 0; i < m_toc->count(); ++i) {
        if (m_toc->item(i)->data(Qt::UserRole).toString() == m_history[m_historyPos]) {
            m_toc->setCurrentRow(i);
            break;
        }
    }
}

void HelpDialog::onForward() {
    if (m_historyPos >= m_history.size() - 1) return;
    ++m_historyPos;
    loadPage(":/help/en/" + m_history[m_historyPos] + ".md");
    updateNavButtons();

    for (int i = 0; i < m_toc->count(); ++i) {
        if (m_toc->item(i)->data(Qt::UserRole).toString() == m_history[m_historyPos]) {
            m_toc->setCurrentRow(i);
            break;
        }
    }
}

void HelpDialog::updateNavButtons() {
    m_backBtn->setEnabled(m_historyPos > 0);
    m_fwdBtn->setEnabled(m_historyPos < m_history.size() - 1);
}

}  // namespace CodeHex
