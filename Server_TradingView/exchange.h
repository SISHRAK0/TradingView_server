#ifndef TRADING_BOT_EXCHANGE_H
#define TRADING_BOT_EXCHANGE_H

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "config.h"

class Exchange {
public:
    virtual ~Exchange() = default;

    virtual std::string name() const = 0;

    virtual double price(const std::string &sym) = 0;

    virtual std::vector<std::vector<double>> klines(const std::string &sym,
                                                    const std::string &tf, int limit) = 0;

    virtual bool order(const std::string &side,
                       const std::string &pos,
                       const std::string &sym,
                       double qty) = 0;

    void setCreds(const ApiCreds &c) { creds = c; }

protected:
    ApiCreds creds;
};

Exchange *getEx(const std::string &name);

extern std::unordered_map<std::string, std::unique_ptr<Exchange>> g_exchanges;


#endif //TRADING_BOT_EXCHANGE_H
