#ifndef TRADING_BOT_OKX_EX_H
#define TRADING_BOT_OKX_EX_H

#pragma once

#include "exchange.h"

class OkxEx : public Exchange {
public:
    std::string name() const override { return "OKX"; }

    double price(const std::string &sym) override;

    std::vector<std::vector<double>> klines(const std::string &sym,
                                            const std::string &tf,
                                            int limit) override;

    bool order(const std::string &side,
               const std::string &pos,
               const std::string &sym,
               double qty) override;
};


#endif //TRADING_BOT_OKX_EX_H
