#pragma once
#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include "../../core/AppConfig.h"
#include "../../core/LlmDiscoveryService.h"
#include "../../core/HuggingFaceImporter.h"
#include "../../data/ModelProfile.h"

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

    void onModelNameChanged(const QString& modelName);
    void onDownloadProfile();
    void onHfProfileImported(const ModelProfile& profile, const QString& savedPath);
    void onHfError(const QString& error);

private:
    void setupUi();
    void loadProvider(int index);
    void updateList();
    void updateProfileStatus(const QString& modelName);

    AppConfig*            m_config;
    LlmDiscoveryService*  m_discovery;
    HuggingFaceImporter*  m_hfImporter = nullptr;
    LlmProviderList       m_workingList;
    int                   m_currentIndex = -1;

    QListWidget* m_providerList;
    QLineEdit*   m_nameEdit;
    QComboBox*   m_typeCombo;
    QLineEdit*   m_urlEdit;
    QLineEdit*   m_keyEdit;
    QComboBox*   m_modelCombo;
    QSpinBox*    m_contextWindowSpin;
    QLabel*      m_statusLabel;

    // HuggingFace profile section
    QLineEdit*   m_hfRepoEdit;
    QPushButton* m_hfDownloadBtn;
    QLabel*      m_profileStatusLabel;
};

} // namespace CodeHex
