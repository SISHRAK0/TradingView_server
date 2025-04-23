cmake_minimum_required(VERSION 3.14)
project(CryptoBacktesterGUI)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(nlohmann_json REQUIRED)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
add_executable(CryptoBacktesterGUI
        src/main.cpp
        src/MainWindow.cpp
        src/backtester.cpp
        src/StrategyManager.h
        src/StrategyManager.cpp
        src/strategy_interface.h
        ../mining/main.cpp
)
qt6_wrap_ui(UISrcs src/MainWindow.ui)
target_sources(CryptoBacktesterGUI PRIVATE ${UISrcs})
target_include_directories(CryptoBacktesterGUI PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(CryptoBacktesterGUI PRIVATE Qt6::Widgets Qt6::Network dl nlohmann_json::nlohmann_json)

