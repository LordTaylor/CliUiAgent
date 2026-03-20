#pragma once
#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "../../core/AppConfig.h"
#include "../../core/LlmDiscoveryService.h"

namespace CodeHex {

/**
 * @brief Dialog to manage LLM provider configurations.
 */
class ProviderSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProviderSettingsDialog(AppConfig* config, QWidget* parent = nullptr);

private slots:
    void onAddProvider();
    void onRemoveProvider();
    void onProviderSelected(int index);
    void onFetchModels();
    void onTestConnection();
    void onTypeChanged(const QString& type);
    void onSave();
    void onModelsReady(const QStringList& models);
    void onDiscoveryError(const QString& error);

private:
    void setupUi();
    void loadProvider(int index);
    void updateList();

    AppConfig* m_config;
    LlmDiscoveryService* m_discovery;
    LlmProviderList m_workingList;
    int m_currentIndex = -1;

    QListWidget* m_providerList;
    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;
    QLineEdit* m_urlEdit;
    QLineEdit* m_keyEdit;
    QComboBox* m_modelCombo;
    QLabel* m_statusLabel;
};

} // namespace CodeHex
