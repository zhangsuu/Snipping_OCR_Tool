#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <string>

class QLineEdit;
class QPushButton;

class ConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog() override = default;

    void setPaths(const std::string &modelPath, const std::string &mmprojPath, const std::string &prompt, const std::string &shortcut,
                  const std::string &pandocPath, const std::string &wordPath);
    std::string getModelPath() const;
    std::string getMmprojPath() const;
    std::string getPrompt() const;
    std::string getShortcut() const;
    std::string getPandocPath() const;
    std::string getWordPath() const;

private slots:
    void onBrowseModel();
    void onBrowseMmproj();
    void onBrowsePandoc();
    void onBrowseWord();

private:
    void setupUi();

    QLineEdit *m_modelPathEdit = nullptr;
    QLineEdit *m_mmprojPathEdit = nullptr;
    QLineEdit *m_promptEdit = nullptr;
    QLineEdit *m_shortcutEdit = nullptr;
    QLineEdit *m_pandocPathEdit = nullptr;
    QLineEdit *m_wordPathEdit = nullptr;
    QPushButton *m_browseModelBtn = nullptr;
    QPushButton *m_browseMmprojBtn = nullptr;
    QPushButton *m_browsePandocBtn = nullptr;
    QPushButton *m_browseWordBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
};

#endif // CONFIGDIALOG_H
