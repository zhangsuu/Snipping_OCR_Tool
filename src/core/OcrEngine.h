#ifndef OCRENGINE_H
#define OCRENGINE_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <mutex>

struct llama_model;
struct llama_context;
struct mtmd_context;
struct llama_vocab;

class OcrEngine {
public:
    using ProgressCallback = std::function<void(const std::string &message)>;

    OcrEngine();
    ~OcrEngine();

    OcrEngine(const OcrEngine &) = delete;
    OcrEngine &operator=(const OcrEngine &) = delete;

    // 在 GPU 显存中初始化与预加载模型
    bool loadModel(const std::string &modelPath,
                   const std::string &mmprojPath,
                   const ProgressCallback &progressCb = nullptr,
                   std::string *errorMsg = nullptr);

    // 从内存二进制图像 Buffer 执行 OCR 识别，支持自定义 Prompt 提示词
    std::string processImageBuffer(const uint8_t *bufferData,
                                   size_t bufferSize,
                                   const std::string &prompt = "",
                                   const ProgressCallback &progressCb = nullptr,
                                   std::string *errorMsg = nullptr);

    // 检查模型是否已加载至显存
    bool isLoaded() const;

    // 显式释放模型资源
    void freeModel();

private:
    mutable std::mutex m_mutex;
    llama_model *m_model = nullptr;
    llama_context *m_lctx = nullptr;
    mtmd_context *m_mctx = nullptr;
    const llama_vocab *m_vocab = nullptr;
    bool m_isLoaded = false;
};

#endif // OCRENGINE_H
