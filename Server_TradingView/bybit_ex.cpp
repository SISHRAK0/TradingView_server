#include "bybit_ex.h"
#include "util.h"
#include <json/json.h>
#include <spdlog/spdlog.h>

double BybitEx::price(const std::string &sym) {
    auto r = httpGet("https://api.bybit.com/v5/market/tickers?category=linear&symbol=" + sym);
    Json::Value j;
    if (!parseJsonStr(r, j)) {
        spdlog::error("Bybit price parse error");
        return 0.0;
    }
    return std::stod(j["result"]["list"][0]["lastPrice"].asString());
}

std::vector<std::vector<double>> BybitEx::klines(const std::string &sym,
                                                 const std::string &tf,
                                                 int limit) {
    auto r = httpGet("https://api.bybit.com/v5/market/kline?category=linear&symbol="
                     + sym + "&interval=" + tf + "&limit=" + std::to_string(limit));
    Json::Value j;
    if (!parseJsonStr(r, j)) {
        spdlog::error("Bybit klines parse error");
        return {};
    }
    std::vector<std::vector<double>> out;
    for (auto &c: j["result"]["list"]) {
        out.push_back({
                              c[0].asDouble(),
                              std::stod(c[1].asString()),
                              std::stod(c[2].asString()),
                              std::stod(c[3].asString()),
                              std::stod(c[4].asString())
                      });
    }
    return out;
}

bool BybitEx::order(const std::string &side,
                    const std::string &pos,
                    const std::string &sym,
                    double qty) {
    spdlog::warn("Bybit order() not implemented");
    return false;
}
