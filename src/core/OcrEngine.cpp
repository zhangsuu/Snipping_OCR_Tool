#include "OcrEngine.h"
#include "AppConfig.h"
#include "llama.h"
#include "mtmd.h"
#include "mtmd-helper.h"

#include <iostream>

OcrEngine::OcrEngine() = default;

OcrEngine::~OcrEngine() {
    freeModel();
}

bool OcrEngine::isLoaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isLoaded;
}

bool OcrEngine::loadModel(const std::string &modelPath,
                          const std::string &mmprojPath,
                          const ProgressCallback &progressCb,
                          std::string *errorMsg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isLoaded) return true;

    if (progressCb) progressCb("正在初始化 GPU 后端并加载模型至显存...");

    // 1. 初始化 C 后端
    llama_backend_init();
    ggml_backend_load_all();
    mtmd_helper_log_set(nullptr, nullptr);

    // 2. 加载 llama 主模型至显存
    llama_model_params model_params = llama_model_default_params();
    m_model = llama_model_load_from_file(modelPath.c_str(), model_params);
    if (!m_model) {
        if (errorMsg) *errorMsg = "模型加载失败: " + modelPath;
        llama_backend_free();
        return false;
    }

    m_vocab = llama_model_get_vocab(m_model);

    // 3. 创建常驻上下文
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx   = 4096;
    ctx_params.n_batch = 512;
    m_lctx = llama_init_from_model(m_model, ctx_params);
    if (!m_lctx) {
        if (errorMsg) *errorMsg = "创建 llama 上下文失败";
        llama_model_free(m_model);
        m_model = nullptr;
        llama_backend_free();
        return false;
    }

    // 4. 初始化视觉 mtmd 模块至显存
    mtmd_context_params mparams = mtmd_context_params_default();
    mparams.use_gpu       = true;
    mparams.print_timings = true;
    mparams.n_threads     = 4;
    m_mctx = mtmd_init_from_file(mmprojPath.c_str(), m_model, mparams);
    if (!m_mctx) {
        if (errorMsg) *errorMsg = "初始化视觉 mtmd 模块失败: " + mmprojPath;
        llama_free(m_lctx);
        m_lctx = nullptr;
        llama_model_free(m_model);
        m_model = nullptr;
        llama_backend_free();
        return false;
    }

    m_isLoaded = true;
    if (progressCb) progressCb("✅ OCR 模型已准备就绪 (GPU 加速)");
    return true;
}

std::string OcrEngine::processImageBuffer(const uint8_t *bufferData,
                                          size_t bufferSize,
                                          const std::string &prompt,
                                          const ProgressCallback &progressCb,
                                          std::string *errorMsg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isLoaded || !m_model || !m_lctx || !m_mctx) {
        if (errorMsg) *errorMsg = "OCR 模型尚未加载至显存";
        return "";
    }

    if (!bufferData || bufferSize == 0) {
        if (errorMsg) *errorMsg = "无效的图像内存数据";
        return "";
    }

    if (progressCb) progressCb("正在清理 KV 缓存并解码图像...");

    // 0. 清理上一次推理在 llama_context 中留存的 KV 缓存与序列状态
    llama_memory_t mem = llama_get_memory(m_lctx);
    if (mem) {
        llama_memory_clear(mem, true);
        llama_memory_seq_rm(mem, -1, -1, -1);
    }

    // 1. 从内存 Buffer 构建 mtmd_bitmap
    auto wrapper = mtmd_helper_bitmap_init_from_buf(m_mctx, bufferData, bufferSize, false);
    if (!wrapper.bitmap) {
        if (errorMsg) *errorMsg = "从内存缓冲区解码图片位图失败";
        return "";
    }

    // 2. 构建 Prompt 并 Tokenize
    std::string actualPrompt = prompt.empty() ? AppConfig::DEFAULT_PROMPT : prompt;
    std::string fullPrompt = std::string(mtmd_default_marker()) + actualPrompt;
    mtmd_input_chunks *chunks = mtmd_input_chunks_init();
    mtmd_input_text text_in = { fullPrompt.c_str(), fullPrompt.size(), true, false };
    const mtmd_bitmap *bitmaps[] = { wrapper.bitmap };

    int32_t ret = mtmd_tokenize(m_mctx, chunks, &text_in, bitmaps, 1);
    if (ret != 0) {
        if (errorMsg) *errorMsg = "mtmd_tokenize 失败, 错误码: " + std::to_string(ret);
        mtmd_bitmap_free(wrapper.bitmap);
        mtmd_input_chunks_free(chunks);
        return "";
    }

    // 3. 直接使用常驻上下文 Eval 图像与文本 Chunk
    if (progressCb) progressCb("正在进行 GPU 图像特征推理...");
    llama_pos n_past = 0;
    ret = mtmd_helper_eval_chunks(m_mctx, m_lctx, chunks, n_past, 0, 512, true, &n_past);
    if (ret != 0) {
        if (errorMsg) *errorMsg = "eval_chunks 评估失败, 错误码: " + std::to_string(ret);
        mtmd_bitmap_free(wrapper.bitmap);
        mtmd_input_chunks_free(chunks);
        return "";
    }

    // 4. 采样器与 Token 文本生成
    if (progressCb) progressCb("正在生成文本结果...");
    llama_sampler *smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

    int max_tokens = 1024;
    llama_token eos = llama_vocab_eos(m_vocab);
    std::vector<llama_token> generated;

    for (int i = 0; i < max_tokens; i++) {
        llama_token new_token = llama_sampler_sample(smpl, m_lctx, -1);
        if (new_token == eos) break;
        generated.push_back(new_token);

        llama_batch batch = llama_batch_get_one(&new_token, 1);
        if (llama_decode(m_lctx, batch) != 0) break;
    }

    // Detokenize 前丢弃最后一个 token
    if (!generated.empty()) {
        generated.pop_back();
    }

    // 5. Detokenize 得到识别文本
    std::string output(16384, '\0');
    int32_t n_out = llama_detokenize(m_vocab,
                                     generated.data(), static_cast<int32_t>(generated.size()),
                                     output.data(), static_cast<int32_t>(output.size()),
                                     false, true);
    if (n_out > 0) {
        output.resize(n_out);
    } else {
        output.clear();
    }

    // 6. 清理单次推理的临时结构
    llama_sampler_free(smpl);
    mtmd_bitmap_free(wrapper.bitmap);
    mtmd_input_chunks_free(chunks);

    return output;
}

void OcrEngine::freeModel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_mctx) {
        mtmd_free(m_mctx);
        m_mctx = nullptr;
    }
    if (m_lctx) {
        llama_free(m_lctx);
        m_lctx = nullptr;
    }
    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
    }
    m_isLoaded = false;
    llama_backend_free();
}
