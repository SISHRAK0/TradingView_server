cmake_minimum_required(VERSION 3.28)
project(TradingView_server LANGUAGES CXX)

# Используем стандарт C++ (в примере C++17, можно заменить на 20, если требуется)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Подавление предупреждений об устаревшем OpenGL (для macOS)
add_definitions(-DGL_SILENCE_DEPRECATION)

# Поиск зависимостей, установленных через Homebrew или систему
find_package(spdlog REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)

# Для линковки с OpenGL на macOS
if(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
endif()

# Пути к источникам ImGui (предполагается, что вы разместили его в external/imgui)
#set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
#set(IMGUI_BACKENDS ${IMGUI_DIR}/backends)

# Список исходников ImGui (убедитесь, что все файлы взяты из одного репозитория)
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_BACKENDS}/imgui_impl_glfw.cpp
        ${IMGUI_BACKENDS}/imgui_impl_opengl3.cpp
)

# Если у вас есть исходники BinaCPP, например, src/binacpp.cpp,
# добавьте их в список исходников. Убедитесь, что реализации функций
# BinaCPP::init, BinaCPP::get_price и т.д. находятся в указанном файле.
#set(BINACPP_SOURCES ${CMAKE_SOURCE_DIR}/binacpp/src/binacpp.cpp)
#include_directories(binacpp)
# Создаём цель (сначала add_executable!)
add_executable(TradingView_server
        Server_TradingView/main.cpp
#        ${IMGUI_SOURCES}
#        ${BINACPP_SOURCES}  # Добавьте этот файл, если используете BinaCPP
)

# Указываем пути для включения заголовочных файлов
target_include_directories(TradingView_server PRIVATE
#        ${IMGUI_DIR}
#        ${IMGUI_BACKENDS}
        ${spdlog_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src    # для binacpp.h
)

# Линкуем цель с необходимыми библиотеками
target_link_libraries(TradingView_server PRIVATE
        spdlog::spdlog
        jsoncpp_lib
        glfw
        CURL::libcurl
#        ${OpenGL_LIBRARY}  # для OpenGL (на macOS это "-framework OpenGL")
)
