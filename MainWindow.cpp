#include "MainWindow.h"
#include "SnipperWidget.h"
#include "OcrEngine.h"
#include "ConfigDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QHotkey>
#include <QKeySequence>
#include <QTimer>
#include <QFrame>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QBuffer>
#include <QByteArray>
#include <thread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_snipperWidget(new SnipperWidget(this))
    , m_ocrEngine(std::make_unique<OcrEngine>()) {
    
    AppConfig::loadOrCreateDefault();

    setupUi();
    updateShortcut();

    connect(m_snipperWidget, &SnipperWidget::imageCaptured, this, &MainWindow::onImageCaptured);
    connect(m_snipperWidget, &SnipperWidget::snippingCancelled, this, &MainWindow::onSnippingCancelled);

    connect(this, &MainWindow::modelLoadedSignal, this, &MainWindow::onModelLoaded);
    connect(this, &MainWindow::ocrStartedSignal, this, &MainWindow::onOcrStarted);
    connect(this, &MainWindow::ocrProgressSignal, this, &MainWindow::onOcrProgress);
    connect(this, &MainWindow::ocrFinishedSignal, this, &MainWindow::onOcrFinished);
    connect(this, &MainWindow::ocrFailedSignal, this, &MainWindow::onOcrFailed);

    // 程序启动时使用 std::thread 异步加载配置中的模型
    startLoadModel();



}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("Snipping OCR Tool (Mathpix Style)");
    resize(780, 520);
    setMinimumSize(560, 380);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setObjectName("headerFrame");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(20, 12, 20, 12);

    QLabel *titleLabel = new QLabel("📸 Snipping OCR Tool", this);
    titleLabel->setObjectName("titleLabel");
    QFont titleFont("Segoe UI", 16, QFont::Bold);
    titleLabel->setFont(titleFont);

    m_settingsButton = new QPushButton("⚙️ 设置", this);
    m_settingsButton->setObjectName("settingsButton");
    m_settingsButton->setCursor(Qt::PointingHandCursor);

    m_snipButton = new QPushButton("⌛ 预加载模型中...", this);
    m_snipButton->setObjectName("snipButton");
    m_snipButton->setEnabled(false);
    m_snipButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_settingsButton);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(m_snipButton);

    mainLayout->addWidget(headerFrame);

    // 中心区域：只展示识别结果文本面板 (单面板设计)
    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(18, 14, 18, 10);

    QHBoxLayout *contentHeader = new QHBoxLayout();
    QLabel *contentTitle = new QLabel("📝 OCR 识别文本", this);
    contentTitle->setObjectName("panelTitle");

    m_copyButton = new QPushButton("📋 复制文本", this);
    m_copyButton->setObjectName("copyButton");
    m_copyButton->setCursor(Qt::PointingHandCursor);
    m_copyButton->setEnabled(false);

    contentHeader->addWidget(contentTitle);
    contentHeader->addStretch();
    contentHeader->addWidget(m_copyButton);

    m_resultTextEdit = new QTextEdit(this);
    m_resultTextEdit->setObjectName("resultTextEdit");
    m_resultTextEdit->setPlaceholderText("使用快捷键或点击 「开始截屏」 按钮框选文字区域，识别结果将自动展示并复制到剪贴板...");

    QHBoxLayout *contentFooter = new QHBoxLayout();
    m_charCountBadge = new QLabel("0 字符", this);
    m_charCountBadge->setObjectName("charCountBadge");
    contentFooter->addStretch();
    contentFooter->addWidget(m_charCountBadge);

    contentLayout->addLayout(contentHeader);
    contentLayout->addWidget(m_resultTextEdit);
    contentLayout->addLayout(contentFooter);

    mainLayout->addLayout(contentLayout);

    // Status Bar
    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(18, 4, 18, 10);

    m_statusLabel = new QLabel("⏳ 正在读取 config.json 并启动预加载...", this);
    m_statusLabel->setObjectName("statusLabel");
    footerLayout->addWidget(m_statusLabel);

    mainLayout->addLayout(footerLayout);
    setCentralWidget(centralWidget);

    connect(m_snipButton, &QPushButton::clicked, this, &MainWindow::onStartSnippingClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(m_copyButton, &QPushButton::clicked, this, &MainWindow::onCopyTextClicked);
}

void MainWindow::updateShortcut() {
    QString keySeqStr = QString::fromStdString(AppConfig::getShortcut()).trimmed();
    if (keySeqStr.isEmpty()) {
        keySeqStr = "Ctrl+A";
    }

    if (!m_hotkey) {
        m_hotkey = new QHotkey(this);
        connect(m_hotkey, &QHotkey::activated, this, &MainWindow::onStartSnippingClicked);
    }

    m_hotkey->setShortcut(QKeySequence(keySeqStr), true);

    if (m_snipButton) {
        m_snipButton->setToolTip(QString("点击或按全局热键 %1 开始截屏").arg(keySeqStr));
    }

    if (!m_hotkey->isRegistered()) {
        if (m_statusLabel) {
            m_statusLabel->setText(QString("❌ 全局热键注册失败（可能被占用）: %1").arg(keySeqStr));
        }
    }
}

void MainWindow::startLoadModel() {
    m_snipButton->setEnabled(false);
    m_snipButton->setText("⌛ 预加载模型中...");
    m_statusLabel->setText("⏳ 正在从配置读取模型路径并加载至 GPU 显存...");

    std::thread([this]() {
        m_ocrEngine->freeModel();

        std::string errorMsg;
        bool ok = m_ocrEngine->loadModel(
            AppConfig::getModelPath(),
            AppConfig::getMmprojPath(),
            [this](const std::string &msg) {
                emit ocrProgressSignal(QString::fromStdString(msg));
            },
            &errorMsg
        );
        emit modelLoadedSignal(ok, ok ? QString("✅ OCR 模型已就绪 (GPU 加速) | 快捷键: %1").arg(QString::fromStdString(AppConfig::getShortcut())) : QString::fromStdString(errorMsg));
    }).detach();
}

