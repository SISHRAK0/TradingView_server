#ifndef TRADING_BOT_BACKTESTER_H
#define TRADING_BOT_BACKTESTER_H

#pragma once

#include <vector>
#include <QString>
#include <cstdint>
#include "StrategyManager.h"

struct Tick {
    int64_t timestamp;
    double price;
    double volume;
};
struct Trade {
    int64_t timestamp;
    bool isBuy;
    double price, qty, pnl;
};

class Backtester {
public:
    Backtester(double commission, double initBal, bool futures, double lev,
               int64_t fromTs, int64_t toTs, StrategyManager *stratMgr);

    bool fetch_data(const QString &symbol, const QString &interval);

    double run();

    const std::vector<Trade> &trades() const;

private:
    std::vector<Tick> ticks_;
    std::vector<Trade> trades_;
    double commission_, balance_, position_;
    bool futures_;
    double leverage_;
    int64_t from_, to_;
    StrategyManager *stratMgr_;
};


#endif //TRADING_BOT_BACKTESTER_H
