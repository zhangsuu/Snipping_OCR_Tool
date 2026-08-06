# 📸 Snipping OCR Tool

基于 Qt6 与 llama.cpp (GPU 加速) 的极速跨平台截屏与文字/公式识别工具 (Mathpix 风格)。支持全局系统热键唤起、全屏框选截屏以及自动复制识别结果至剪贴板。

---

## 🛠️ 环境要求

- **C++ 编译器**: 支持 C++20 标准 (如 MSVC 2022 / GCC 10+ / Clang 12+)
- **构建工具**: CMake 3.20 或更高版本
- **Qt 框架**: Qt 6.x (包含 `Core`, `Gui`, `Widgets` 模块)
- **llama.cpp**: 已编译出 `llama.lib` / `mtmd.lib` / `ggml.lib` 的源码及构建目录

---

## 🚀 编译与构建步骤

本项目使用 CMake 进行构建，需在执行 `cmake` 时通过 `-D` 参数显式传入：

### 参数说明

| CMake 参数名 | 必填 | 示例值 | 说明 |
| :--- | :---: | :--- | :--- |
| `-DQt6_DIR` | 是 | `C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6` | Qt6 CMake 配置文件目录 |
| `-DLLAMACPP_DIR` | 是 | `D:/LearnWorkSpace/project/llama.cpp` | llama.cpp 源码根目录 |
| `-DLLAMACPP_BUILD` | 否 | `D:/LearnWorkSpace/project/llama.cpp/build` | llama.cpp 构建目录 (默认自动设为 `${LLAMACPP_DIR}/build`) |

---

### 命令行编译示例 (Windows MSVC / PowerShell)

```bash
# 1. 进入项目根目录并创建构建文件夹
cd Snipping_OCR_Tool
mkdir cmake-build-release
cd cmake-build-release

# 2. 运行 CMake 配置 (请将路径替换为您本地的真实路径)
cmake .. -DQt6_DIR="C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6" -DLLAMACPP_DIR="D:/LearnWorkSpace/project/llama.cpp"

# 3. 执行编译 (发布版 Release)
cmake --build . --config Release
```

---

## 🏃 运行程序

编译完成后，可执行程序生成在 `build/Release/Snipping_OCR_Tool.exe` 或 `build/Snipping_OCR_Tool.exe`。

请确保运行环境中包含相应的动态库路径 (例如将 Qt `bin` 目录与 llama.cpp `bin/Release` 目录加入系统的 `PATH` 环境变量中)：

```cmd
:: 设置环境变量并启动程序
set "PATH=D:\LearnWorkSpace\project\llama.cpp\build\bin\Release;C:\Qt\6.11.1\msvc2022_64\bin;%PATH%"
.\build\Release\Snipping_OCR_Tool.exe
```

---