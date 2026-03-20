#pragma once
#include <QDialog>

class QListWidget;

namespace CodeHex {

class SkillsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SkillsDialog(QWidget* parent = nullptr);

private:
    void setupUi();
    void addSkill(const QString& name, const QString& desc, const QString& icon);
    
    QListWidget* m_list;
};

} // namespace CodeHex
