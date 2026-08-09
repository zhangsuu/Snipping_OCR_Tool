#include "AppConfig.h"
#include <fstream>
#include <iostream>
#include <QCoreApplication>

namespace AppConfig {

const std::string DEFAULT_MODEL_PATH = "D:/LearnWorkSpace/model/enginil_dots.mocr-IQ4_NL-GGUF/dots.mocr-iq4_nl-imat.gguf";
const std::string DEFAULT_MMPROJ_PATH = "D:/LearnWorkSpace/model/enginil_dots.mocr-IQ4_NL-GGUF/mmproj-dotsmocr-q8_0.gguf";
const std::string DEFAULT_PROMPT = "Transcribe all text from this image.";
const std::string DEFAULT_SHORTCUT = "Alt+A";
const std::string DEFAULT_CONFIG_FILENAME = "config.json";
const std::string DEFAULT_PANDOC_PATH = "C:/Program Files/Pandoc/pandoc.exe";
const std::string DEFAULT_WORD_PATH = "C:/Program Files/Microsoft Office/root/Office16/WINWORD.EXE";

namespace {
    std::string g_modelPath = DEFAULT_MODEL_PATH;
    std::string g_mmprojPath = DEFAULT_MMPROJ_PATH;
    std::string g_prompt = DEFAULT_PROMPT;
    std::string g_shortcut = DEFAULT_SHORTCUT;
    std::string g_pandocPath = DEFAULT_PANDOC_PATH;
    std::string g_wordPath = DEFAULT_WORD_PATH;
    std::string g_configFilePath;
}

void setConfigFilePath(const std::string &filePath) {
    g_configFilePath = filePath;
}

std::string getConfigFilePath() {
    if (!g_configFilePath.empty()) {
        return g_configFilePath;
    }
    if (QCoreApplication::instance()) {
        return (QCoreApplication::applicationDirPath() + "/" + QString::fromStdString(DEFAULT_CONFIG_FILENAME)).toStdString();
    }
    return DEFAULT_CONFIG_FILENAME;
}

void setModelPath(const std::string &path) {
    g_modelPath = path;
}

void setMmprojPath(const std::string &path) {
    g_mmprojPath = path;
}

void setPrompt(const std::string &p) {
    g_prompt = p;
}

void setShortcut(const std::string &s) {
    g_shortcut = s;
}

void setPandocPath(const std::string &path) {
    g_pandocPath = path;
}

void setWordPath(const std::string &path) {
    g_wordPath = path;
}

std::string getModelPath() {
    return g_modelPath.empty() ? DEFAULT_MODEL_PATH : g_modelPath;
}

std::string getMmprojPath() {
    return g_mmprojPath.empty() ? DEFAULT_MMPROJ_PATH : g_mmprojPath;
}

std::string getPrompt() {
    return g_prompt.empty() ? DEFAULT_PROMPT : g_prompt;
}

std::string getShortcut() {
    return g_shortcut.empty() ? DEFAULT_SHORTCUT : g_shortcut;
}

std::string getPandocPath() {
    return g_pandocPath.empty() ? DEFAULT_PANDOC_PATH : g_pandocPath;
}

std::string getWordPath() {
    return g_wordPath.empty() ? DEFAULT_WORD_PATH : g_wordPath;
}

nlohmann::json toJson() {
    return nlohmann::json{
        {"model_path", g_modelPath},
        {"mmproj_path", g_mmprojPath},
        {"prompt", g_prompt},
        {"shortcut", g_shortcut},
        {"pandoc_path", g_pandocPath},
        {"word_path", g_wordPath}
    };
}

void fromJson(const nlohmann::json &j) {
    if (j.contains("model_path") && j["model_path"].is_string()) {
        g_modelPath = j["model_path"].get<std::string>();
    }
    if (j.contains("mmproj_path") && j["mmproj_path"].is_string()) {
        g_mmprojPath = j["mmproj_path"].get<std::string>();
    }
    if (j.contains("prompt") && j["prompt"].is_string()) {
        g_prompt = j["prompt"].get<std::string>();
    }
    if (j.contains("shortcut") && j["shortcut"].is_string()) {
        g_shortcut = j["shortcut"].get<std::string>();
    }
    if (j.contains("pandoc_path") && j["pandoc_path"].is_string()) {
        g_pandocPath = j["pandoc_path"].get<std::string>();
    }
    if (j.contains("word_path") && j["word_path"].is_string()) {
        g_wordPath = j["word_path"].get<std::string>();
    }
}

bool loadFromFile(const std::string &filePath) {
    std::string targetPath = filePath.empty() ? getConfigFilePath() : filePath;
    std::ifstream inFile(targetPath);
    if (!inFile.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        inFile >> j;
        fromJson(j);
        return true;
    } catch (const std::exception &e) {
        std::cerr << "JSON 解析错误: " << e.what() << std::endl;
        return false;
    }
}

bool saveToFile(const std::string &filePath) {
    std::string targetPath = filePath.empty() ? getConfigFilePath() : filePath;
    std::ofstream outFile(targetPath);
    if (!outFile.is_open()) {
        return false;
    }

    try {
        nlohmann::json j = toJson();
        outFile << j.dump(4);
        return true;
    } catch (const std::exception &e) {
        std::cerr << "JSON 保存错误: " << e.what() << std::endl;
        return false;
    }
}

bool load(const std::string &filePath) {
    return loadFromFile(filePath);
}

bool save(const std::string &filePath) {
    return saveToFile(filePath);
}

void loadOrCreateDefault(const std::string &filePath) {
    std::string targetPath = filePath.empty() ? getConfigFilePath() : filePath;
    if (!loadFromFile(targetPath)) {
        saveToFile(targetPath);
    }
}

} // namespace AppConfig
