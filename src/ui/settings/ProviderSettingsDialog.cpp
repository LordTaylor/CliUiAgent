#include "ProviderSettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFormLayout>
#include <QUuid>
#include <QIcon>
#include <QGroupBox>
#include <QSpinBox>

namespace CodeHex {

/**
 * @brief Manages LLM provider configurations.
 * Allows adding, removing, and testing LLM providers (Ollama, LM Studio, OpenAI).
 */
ProviderSettingsDialog::ProviderSettingsDialog(AppConfig* config, QWidget* parent)
    : QDialog(parent), m_config(config) {
    setWindowTitle("LLM Provider Manager");
    resize(700, 450);
    setMinimumSize(600, 400);

    m_discovery = new LlmDiscoveryService(this);
    m_workingList = m_config->providers();

    setupUi();

    connect(m_discovery, &LlmDiscoveryService::modelsReady, this, &ProviderSettingsDialog::onModelsReady);
    connect(m_discovery, &LlmDiscoveryService::errorOccurred, this, &ProviderSettingsDialog::onDiscoveryError);

    updateList();
    if (!m_workingList.isEmpty()) {
        m_providerList->setCurrentRow(0);
    }
}

void ProviderSettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    // Left Side: List
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(12, 12, 12, 12);

    auto* listHeader = new QLabel("<b>Available Providers</b>", leftWidget);
    leftLayout->addWidget(listHeader);

    m_providerList = new QListWidget(leftWidget);
    m_providerList->setObjectName("providerList");
    connect(m_providerList, &QListWidget::currentRowChanged, this, &ProviderSettingsDialog::onProviderSelected);
    leftLayout->addWidget(m_providerList);

    auto* listButtons = new QHBoxLayout();
    auto* addBtn = new QPushButton("Add", leftWidget);
    auto* delBtn = new QPushButton("Remove", leftWidget);
    connect(addBtn, &QPushButton::clicked, this, &ProviderSettingsDialog::onAddProvider);
    connect(delBtn, &QPushButton::clicked, this, &ProviderSettingsDialog::onRemoveProvider);
    listButtons->addWidget(addBtn);
    listButtons->addWidget(delBtn);
    leftLayout->addLayout(listButtons);

    splitter->addWidget(leftWidget);

    // Right Side: Editor
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(20, 20, 20, 20);

    auto* formGroupBox = new QGroupBox("Provider Configuration", rightWidget);
    auto* formLayout = new QFormLayout(formGroupBox);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setSpacing(12);

    m_nameEdit = new QLineEdit(formGroupBox);
    m_nameEdit->setToolTip("A friendly display name for this provider (e.g., 'Local Ollama' or 'OpenAI API').");
    
    m_typeCombo = new QComboBox(formGroupBox);
    m_typeCombo->addItems({"ollama", "lmstudio", "openai", "custom"});
    m_typeCombo->setToolTip("Select the provider type. This determines how API requests are formatted.");
    connect(m_typeCombo, &QComboBox::currentTextChanged, this, &ProviderSettingsDialog::onTypeChanged);

    m_urlEdit = new QLineEdit(formGroupBox);
    m_urlEdit->setToolTip("The base URL of the LLM provider (e.g., http://localhost:11434).");
    
    m_keyEdit = new QLineEdit(formGroupBox);
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setToolTip("The API authentication key. Leave blank for local providers like Ollama or LM Studio.");

    m_modelCombo = new QComboBox(formGroupBox);
    m_modelCombo->setEditable(true);
    m_modelCombo->setToolTip("The specific model identifier to be used (e.g., 'llama3' or 'gpt-4o'). You can type it or fetch from the provider.");

    auto* fetchBtn = new QPushButton("Fetch Models", formGroupBox);
    fetchBtn->setToolTip("Query the configured endpoint URL to retrieve a list of available models.");
    connect(fetchBtn, &QPushButton::clicked, this, &ProviderSettingsDialog::onFetchModels);

    formLayout->addRow("Name:", m_nameEdit);
    formLayout->addRow("Type:", m_typeCombo);
    formLayout->addRow("Endpoint URL:", m_urlEdit);
    formLayout->addRow("API Key:", m_keyEdit);
    formLayout->addRow("Selected Model:", m_modelCombo);
    
    m_contextWindowSpin = new QSpinBox(formGroupBox);
    m_contextWindowSpin->setRange(1024, 2048000);
    m_contextWindowSpin->setSingleStep(1024);
    m_contextWindowSpin->setValue(32768);
    m_contextWindowSpin->setSuffix(" tokens");
    m_contextWindowSpin->setToolTip("The maximum context window size for this model. Old messages will be pruned when this limit is approached.");
    formLayout->addRow("Context Window:", m_contextWindowSpin);

