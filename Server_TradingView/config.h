#ifndef TRADING_BOT_CONFIG_H
#define TRADING_BOT_CONFIG_H

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <json/json.h>


struct ApiCreds {
    std::string key, secret;
};
extern std::unordered_map<std::string, ApiCreds> g_creds;

// Telegram settings
extern std::string g_tgToken, g_tgChat;

// Мониторинг и конфиг для каждой монеты
enum AlgoId {
    ALG_NONE = -1, ALG_RSI_MA, ALG_RSI, ALG_BB_MA, ALG_MACD_X
};
struct CoinCfg {
    std::string ex = "BINANCE", tf = "1m";
    int algo = ALG_NONE, rsiPeriod = 14, maPeriod = 50;
    double oversold = 30, overbought = 70, qty = 0.001;
    int bbPeriod = 20;
    double bbK = 2.0;
    int emaFast = 12, emaSlow = 26, emaSig = 9;
};
extern std::unordered_map<std::string, CoinCfg> g_cfg;
extern std::vector<std::string> g_watch;
extern std::mutex g_m;
extern std::vector<std::string> g_signals;

#endif //TRADING_BOT_CONFIG_H
