#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <string>
#include <nlohmann/json.hpp>

namespace AppConfig {
    extern const std::string DEFAULT_MODEL_PATH;
    extern const std::string DEFAULT_MMPROJ_PATH;
    extern const std::string DEFAULT_PROMPT;
    extern const std::string DEFAULT_SHORTCUT;
    extern const std::string DEFAULT_CONFIG_FILENAME;

    // 配置文件路径设置与获取
    void setConfigFilePath(const std::string &filePath);
    std::string getConfigFilePath();

    // 设置各个配置项
    void setModelPath(const std::string &path);
    void setMmprojPath(const std::string &path);
    void setPrompt(const std::string &p);
    void setShortcut(const std::string &s);

    // 获取各个配置项（非空时返回设置值，若为空则返回对应的默认值）
    std::string getModelPath();
    std::string getMmprojPath();
    std::string getPrompt();
    std::string getShortcut();

    // 加载与保存配置（若未指定 path 则默认使用全局配置路径）
    bool load(const std::string &filePath = "");
    bool save(const std::string &filePath = "");

    bool loadFromFile(const std::string &filePath = "");
    bool saveToFile(const std::string &filePath = "");

    void loadOrCreateDefault(const std::string &filePath = "");

    // 序列化与反序列化支持
    nlohmann::json toJson();
    void fromJson(const nlohmann::json &j);
}

#endif // APPCONFIG_H
