cmake_minimum_required(VERSION 3.22)
 project(trading_bot LANGUAGES CXX)
 set(CMAKE_CXX_STANDARD 17)
 set(CMAKE_CXX_STANDARD_REQUIRED ON)
 set(CMAKE_AUTOMOC ON)
 set(CMAKE_AUTOUIC ON)
 set(CMAKE_AUTORCC ON)
 find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)
 find_package(spdlog   REQUIRED)
 find_package(CURL     REQUIRED)
 find_package(JsonCpp  REQUIRED)
 find_package(OpenSSL  REQUIRED)
 find_package(Threads  REQUIRED)
 add_executable(trading_bot Server_TradingView/main.cpp)
 target_link_libraries(trading_bot PRIVATE Qt6::Widgets Qt6::Charts
 CURL::libcurl spdlog::spdlog jsoncpp_lib OpenSSL::Crypto Threads::Threads)