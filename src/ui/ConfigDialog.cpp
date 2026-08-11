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
    resize(600, 520);

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

    // 5. Pandoc 可执行文件路径
    QVBoxLayout *pandocLayout = new QVBoxLayout();
    QLabel *pandocLabel = new QLabel("Pandoc 可执行文件路径:", this);
    pandocLabel->setObjectName("fieldLabel");

    QHBoxLayout *pandocInputLayout = new QHBoxLayout();
    m_pandocPathEdit = new QLineEdit(this);
    m_pandocPathEdit->setPlaceholderText(QString::fromStdString(AppConfig::DEFAULT_PANDOC_PATH));
    m_browsePandocBtn = new QPushButton("📂 浏览...", this);
    m_browsePandocBtn->setObjectName("browseBtn");
    m_browsePandocBtn->setCursor(Qt::PointingHandCursor);

    pandocInputLayout->addWidget(m_pandocPathEdit);
    pandocInputLayout->addWidget(m_browsePandocBtn);
    pandocLayout->addWidget(pandocLabel);
    pandocLayout->addLayout(pandocInputLayout);

    mainLayout->addLayout(pandocLayout);

    // 6. Word 可执行文件路径
    QVBoxLayout *wordLayout = new QVBoxLayout();
    QLabel *wordLabel = new QLabel("Microsoft Word (WINWORD.EXE) 路径:", this);
    wordLabel->setObjectName("fieldLabel");

    QHBoxLayout *wordInputLayout = new QHBoxLayout();
    m_wordPathEdit = new QLineEdit(this);
    m_wordPathEdit->setPlaceholderText(QString::fromStdString(AppConfig::DEFAULT_WORD_PATH));
    m_browseWordBtn = new QPushButton("📂 浏览...", this);
    m_browseWordBtn->setObjectName("browseBtn");
    m_browseWordBtn->setCursor(Qt::PointingHandCursor);

    wordInputLayout->addWidget(m_wordPathEdit);
    wordInputLayout->addWidget(m_browseWordBtn);
    wordLayout->addWidget(wordLabel);
    wordLayout->addLayout(wordInputLayout);

    mainLayout->addLayout(wordLayout);

    // 7. 底部控制按鈕钮
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
    connect(m_browsePandocBtn, &QPushButton::clicked, this, &ConfigDialog::onBrowsePandoc);
    connect(m_browseWordBtn, &QPushButton::clicked, this, &ConfigDialog::onBrowseWord);
    connect(m_saveBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ConfigDialog::setPaths(const std::string &modelPath, const std::string &mmprojPath, const std::string &prompt, const std::string &shortcut,
                             const std::string &pandocPath, const std::string &wordPath) {
    m_modelPathEdit->setText(QString::fromStdString(modelPath));
    m_mmprojPathEdit->setText(QString::fromStdString(mmprojPath));
    m_promptEdit->setText(QString::fromStdString(prompt));
    m_shortcutEdit->setText(QString::fromStdString(shortcut));
    m_pandocPathEdit->setText(QString::fromStdString(pandocPath));
    m_wordPathEdit->setText(QString::fromStdString(wordPath));
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

std::string ConfigDialog::getPandocPath() const {
    return m_pandocPathEdit->text().trimmed().toStdString();
}

std::string ConfigDialog::getWordPath() const {
    return m_wordPathEdit->text().trimmed().toStdString();
}

void ConfigDialog::onBrowseModel() {
    QString file = QFileDialog::getOpenFileName(this, "选择主模型文件", m_modelPathEdit->text(), "GGUF 模型文件 (*.gguf);;所有文件 (*.*)");
    if (!file.isEmpty()) {
        m_modelPathEdit->setText(file);
    }
}

void ConfigDialog::onBrowseMmproj() {
    QString file = QFileDialog::getOpenFileName(this, "选择视觉 mmproj 模型文件", m_mmprojPathEdit->text(), "GGUF 模型文件 (*.gguf);;所有文件 (*.*)" );
    if (!file.isEmpty()) {
        m_mmprojPathEdit->setText(file);
    }
}

void ConfigDialog::onBrowsePandoc() {
    QString file = QFileDialog::getOpenFileName(this, "选择 Pandoc 可执行文件", m_pandocPathEdit->text(), "可执行文件 (*.exe);;所有文件 (*.*)" );
    if (!file.isEmpty()) {
        m_pandocPathEdit->setText(file);
    }
}

void ConfigDialog::onBrowseWord() {
    QString file = QFileDialog::getOpenFileName(this, "选择 Microsoft Word 可执行文件", m_wordPathEdit->text(), "可执行文件 (*.exe);;所有文件 (*.*)" );
    if (!file.isEmpty()) {
        m_wordPathEdit->setText(file);
    }
}
