#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <memory>
#include <QPlainTextEdit>

#include "AppConfig.h"

class QLabel;
class QPushButton;
class QTextEdit;
class QHotkey;
class SnipperWidget;
class OcrEngine;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void ocrProgressSignal(const QString &statusMessage);
    void modelLoadedSignal(bool success, const QString &statusMessage);
    void ocrStartedSignal();
    void ocrFinishedSignal(const QString &resultText);
    void ocrFailedSignal(const QString &errorMessage);

private slots:
    void onStartSnippingClicked();
    void onSettingsClicked();
    void onImageCaptured(const QPixmap &pixmap);
    void onSnippingCancelled();

    void onModelLoaded(bool success, const QString &statusMessage);
    void onOcrStarted();
    void onOcrProgress(const QString &statusMessage);
    void onOcrFinished(const QString &resultText);
    void onOcrFailed(const QString &errorMessage);
    void onCopyTextClicked();
    void onExportDocxClicked();
    void onRemoveSpacesClicked();

private:
    void setupUi();
    void startLoadModel();
    void updateShortcut();

    QPushButton *m_snipButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_exportDocxButton = nullptr;
    QPushButton *m_removeSpacesButton = nullptr;
    QPlainTextEdit *m_resultTextEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_charCountBadge = nullptr;

    QHotkey *m_hotkey = nullptr;
    SnipperWidget *m_snipperWidget = nullptr;
    std::unique_ptr<OcrEngine> m_ocrEngine;
};

#endif // MAINWINDOW_H