void MainWindow::onSettingsClicked() {
    ConfigDialog dialog(this);
    dialog.setPaths(AppConfig::getModelPath(), AppConfig::getMmprojPath(), AppConfig::getPrompt(), AppConfig::getShortcut());

    if (dialog.exec() == QDialog::Accepted) {
        std::string newModel = dialog.getModelPath();
        std::string newMmproj = dialog.getMmprojPath();
        std::string newPrompt = dialog.getPrompt();
        std::string newShortcut = dialog.getShortcut();

        bool modelChanged = (newModel != AppConfig::getModelPath() || newMmproj != AppConfig::getMmprojPath());

        AppConfig::setModelPath(newModel);
        AppConfig::setMmprojPath(newMmproj);
        AppConfig::setPrompt(newPrompt);
        AppConfig::setShortcut(newShortcut);

        // 更新 QShortcut 绑定
        updateShortcut();

        if (AppConfig::saveToFile()) {
            m_statusLabel->setText("配置已成功保存至 config.json");
        } else {
            m_statusLabel->setText("⚠️ 配置保存失败");
        }

        if (modelChanged) {
            startLoadModel();
        }
    }
}

void MainWindow::onModelLoaded(bool success, const QString &statusMessage) {
    if (success) {
        m_snipButton->setEnabled(true);
        m_snipButton->setText("✂️  开始截屏");
        m_statusLabel->setText(statusMessage);
    } else {
        m_snipButton->setEnabled(false);
        m_snipButton->setText("❌ 模型加载失败");
        m_statusLabel->setText(statusMessage);
        m_resultTextEdit->setPlainText(QString("❌ 模型预加载失败:\n%1\n请检查「⚙️ 设置」中的模型路径。").arg(statusMessage));
    }
}

void MainWindow::onStartSnippingClicked() {
    m_statusLabel->setText("正在准备全屏截图...");
    hide();

    QTimer::singleShot(200, this, [this]() {
        m_snipperWidget->startSnipping();
    });
}

void MainWindow::onImageCaptured(const QPixmap &pixmap) {
    showNormal();
    activateWindow();
    raise();

    m_statusLabel->setText("截图成功，正在提交至 GPU 显存执行极速 OCR 识别...");

    QByteArray imageBytes;
    QBuffer buffer(&imageBytes);
    buffer.open(QIODevice::WriteOnly);
    if (!pixmap.save(&buffer, "PNG")) {
        onOcrFailed("无法将截图转换为内存 PNG 数据。");
        return;
    }

    std::string promptStr = AppConfig::getPrompt();

    std::thread([this, imageBytes, promptStr]() {
        emit ocrStartedSignal();

        std::string errorMsg;
        std::string text = m_ocrEngine->processImageBuffer(
            reinterpret_cast<const uint8_t*>(imageBytes.constData()),
            static_cast<size_t>(imageBytes.size()),
            promptStr,
            [this](const std::string &msg) {
                emit ocrProgressSignal(QString::fromStdString(msg));
            },
            &errorMsg
        );

        if (errorMsg.empty()) {
            emit ocrFinishedSignal(QString::fromStdString(text));
        } else {
            emit ocrFailedSignal(QString::fromStdString(errorMsg));
        }
    }).detach();
}

void MainWindow::onSnippingCancelled() {
    showNormal();
    activateWindow();
    raise();
    m_statusLabel->setText("截屏已取消");
}

void MainWindow::onOcrStarted() {
    m_snipButton->setEnabled(false);
    m_snipButton->setText("⌛ 识别中...");
    m_copyButton->setEnabled(false);
    m_resultTextEdit->setPlainText("🔍 正在推理识别文本，请稍候...");
    m_charCountBadge->setText("识别中...");
}

void MainWindow::onOcrProgress(const QString &statusMessage) {
    m_statusLabel->setText(statusMessage);
}

void MainWindow::onOcrFinished(const QString &resultText) {
    m_snipButton->setEnabled(true);
    m_snipButton->setText("✂️  开始截屏");

    QString trimmed = resultText.trimmed();
    m_resultTextEdit->setPlainText(trimmed);

    if (!trimmed.isEmpty()) {
        // 自动复制到系统剪贴板
        QGuiApplication::clipboard()->setText(trimmed);
        m_copyButton->setEnabled(true);
        m_charCountBadge->setText(QString("%1 字符").arg(trimmed.length()));
        m_statusLabel->setText("✅ OCR 识别完成！文本已自动复制到剪贴板");
    } else {
        m_copyButton->setEnabled(false);
        m_charCountBadge->setText("0 字符");
        m_statusLabel->setText("✅ OCR 识别完成！未检测到文字内容");
    }
}

void MainWindow::onOcrFailed(const QString &errorMessage) {
    m_snipButton->setEnabled(true);
    m_snipButton->setText("✂️  开始截屏");
    m_copyButton->setEnabled(false);

    m_resultTextEdit->setPlainText(QString("❌ OCR 识别失败:\n%1").arg(errorMessage));
    m_charCountBadge->setText("识别失败");
    m_statusLabel->setText(QString("❌ OCR 失败: %1").arg(errorMessage));
}

void MainWindow::onCopyTextClicked() {
    QString text = m_resultTextEdit->toPlainText();
    if (!text.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
        m_copyButton->setText("✅ 已复制!");

        QTimer::singleShot(1500, this, [this]() {
            m_copyButton->setText("📋 复制文本");
        });
    }
}
