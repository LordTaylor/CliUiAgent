#include "ThemeManager.h"
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDebug>

namespace CodeHex {

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {
    initVariables();
}

void ThemeManager::initVariables() {
    // DARK THEME
    m_darkVars["{{BG_MAIN}}"]     = "#0F172A";
    m_darkVars["{{BG_DARKER}}"]  = "#0B0F1A";
    m_darkVars["{{BG_WIDGET}}"]  = "#1F2937";
    m_darkVars["{{BG_HOVER}}"]   = "#374151";
    m_darkVars["{{TEXT_MAIN}}"]  = "#F8FAFC";
    m_darkVars["{{TEXT_DIM}}"]   = "#9CA3AF";
    m_darkVars["{{ACCENT}}"]     = "#2563EB";
    m_darkVars["{{ACCENT_HOVER}}"] = "#3B82F6";
    m_darkVars["{{BORDER}}"]     = "#374151";
    m_darkVars["{{GLASS_BG}}"]   = "rgba(30, 41, 59, 0.7)";
    m_darkVars["{{GLASS_BORDER}}"] = "rgba(71, 85, 105, 0.3)";

    // LIGHT THEME (New Palette!)
    m_lightVars["{{BG_MAIN}}"]     = "#F1F5F9";
    m_lightVars["{{BG_DARKER}}"]  = "#E2E8F0";
    m_lightVars["{{BG_WIDGET}}"]  = "#FFFFFF";
    m_lightVars["{{BG_HOVER}}"]   = "#F8FAFC";
    m_lightVars["{{TEXT_MAIN}}"]  = "#0F172A";
    m_lightVars["{{TEXT_DIM}}"]   = "#64748B";
    m_lightVars["{{ACCENT}}"]     = "#3B82F6";
    m_lightVars["{{ACCENT_HOVER}}"] = "#2563EB";
    m_lightVars["{{BORDER}}"]     = "#CBD5E1";
    m_lightVars["{{GLASS_BG}}"]   = "rgba(255, 255, 255, 0.8)";
    m_lightVars["{{GLASS_BORDER}}"] = "rgba(203, 213, 225, 0.5)";
}

void ThemeManager::setFontFamily(const QString& family) {
    if (m_fontFamily == family) return;
    m_fontFamily = family;
    qApp->setStyleSheet(currentStyleSheet());
}

void ThemeManager::setTheme(bool dark) {
    if (m_isDark == dark) return;
    m_isDark = dark;
    
    qApp->setStyleSheet(currentStyleSheet());
    emit themeChanged(dark);
}

QString ThemeManager::currentStyleSheet() const {
    QFile file(":/resources/stylesheets/theme_template.qss");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[ThemeManager] Could not open template QSS";
        return "";
    }
    
    QTextStream in(&file);
    QString raw = in.readAll();
    return processQss(raw);
}

QString ThemeManager::processQss(const QString& rawQss) const {
    QString processed = rawQss;
    const auto& vars = m_isDark ? m_darkVars : m_lightVars;
    
    auto it = vars.begin();
    while (it != vars.end()) {
        processed.replace(it.key(), it.value());
        ++it;
    }
    processed.replace("{{FONT_FAMILY}}", m_fontFamily);
    return processed;
}

} // namespace CodeHex
