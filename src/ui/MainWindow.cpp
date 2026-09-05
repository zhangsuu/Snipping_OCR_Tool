#include "MainWindow.h"
#include "SnipperWidget.h"
#include "OcrEngine.h"
#include "ConfigDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QHotkey>
#include <QKeySequence>
#include <QTimer>
#include <QFrame>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QBuffer>
#include <QByteArray>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QRegularExpression>
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

    m_removeSpacesButton = new QPushButton("🧹 去除中英空格", this);
    m_removeSpacesButton->setObjectName("removeSpacesButton");
    m_removeSpacesButton->setToolTip("删除中文字符与英文、数字之间的空格");
    m_removeSpacesButton->setCursor(Qt::PointingHandCursor);
    m_removeSpacesButton->setEnabled(false);

    m_exportDocxButton = new QPushButton("📄 导出 Docx", this);
    m_exportDocxButton->setObjectName("exportDocxButton");
    m_exportDocxButton->setCursor(Qt::PointingHandCursor);
    // m_exportDocxButton->setEnabled(false);

    contentHeader->addWidget(contentTitle);
    contentHeader->addStretch();
    contentHeader->addWidget(m_removeSpacesButton);
    contentHeader->addSpacing(6);
    contentHeader->addWidget(m_exportDocxButton);
    contentHeader->addSpacing(6);
    contentHeader->addWidget(m_copyButton);

    m_resultTextEdit = new QPlainTextEdit(this);
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
    connect(m_removeSpacesButton, &QPushButton::clicked, this, &MainWindow::onRemoveSpacesClicked);
    connect(m_copyButton, &QPushButton::clicked, this, &MainWindow::onCopyTextClicked);
    connect(m_exportDocxButton, &QPushButton::clicked, this, &MainWindow::onExportDocxClicked);
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
    dialog.setPaths(AppConfig::getModelPath(), AppConfig::getMmprojPath(), AppConfig::getPrompt(), AppConfig::getShortcut(),
                    AppConfig::getPandocPath(), AppConfig::getWordPath());

    if (dialog.exec() == QDialog::Accepted) {
        std::string newModel = dialog.getModelPath();
        std::string newMmproj = dialog.getMmprojPath();
        std::string newPrompt = dialog.getPrompt();
        std::string newShortcut = dialog.getShortcut();
        std::string newPandoc = dialog.getPandocPath();
        std::string newWord = dialog.getWordPath();

        bool modelChanged = (newModel != AppConfig::getModelPath() || newMmproj != AppConfig::getMmprojPath());

        AppConfig::setModelPath(newModel);
        AppConfig::setMmprojPath(newMmproj);
        AppConfig::setPrompt(newPrompt);
        AppConfig::setShortcut(newShortcut);
        AppConfig::setPandocPath(newPandoc);
        AppConfig::setWordPath(newWord);

        // 更新快捷键绑定
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
    m_exportDocxButton->setEnabled(false);
    m_removeSpacesButton->setEnabled(false);
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
        m_exportDocxButton->setEnabled(true);
        m_removeSpacesButton->setEnabled(true);
        m_charCountBadge->setText(QString("%1 字符").arg(trimmed.length()));
        m_statusLabel->setText("✅ OCR 识别完成！文本已自动复制到剪贴板");
    } else {
        m_copyButton->setEnabled(false);
        m_exportDocxButton->setEnabled(false);
        m_removeSpacesButton->setEnabled(false);
        m_charCountBadge->setText("0 字符");
        m_statusLabel->setText("✅ OCR 识别完成！未检测到文字内容");
    }
}

