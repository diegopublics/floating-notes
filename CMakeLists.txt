cmake_minimum_required(VERSION 3.30)

project(FloatingNotes
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Sql Svg Widgets)
find_package(Qt6 6.11 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(REQUIRES 6.11)

qt_add_executable(FloatingNotes
    src/main.cpp
    src/app/AppController.cpp
    src/app/AppController.h
    src/app/AppSettings.cpp
    src/app/AppSettings.h
    src/app/ImportExport.cpp
    src/app/ImportExport.h
    src/core/Note.cpp
    src/core/Note.h
    src/core/NoteBodyFormat.cpp
    src/core/NoteBodyFormat.h
    src/platform/GlobalHotkeys.cpp
    src/platform/GlobalHotkeys.h
    src/platform/StartupIntegration.cpp
    src/platform/StartupIntegration.h
    src/platform/windows/WindowsGlobalHotkeys.cpp
    src/platform/windows/WindowsGlobalHotkeys.h
    src/platform/windows/WindowsStartupIntegration.cpp
    src/platform/windows/WindowsStartupIntegration.h
    src/persistence/NoteRepository.cpp
    src/persistence/NoteRepository.h
    src/ui/AllNotesWindow.cpp
    src/ui/AllNotesWindow.h
    src/ui/AppTheme.cpp
    src/ui/AppTheme.h
    src/ui/AppIcons.cpp
    src/ui/AppIcons.h
    src/ui/EdgeDockWindow.cpp
    src/ui/EdgeDockWindow.h
    src/ui/HelpDialog.cpp
    src/ui/HelpDialog.h
    src/ui/MarkdownEditor.cpp
    src/ui/MarkdownEditor.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
    resources/resources.qrc
)

target_link_libraries(FloatingNotes
    PRIVATE
        Qt6::Sql
        Qt6::Svg
        Qt6::Widgets
)

if(WIN32)
    target_link_libraries(FloatingNotes PRIVATE ole32 shell32 user32)
endif()

target_include_directories(FloatingNotes
    PRIVATE
        src
)

set_target_properties(FloatingNotes PROPERTIES
    WIN32_EXECUTABLE ON
)

qt_add_translations(FloatingNotes
    SOURCE_TARGETS FloatingNotes
    TS_FILES translations/FloatingNotes_es.ts
)
