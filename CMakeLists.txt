cmake_minimum_required(VERSION 3.16)
project(FlexiChat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 6.7+ is required (QCheckBox::checkStateChanged, etc.)
find_package(Qt6 6.7 REQUIRED COMPONENTS
    Widgets Gui Network LinguistTools Multimedia)

set(PROJECT_SOURCES
    src/AppSettings.cpp
    src/AppSettings.h
    src/LMStudioClient.cpp
    src/LMStudioClient.h
    src/main.cpp
    src/MainLogic.cpp
    src/MainLogic.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/MainWindow.ui
    src/OpenAITTSClient.cpp
    src/OpenAITTSClient.h
    src/ProfileManager.cpp
    src/ProfileManager.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/SettingsDialog.ui
    src/SystemPromptProfile.h
)

qt_add_executable(FlexiChat
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_link_libraries(FlexiChat PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::Multimedia)

set_target_properties(FlexiChat PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS FlexiChat
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(FlexiChat)
