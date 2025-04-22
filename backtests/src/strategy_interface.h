#ifndef TRADING_BOT_STRATEGY_INTERFACE_H
#define TRADING_BOT_STRATEGY_INTERFACE_H

#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct Tick {
    int64_t timestamp;  // Unix ms
    double price;
    double volume;
};

struct Trade {
    int64_t timestamp;
    enum Type {
        BUY, SELL
    } type;
    double price;
    double quantity;
    double pnl;  // profit/loss for this trade (from previous position)
};

int strategy(const std::vector<double> &prices);

#endif //TRADING_BOT_STRATEGY_INTERFACE_H