void MainWindow::onOcrFailed(const QString &errorMessage) {
    m_snipButton->setEnabled(true);
    m_snipButton->setText("✂️  开始截屏");
    m_copyButton->setEnabled(false);
    m_exportDocxButton->setEnabled(false);
    m_removeSpacesButton->setEnabled(false);

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

void MainWindow::onExportDocxClicked() {
    QString text = m_resultTextEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        QMessageBox::warning(this, "导出失败", "文本内容为空，无法导出。");
        return;
    }

    // 1. 将文本写入临时 Markdown 文件
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString mdPath = tempDir + "/snipping_ocr_" + timestamp + ".md";
    QString docxPath = tempDir + "/snipping_ocr_" + timestamp + ".docx";

    QFile mdFile(mdPath);
    if (!mdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法创建临时 Markdown 文件：\n" + mdPath);
        return;
    }
    QTextStream out(&mdFile);
    out.setEncoding(QStringConverter::Utf8);
    out << text;
    mdFile.close();

    // 2. 调用 pandoc 将 Markdown 转换为 Docx
    m_statusLabel->setText("⏳ 正在调用 pandoc 生成 Docx...");
    m_exportDocxButton->setEnabled(false);

    QProcess *pandocProcess = new QProcess(this);
    pandocProcess->setProgram(QString::fromStdString(AppConfig::getPandocPath()));
    pandocProcess->setArguments({mdPath, "-o", docxPath});

    connect(pandocProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, pandocProcess, mdPath, docxPath](int exitCode, QProcess::ExitStatus) {
        pandocProcess->deleteLater();
        QFile::remove(mdPath);
        m_exportDocxButton->setEnabled(true);

        if (exitCode != 0) {
            QString errMsg = pandocProcess->readAllStandardError();
            m_statusLabel->setText("❌ pandoc 转换失败");
            QMessageBox::critical(this, "导出失败",
                "pandoc 转换失败（请确认已安装 pandoc 并添加到 PATH 环境变量）。\n\n错误信息：\n" + errMsg);
            return;
        }

        m_statusLabel->setText("✅ Docx 文件已生成，正在用 Word 打开...");

        // 3. 使用配置的 WINWORD.EXE 打开生成的 Docx 文件
        QString wordExe = QString::fromStdString(AppConfig::getWordPath());
        // Word 需要 /f 参数，且路径中使用反斜杠
        QString docxNative = QDir::toNativeSeparators(docxPath);
        if (QFileInfo::exists(wordExe)) {
            QProcess::startDetached(wordExe, {"/f", docxNative});
        } else {
            m_statusLabel->setText(QString("❌ 未找到 Word 可执行文件，请在设置中配置正确路径：%1").arg(wordExe));
        }
    });

    pandocProcess->start();
    if (!pandocProcess->waitForStarted(3000)) {
        pandocProcess->deleteLater();
        QFile::remove(mdPath);
        m_exportDocxButton->setEnabled(true);
        m_statusLabel->setText("❌ 启动 pandoc 失败");
        QMessageBox::critical(this, "导出失败",
            "无法启动 pandoc。\n请确认已安装 pandoc：https://pandoc.org/installing.html\n并确保 pandoc 已添加到系统 PATH 环境变量。");
    }
}

void MainWindow::onRemoveSpacesClicked() {
    QString text = m_resultTextEdit->toPlainText();
    if (text.isEmpty()) {
        return;
    }

    // 1. 删除中文字符与英文字符/数字之间的空格 (中文 + 空格 + 英文/数字)
    static const QRegularExpression regex1(QStringLiteral("([\\p{Han}])[\\h]+([a-zA-Z0-9])"));
    // 2. 删除英文字符/数字与中文字符之间的空格 (英文/数字 + 空格 + 中文)
    static const QRegularExpression regex2(QStringLiteral("([a-zA-Z0-9])[\\h]+([\\p{Han}])"));

    QString processed = text;
    processed.replace(regex1, QStringLiteral("\\1\\2"));
    processed.replace(regex2, QStringLiteral("\\1\\2"));

    if (processed != text) {
        m_resultTextEdit->setPlainText(processed);
        m_charCountBadge->setText(QString("%1 字符").arg(processed.length()));
        m_statusLabel->setText("✅ 已去除中文字符与英文、数字之间的空格");
    } else {
        m_statusLabel->setText("ℹ️ 未发现需要去除的中英/数字空格");
    }
}
