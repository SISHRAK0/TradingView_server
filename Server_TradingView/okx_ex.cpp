#include "okx_ex.h"
#include "util.h"
#include <json/json.h>
#include <spdlog/spdlog.h>

double OkxEx::price(const std::string &sym) {
    auto r = httpGet("https://www.okx.com/api/v5/market/ticker?instId=" + sym);
    Json::Value j;
    if (!parseJsonStr(r, j)) {
        spdlog::error("OKX price parse error");
        return 0.0;
    }
    return std::stod(j["data"][0]["last"].asString());
}

std::vector<std::vector<double>> OkxEx::klines(const std::string &sym,
                                               const std::string &tf,
                                               int limit) {
    auto r = httpGet("https://www.okx.com/api/v5/market/candles?instId=" + sym +
                     "&bar=" + tf + "&limit=" + std::to_string(limit));
    Json::Value v;
    if (!parseJsonStr(r, v)) {
        spdlog::error("OKX klines parse error");
        return {};
    }
    std::vector<std::vector<double>> out;
    for (auto &c: v["data"]) {
        out.push_back({
                              std::stod(c[0].asString()),
                              std::stod(c[1].asString()),
                              std::stod(c[2].asString()),
                              std::stod(c[3].asString()),
                              std::stod(c[4].asString())
                      });
    }
    return out;
}

bool OkxEx::order(const std::string &side,
                  const std::string &pos,
                  const std::string &sym,
                  double qty) {
    spdlog::warn("OKX order() not implemented");
    return false;
}
