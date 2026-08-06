#include "ConfigDialog.h"
#include "AppConfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QFrame>

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi();
}

void ConfigDialog::setupUi() {
    setWindowTitle("⚙️ OCR 模型与系统设置");
    resize(580, 380);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    QLabel *title = new QLabel("⚙️ OCR 模型与系统设置", this);
    title->setObjectName("dialogTitle");
    mainLayout->addWidget(title);

    // 1. 主模型路径
    QVBoxLayout *modelLayout = new QVBoxLayout();
    QLabel *modelLabel = new QLabel("主模型路径 (.gguf):", this);
    modelLabel->setObjectName("fieldLabel");

    QHBoxLayout *modelInputLayout = new QHBoxLayout();
    m_modelPathEdit = new QLineEdit(this);
    m_modelPathEdit->setPlaceholderText(QString::fromStdString(AppConfig::DEFAULT_MODEL_PATH));
    m_browseModelBtn = new QPushButton("📂 浏览...", this);
    m_browseModelBtn->setObjectName("browseBtn");
    m_browseModelBtn->setCursor(Qt::PointingHandCursor);

    modelInputLayout->addWidget(m_modelPathEdit);
    modelInputLayout->addWidget(m_browseModelBtn);
    modelLayout->addWidget(modelLabel);
    modelLayout->addLayout(modelInputLayout);

    mainLayout->addLayout(modelLayout);

    // 2. 视觉模型路径
    QVBoxLayout *mmprojLayout = new QVBoxLayout();
    QLabel *mmprojLabel = new QLabel("视觉投影模型路径 (mmproj .gguf):", this);
    mmprojLabel->setObjectName("fieldLabel");

    QHBoxLayout *mmprojInputLayout = new QHBoxLayout();
    m_mmprojPathEdit = new QLineEdit(this);
    m_mmprojPathEdit->setPlaceholderText(QString::fromStdString(AppConfig::DEFAULT_MMPROJ_PATH));
    m_browseMmprojBtn = new QPushButton("📂 浏览...", this);
    m_browseMmprojBtn->setObjectName("browseBtn");
    m_browseMmprojBtn->setCursor(Qt::PointingHandCursor);

    mmprojInputLayout->addWidget(m_mmprojPathEdit);
    mmprojInputLayout->addWidget(m_browseMmprojBtn);
    mmprojLayout->addWidget(mmprojLabel);
    mmprojLayout->addLayout(mmprojInputLayout);

    mainLayout->addLayout(mmprojLayout);

    // 3. Prompt 提示词设置
    QVBoxLayout *promptLayout = new QVBoxLayout();
    QLabel *promptLabel = new QLabel("Prompt 提示词:", this);
    promptLabel->setObjectName("fieldLabel");

    m_promptEdit = new QLineEdit(this);
    m_promptEdit->setPlaceholderText(QString::fromStdString(AppConfig::DEFAULT_PROMPT));

    promptLayout->addWidget(promptLabel);
    promptLayout->addWidget(m_promptEdit);

    mainLayout->addLayout(promptLayout);

    // 4. 截屏快捷键设置
    QVBoxLayout *shortcutLayout = new QVBoxLayout();
    QLabel *shortcutLabel = new QLabel("截屏快捷键 (如 Ctrl+A, Alt+S, F4):", this);
    shortcutLabel->setObjectName("fieldLabel");

    m_shortcutEdit = new QLineEdit(this);
    m_shortcutEdit->setPlaceholderText(QString::fromStdString(AppConfig::DEFAULT_SHORTCUT));

    shortcutLayout->addWidget(shortcutLabel);
    shortcutLayout->addWidget(m_shortcutEdit);

    mainLayout->addLayout(shortcutLayout);

    // 5. 底部控制按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("取消", this);
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setCursor(Qt::PointingHandCursor);

    m_saveBtn = new QPushButton("💾 保存配置", this);
    m_saveBtn->setObjectName("saveBtn");
    m_saveBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_browseModelBtn, &QPushButton::clicked, this, &ConfigDialog::onBrowseModel);
    connect(m_browseMmprojBtn, &QPushButton::clicked, this, &ConfigDialog::onBrowseMmproj);
    connect(m_saveBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ConfigDialog::setPaths(const std::string &modelPath, const std::string &mmprojPath, const std::string &prompt, const std::string &shortcut) {
    m_modelPathEdit->setText(QString::fromStdString(modelPath));
    m_mmprojPathEdit->setText(QString::fromStdString(mmprojPath));
    m_promptEdit->setText(QString::fromStdString(prompt));
    m_shortcutEdit->setText(QString::fromStdString(shortcut));
}

std::string ConfigDialog::getModelPath() const {
    return m_modelPathEdit->text().trimmed().toStdString();
}

std::string ConfigDialog::getMmprojPath() const {
    return m_mmprojPathEdit->text().trimmed().toStdString();
}

std::string ConfigDialog::getPrompt() const {
    return m_promptEdit->text().trimmed().toStdString();
}

std::string ConfigDialog::getShortcut() const {
    return m_shortcutEdit->text().trimmed().toStdString();
}

void ConfigDialog::onBrowseModel() {
    QString file = QFileDialog::getOpenFileName(this, "选择主模型文件", m_modelPathEdit->text(), "GGUF 模型文件 (*.gguf);;所有文件 (*.*)");
    if (!file.isEmpty()) {
        m_modelPathEdit->setText(file);
    }
}

void ConfigDialog::onBrowseMmproj() {
    QString file = QFileDialog::getOpenFileName(this, "选择视觉 mmproj 模型文件", m_mmprojPathEdit->text(), "GGUF 模型文件 (*.gguf);;所有文件 (*.*)");
    if (!file.isEmpty()) {
        m_mmprojPathEdit->setText(file);
    }
}
