#include "binance_ex.h"
#include "util.h"
#include <json/json.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <curl/curl.h>

std::string BinanceEx::name() const { return "BINANCE"; }

double BinanceEx::price(const std::string &sym) {
    auto r = httpGet("https://fapi.binance.com/fapi/v1/ticker/price?symbol=" + sym);
    Json::Value j;
    if (!parseJsonStr(r, j)) {
        spdlog::error("Binance price parse error");
        return 0.0;
    }
    return std::stod(j["price"].asString());
}

std::vector<std::vector<double>> BinanceEx::klines(const std::string &sym,
                                                   const std::string &tf,
                                                   int limit) {
    auto r = httpGet("https://fapi.binance.com/fapi/v1/klines?symbol="
                     + sym + "&interval=" + tf + "&limit=" + std::to_string(limit));
    Json::Value a;
    if (!parseJsonStr(r, a) || !a.isArray()) {
        spdlog::error("Binance klines parse error");
        return {};
    }
    std::vector<std::vector<double>> out;
    for (auto &c: a) {
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

bool BinanceEx::order(const std::string &side,
                      const std::string &pos,
                      const std::string &sym,
                      double qty) {
    if (creds.key.empty()) {
        spdlog::warn("No API credentials");
        return false;
    }
    long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream p;
    p << "symbol=" << sym << "&side=" << side
      << "&type=MARKET&quantity=" << qty
      << "&positionSide=" << pos << "&timestamp=" << ts;
    std::string sig = hmacSha256(creds.secret, p.str());
    auto hdr =
            curl_slist_append(nullptr, ("X-MBX-APIKEY: " + creds.key).c_str());
    auto resp = httpPost("https://fapi.binance.com/fapi/v1/order",
                         p.str() + "&signature=" + sig,
                         "application/x-www-form-urlencoded", hdr);
    spdlog::info(resp);
    tgSend(resp);
    return !resp.empty();
}
