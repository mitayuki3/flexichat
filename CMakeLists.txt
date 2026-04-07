cmake_minimum_required(VERSION 3.16)
project(FlexiChat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6の検索と読み込み
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# ソースファイルの設定
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ChatWidget.cpp
    src/LMStudioClient.cpp
)

set(HEADERS
    src/MainWindow.h
    src/ChatWidget.h
    src/LMStudioClient.h
)

# 実行ファイルの作成
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# Qtライブラリをリンク
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)
