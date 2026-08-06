cmake_minimum_required(VERSION 3.20)
project(Snipping_OCR_Tool)

set(CMAKE_CXX_STANDARD 20)

# MSVC: force UTF-8 for source files
if (MSVC)
    add_compile_options("/utf-8")
endif()

# ===== 外部依赖路径参数配置 (通过 -D 传入) =====
set(Qt6_DIR "" CACHE PATH "Path to Qt6 CMake config (e.g. C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6)")
set(LLAMACPP_DIR "" CACHE PATH "Path to llama.cpp root directory (e.g. D:/LearnWorkSpace/project/llama.cpp)")
set(LLAMACPP_BUILD "" CACHE PATH "Path to llama.cpp build directory (defaults to LLAMACPP_DIR/build)")

if (NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    message(FATAL_ERROR "Qt6_DIR is not set. Please pass -DQt6_DIR=\"<path_to_Qt6_cmake>\" (e.g. -DQt6_DIR=\"C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6\") or set CMAKE_PREFIX_PATH.")
endif()

if (NOT LLAMACPP_DIR)
    message(FATAL_ERROR "LLAMACPP_DIR is not set. Please pass -DLLAMACPP_DIR=\"<path_to_llama_cpp>\" (e.g. -DLLAMACPP_DIR=\"D:/LearnWorkSpace/project/llama.cpp\").")
endif()

if (NOT LLAMACPP_BUILD)
    set(LLAMACPP_BUILD "${LLAMACPP_DIR}/build")
endif()

include(FetchContent)

FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/include.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(nlohmann_json)

include_directories("${nlohmann_json_SOURCE_DIR}/single_include")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 让qhotkey适配qt6
set(QT_DEFAULT_MAJOR_VERSION 6 CACHE STRING "" FORCE)
# 让新版CMake兼容老项目qhotkey中的的cmake_minimum_required
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
        qhotkey
        GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
        GIT_TAG "1.5.0"
)

FetchContent_MakeAvailable(qhotkey)

# ===== llama.cpp =====
include_directories("${LLAMACPP_DIR}/include")
include_directories("${LLAMACPP_DIR}/ggml/include")
include_directories("${LLAMACPP_DIR}/tools/mtmd")

add_executable(Snipping_OCR_Tool
    main.cpp
    MainWindow.h
    MainWindow.cpp
    SnipperWidget.h
    SnipperWidget.cpp
    OcrEngine.h
    OcrEngine.cpp
    AppConfig.h
    AppConfig.cpp
    ConfigDialog.h
    ConfigDialog.cpp
    resources.qrc
)

target_link_libraries(Snipping_OCR_Tool PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    QHotkey::QHotkey
    "${LLAMACPP_BUILD}/src/Release/llama.lib"
    "${LLAMACPP_BUILD}/tools/mtmd/Release/mtmd.lib"
    "${LLAMACPP_BUILD}/ggml/src/Release/ggml.lib"
    "${LLAMACPP_BUILD}/ggml/src/Release/ggml-base.lib"
    "${LLAMACPP_BUILD}/ggml/src/Release/ggml-cpu.lib"
)

if(MSVC)
    target_compile_options(Snipping_OCR_Tool PRIVATE /Zc:__cplusplus)
endif()
