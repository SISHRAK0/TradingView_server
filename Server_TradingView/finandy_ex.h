#ifndef TRADING_BOT_FINANDY_EX_H
#define TRADING_BOT_FINANDY_EX_H

#pragma once

#include "binance_ex.h"
#include <json/json.h>

class FinandyEx : public BinanceEx {
public:
     [[nodiscard]] std::string name() const override { return "FINANDY"; }

    void setJson(const Json::Value &j);

    bool order(const std::string &side,
               const std::string &pos,
               const std::string &sym,
               double qty) override;

private:
    Json::Value tmpl_;
};


#endif //TRADING_BOT_FINANDY_EX_H
