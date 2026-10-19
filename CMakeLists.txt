cmake_minimum_required(VERSION 3.20)
project(MathPaperStudio LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MINGW)
    add_compile_options(-O2 -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

set(CORE_SOURCES
    src/util.cpp
    src/json.cpp
    src/model.cpp
    src/seed.cpp
    src/store.cpp
    src/ai.cpp
    src/docx.cpp
    src/image_data.cpp
    src/math_text.cpp
)

add_executable(MathPaperStudio WIN32
    src/main.cpp
    src/app.cpp
    src/app.h
    src/editor.cpp
    src/editor.h
    src/ui.cpp
    src/ui.h
    src/image_data.h
    ${CORE_SOURCES}
    res/app.rc
    res/app.manifest
    res/app.ico
)

target_include_directories(MathPaperStudio PRIVATE src)
target_compile_definitions(MathPaperStudio PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(MathPaperStudio PRIVATE
    gdiplus
    winhttp
    crypt32
    shell32
    ole32
    comdlg32
    comctl32
    dwmapi
    uxtheme
    uuid
    gdi32
    user32
)
if(MINGW)
    target_link_options(MathPaperStudio PRIVATE -municode)
endif()
set_target_properties(MathPaperStudio PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_NAME "MathPaperStudio"
)

add_executable(MathPaperStudioSelfTest
    src/selftest.cpp
    ${CORE_SOURCES}
)
target_include_directories(MathPaperStudioSelfTest PRIVATE src)
target_compile_definitions(MathPaperStudioSelfTest PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(MathPaperStudioSelfTest PRIVATE
    winhttp
    crypt32
    shell32
    ole32
    gdiplus
)
set_target_properties(MathPaperStudioSelfTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    OUTPUT_NAME "selftest"
)