    formLayout->addRow("", fetchBtn);

    rightLayout->addWidget(formGroupBox);

    m_statusLabel = new QLabel("", rightWidget);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(m_statusLabel);

    rightLayout->addStretch();

    auto* diagButtons = new QHBoxLayout();
    auto* testBtn = new QPushButton("Test Connection", rightWidget);
    auto* saveBtn = new QPushButton("Save Changes", rightWidget);
    saveBtn->setDefault(true);
    saveBtn->setStyleSheet("background-color: #3B82F6; color: white; border: none; font-weight: bold;");
    
    connect(testBtn, &QPushButton::clicked, this, &ProviderSettingsDialog::onTestConnection);
    connect(saveBtn, &QPushButton::clicked, this, &ProviderSettingsDialog::onSave);

    diagButtons->addStretch();
    diagButtons->addWidget(testBtn);
    diagButtons->addWidget(saveBtn);
    rightLayout->addLayout(diagButtons);

    splitter->addWidget(rightWidget);
    mainLayout->addWidget(splitter);
}

void ProviderSettingsDialog::onAddProvider() {
    LlmProvider p;
    p.id = QUuid::createUuid().toString();
    p.name = "New Provider";
    p.url = "http://localhost:11434";
    p.type = "ollama";
    m_workingList.append(p);
    updateList();
    m_providerList->setCurrentRow(m_workingList.size() - 1);
}

void ProviderSettingsDialog::onRemoveProvider() {
    if (m_currentIndex < 0 || m_currentIndex >= m_workingList.size()) return;
    m_workingList.removeAt(m_currentIndex);
    updateList();
}

void ProviderSettingsDialog::onProviderSelected(int index) {
    // Save current before switching
    if (m_currentIndex >= 0 && m_currentIndex < m_workingList.size()) {
        auto& p = m_workingList[m_currentIndex];
        p.name = m_nameEdit->text();
        p.type = m_typeCombo->currentText();
        p.url = m_urlEdit->text();
        p.apiKey = m_keyEdit->text();
        p.selectedModel = m_modelCombo->currentText();
        p.contextWindow = m_contextWindowSpin->value();
    }

    m_currentIndex = index;
    if (index >= 0 && index < m_workingList.size()) {
        loadProvider(index);
    }
}

void ProviderSettingsDialog::loadProvider(int index) {
    const auto& p = m_workingList[index];
    m_nameEdit->setText(p.name);
    m_typeCombo->setCurrentText(p.type);
    m_urlEdit->setText(p.url);
    m_keyEdit->setText(p.apiKey);
    
    m_modelCombo->clear();
    if (!p.selectedModel.isEmpty()) {
        m_modelCombo->addItem(p.selectedModel);
        m_modelCombo->setCurrentText(p.selectedModel);
    }
    m_contextWindowSpin->setValue(p.contextWindow);

    m_statusLabel->setText("");
}

/**
 * @brief Fetches models from the current provider's endpoint.
 * construction of the target URL depends on the provider type.
 */
void ProviderSettingsDialog::onFetchModels() {
    m_statusLabel->setText("<font color='#3B82F6'>Fetching models...</font>");
    m_discovery->fetchModels(m_urlEdit->text(), m_typeCombo->currentText(), m_keyEdit->text());
}

void ProviderSettingsDialog::onTestConnection() {
    onFetchModels(); // Test is essentially success of fetch
}

void ProviderSettingsDialog::onTypeChanged(const QString& type) {
    if (type == "ollama") m_urlEdit->setText("http://localhost:11434");
    else if (type == "lmstudio") m_urlEdit->setText("http://localhost:1234/v1");
    else if (type == "openai") m_urlEdit->setText("https://api.openai.com/v1");
}

void ProviderSettingsDialog::onSave() {
    onProviderSelected(m_currentIndex); // Ensure current edits are saved
    m_config->setProviders(m_workingList);
    accept();
}

void ProviderSettingsDialog::onModelsReady(const QStringList& models) {
    m_modelCombo->clear();
    m_modelCombo->addItems(models);
    m_statusLabel->setText("<font color='#10B981'><b>Success!</b> Connection established.</font>");
}

void ProviderSettingsDialog::onDiscoveryError(const QString& error) {
    m_statusLabel->setText(QString("<font color='#EF4444'><b>Error:</b> %1</font>").arg(error));
}

void ProviderSettingsDialog::updateList() {
    m_providerList->clear();
    for (const auto& p : m_workingList) {
        m_providerList->addItem(p.name);
    }
}

} // namespace CodeHex
