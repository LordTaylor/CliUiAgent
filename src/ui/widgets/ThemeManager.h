#pragma once
#include <QObject>
#include <QString>
#include <QMap>

namespace CodeHex {

class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    void setTheme(bool dark);
    bool isDark() const { return m_isDark; }
    
    QString currentStyleSheet() const;

signals:
    void themeChanged(bool dark);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    QString processQss(const QString& rawQss) const;
    
    bool m_isDark = true;
    QMap<QString, QString> m_darkVars;
    QMap<QString, QString> m_lightVars;

    void initVariables();
};

} // namespace CodeHex
