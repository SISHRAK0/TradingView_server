cmake_minimum_required(VERSION 3.22)
 project(trading_bot LANGUAGES CXX)
 set(CMAKE_CXX_STANDARD 17)
 set(CMAKE_CXX_STANDARD_REQUIRED ON)
 find_package(spdlog   REQUIRED)
 find_package(CURL     REQUIRED)
 find_package(JsonCpp  REQUIRED)
 find_package(OpenSSL  REQUIRED)
 find_package(Threads  REQUIRED)
 add_executable(trading_bot Server_TradingView/server.cpp
         Server_TradingView/util.cpp
         Server_TradingView/util.h
         Server_TradingView/config.h
         Server_TradingView/exchange.h
         Server_TradingView/binance_ex.cpp
         Server_TradingView/binance_ex.h
         Server_TradingView/indicators.cpp
         Server_TradingView/indicators.h
         Server_TradingView/trader.cpp
         Server_TradingView/trader.h
         Server_TradingView/config.cpp
         Server_TradingView/finandy_ex.cpp
         Server_TradingView/finandy_ex.h
         Server_TradingView/bybit_ex.cpp
         Server_TradingView/bybit_ex.h
         Server_TradingView/okx_ex.cpp
         Server_TradingView/okx_ex.h
         Server_TradingView/exchange.cpp)
 target_link_libraries(trading_bot PRIVATE
 CURL::libcurl spdlog::spdlog jsoncpp_lib OpenSSL::Crypto Threads::Threads)