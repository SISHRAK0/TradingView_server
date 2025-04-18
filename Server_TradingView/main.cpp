/*───────────────────────────────────────────────────────────────────────────────
  Полностью рабочий backend‑файл `main.cpp`
  (C++17, single‑file, собирается: 
   g++ -std=c++17 -O2 main.cpp -lcurl -lssl -lcrypto -ljsoncpp -lpthread -lspdlog)
───────────────────────────────────────────────────────────────────────────────*/

#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <curl/curl.h>
#include <json/json.h>
#include <openssl/hmac.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "httplib.h"

using namespace httplib;

/* ───────── helpers ───────── */
static size_t writeCb(void *p, size_t s, size_t n, void *ud) {
    ((std::string *) ud)->append((char *) p, s * n);
    return s * n;
}

static bool parseJsonStr(const std::string &raw, Json::Value &out) {
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream is(raw);
    return Json::parseFromStream(builder, is, &out, &errs);
}

static std::string httpGet(const std::string &u) {
    CURL *c = curl_easy_init();
    if (!c) return {};
    std::string b;
    curl_easy_setopt(c, CURLOPT_URL, u.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 5L);
    curl_easy_perform(c);
    curl_easy_cleanup(c);
    return b;
}

static std::string httpPost(const std::string &u, const std::string &body,
                            const std::string &ct = "application/json",
                            struct curl_slist *hdr = nullptr) {
    CURL *c = curl_easy_init();
    if (!c) return {};
    std::string resp;
    struct curl_slist *h = hdr;
    if (!h) h = curl_slist_append(nullptr, ("Content-Type: " + ct).c_str());
    curl_easy_setopt(c, CURLOPT_URL, u.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 5L);
    curl_easy_perform(c);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return resp;
}

/* ───────── global settings ───────── */
struct ApiCreds {
    std::string key, secret;
};
static std::unordered_map<std::string, ApiCreds> g_creds;

static std::string g_tgToken, g_tgChat;

static void tgSend(const std::string &m) {
    if (g_tgToken.empty()) return;
    Json::Value p;
    p["chat_id"] = g_tgChat;
    p["text"] = m;
    httpPost("https://api.telegram.org/bot" + g_tgToken + "/sendMessage",
             p.toStyledString());
}

/* ───────── exchange abstraction ───────── */
class Exchange {
public:
    virtual ~Exchange() = default;

    virtual std::string name() const = 0;

    virtual double price(const std::string &) = 0;

    virtual std::vector<std::vector<double>> klines(const std::string &,
                                                    const std::string &, int) = 0;

    virtual bool order(const std::string &, const std::string &,
                       const std::string &, double) = 0;

    void setCreds(const ApiCreds &c) { creds = c; }

protected:
    ApiCreds creds;
};

static std::unordered_map<std::string, std::unique_ptr<Exchange>> g_exchanges;

static Exchange *getEx(const std::string &n) {
    auto it = g_exchanges.find(n);
    return it == g_exchanges.end() ? nullptr : it->second.get();
}

/* ───────── HMAC‑SHA256 ───────── */
static std::string hmacSha256(const std::string &k, const std::string &d) {
    unsigned char *h = HMAC(EVP_sha256(), k.data(), k.size(),
                            (unsigned char *) d.data(), d.size(), nullptr, nullptr);
    std::ostringstream o;
    for (int i = 0; i < 32; ++i)
        o << std::hex << std::setw(2) << std::setfill('0') << (int) h[i];
    return o.str();
}

/* ───────── Binance (futures) ───────── */
class BinanceEx : public Exchange {
public:
    std::string name() const override { return "BINANCE"; }

    double price(const std::string &sym) override {
        auto r = httpGet(
                "https://fapi.binance.com/fapi/v1/ticker/price?symbol=" + sym);
        Json::Value j;
        if (!parseJsonStr(r, j)) {
            spdlog::error("Failed to parse JSON from Binance price response");
            return 0.0;  // или любое дефолтное значение / обработка ошибки
        }
        double price = std::stod(j["price"].asString());
        return price;
    }

    std::vector<std::vector<double>> klines(const std::string &sym,
                                            const std::string &tf,
                                            int lim) override {
        auto r = httpGet("https://fapi.binance.com/fapi/v1/klines?symbol=" + sym +
                         "&interval=" + tf + "&limit=" + std::to_string(lim));
        Json::Value a;
        if (!parseJsonStr(r,a)) {
            spdlog::error("Failed to parse JSON from Binance price response");
//            return 0.0;  // или любое дефолтное значение / обработка ошибки
        }
        std::vector<std::vector<double>> out;
        if (!a.isArray()) return out;
        for (auto &c: a) {
            out.push_back({c[0].asDouble(),
                           std::stod(c[1].asString()),
                           std::stod(c[2].asString()),
                           std::stod(c[3].asString()),
                           std::stod(c[4].asString())});
        }
        return out;
    }

    bool order(const std::string &side, const std::string &pos,
               const std::string &sym, double qty) override {
        if (creds.key.empty()) {
            spdlog::warn("no creds");
            return false;
        }
        long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        std::ostringstream p;
        p << "symbol=" << sym << "&side=" << side
          << "&type=MARKET&quantity=" << qty
          << "&positionSide=" << pos << "&timestamp=" << ts;
        std::string sig = hmacSha256(creds.secret, p.str());
        auto h =
                curl_slist_append(nullptr, ("X-MBX-APIKEY: " + creds.key).c_str());
        auto resp =
                httpPost("https://fapi.binance.com/fapi/v1/order",
                         p.str() + "&signature=" + sig,
                         "application/x-www-form-urlencoded", h);
        spdlog::info(resp);
        tgSend(resp);
        return !resp.empty();
    }
};

/* ───────── Finandy ───────── */
class FinandyEx : public BinanceEx {
public:
    std::string name() const override { return "FINANDY"; }

    void setJson(const Json::Value &j) { tmpl = j; }

    bool order(const std::string &side, const std::string &pos,
               const std::string &sym, double) override {
        Json::Value p = tmpl;  // пользовательский шаблон
        p["side"] = side;
        p["positionSide"] = pos;
        p["symbol"] = sym;
        auto r = httpPost(
                "https://hook.finandy.com/wwsYwdfipGXpqCE0rlUK",
                p.toStyledString());
        spdlog::info(r);
        tgSend(r);
        return !r.empty();
    }

private:
    Json::Value tmpl;  // хранит шаблон от пользователя
};

/* ───────── Bybit (linear) ───────── */
class BybitEx : public Exchange {
public:
    std::string name() const override { return "BYBIT"; }

    double price(const std::string &sym) override {
        auto r = httpGet(
                "https://api.bybit.com/v5/market/tickers?category=linear&symbol=" +
                sym);
        Json::Value j;
        if (!parseJsonStr(r, j)) {
            spdlog::error("Failed to parse JSON from Binance price response");
            return 0.0;  // или любое дефолтное значение / обработка ошибки
        }
        return std::stod(j["result"]["list"][0]["lastPrice"].asString());
    }

    std::vector<std::vector<double>> klines(const std::string &sym,
                                            const std::string &tf,
                                            int lim) override {
        auto r = httpGet(
                "https://api.bybit.com/v5/market/kline?category=linear&symbol=" +
                sym + "&interval=" + tf + "&limit=" + std::to_string(lim));
        Json::Value j;
        if (!parseJsonStr(r, j)) {
            spdlog::error("Failed to parse JSON from Binance price response");
//            return 0.0;  // или любое дефолтное значение / обработка ошибки
        }
        std::vector<std::vector<double>> out;
        for (auto &c: j["result"]["list"]) {
            out.push_back({c[0].asDouble(),
                           std::stod(c[1].asString()),
                           std::stod(c[2].asString()),
                           std::stod(c[3].asString()),
                           std::stod(c[4].asString())});
        }
        return out;
    }

    bool order(const std::string &, const std::string &, const std::string &,
               double) override {
        spdlog::warn("Bybit order(): реализуйте подпись запроса");
        return false;
    }
};

/* ───────── OKX (swap) ───────── */
class OkxEx : public Exchange {
public:
    std::string name() const override { return "OKX"; }

    double price(const std::string &sym) override {
        auto r = httpGet(
                "https://www.okx.com/api/v5/market/ticker?instId=" + sym);
        Json::Value j;
        if (!parseJsonStr(r, j)) {
            spdlog::error("Failed to parse JSON from Binance price response");
            return 0.0;  // или любое дефолтное значение / обработка ошибки
        }
        return std::stod(j["data"][0]["last"].asString());
    }

    std::vector<std::vector<double>> klines(const std::string &sym,
                                            const std::string &tf,
                                            int lim) override {
        auto r = httpGet(
                "https://www.okx.com/api/v5/market/candles?instId=" + sym +
                "&bar=" + tf + "&limit=" + std::to_string(lim));
        Json::Value v;
        if (!parseJsonStr(r, v)) {
            spdlog::error("Failed to parse JSON from Binance price response");
//            return 0.0;  // или любое дефолтное значение / обработка ошибки
        }
        std::vector<std::vector<double>> out;
        for (auto &c: v["data"]) {
            out.push_back({std::stod(c[0].asString()),
                           std::stod(c[1].asString()),
                           std::stod(c[2].asString()),
                           std::stod(c[3].asString()),
                           std::stod(c[4].asString())});
        }
        return out;
    }

    bool order(const std::string &, const std::string &, const std::string &,
               double) override {
        spdlog::warn("OKX order(): реализуйте подпись запроса");
        return false;
    }
};

/* ───────── стратегии / конфиг ───────── */
enum AlgoId {
    ALG_NONE = -1, ALG_RSI_MA = 0, ALG_RSI = 1,
    ALG_BB_MA = 2, ALG_MACD_X = 3
};

struct CoinCfg {
    std::string ex = "BINANCE";
    std::string tf = "1m";
    int algo = ALG_NONE;
    int rsiPeriod = 14, maPeriod = 50;
    double oversold = 30, overbought = 70;
    double qty = 0.001;
    int bbPeriod = 20;
    double bbK = 2.0;
    int emaFast = 12, emaSlow = 26, emaSig = 9;
};

static std::unordered_map<std::string, CoinCfg> g_cfg;
static std::vector<std::string> g_watch;
static std::mutex g_m;
static std::vector<std::string> g_signals;

/* ───────── индикаторы ───────── */
static double ma(const std::vector<double> &p, int per) {
    if ((int) p.size() < per) return 0;
    double s = 0;
    for (int i = p.size() - per; i < (int) p.size(); ++i) s += p[i];
    return s / per;
}

static double rsi(const std::vector<double> &p, int per) {
    if ((int) p.size() <= per) return 0;
    double g = 0, l = 0;
    for (int i = p.size() - per; i < (int) p.size() - 1; ++i) {
        double d = p[i + 1] - p[i];
        if (d > 0)
            g += d;
        else
            l -= d;
    }
    double ag = g / per, al = l / per;
    if (al == 0) return 100;
    double rs = ag / al;
    return 100 - (100 / (1 + rs));
}

static double ema(const std::vector<double> &p, int per) {
    if ((int) p.size() < per) return 0;
    double k = 2.0 / (per + 1);
    double e = ma({p.end() - per, p.end()}, per);
    for (int i = p.size() - per + 1; i < (int) p.size(); ++i)
        e = p[i] * k + e * (1 - k);
    return e;
}

static std::pair<double, double> macd(const std::vector<double> &p, int f,
                                      int s, int sig) {
    double fast = ema(p, f), slow = ema(p, s);
    double line = fast - slow;
    static double buf = 0;
    std::vector<double> tmp(sig, line);
    double sigl = ema(tmp, sig);
    return {line, sigl};
}

static double stdev(const std::vector<double> &p, int per, double m) {
    double sum = 0;
    for (int i = p.size() - per; i < (int) p.size(); ++i) {
        double d = p[i] - m;
        sum += d * d;
    }
    return std::sqrt(sum / per);
}

/* ───────── трейдер ───────── */
static std::unordered_map<std::string, std::pair<double, double>> g_prevMacd;

static void trader() {
    spdlog::info("trader start");
    while (true) {
        std::vector<std::string> lst;
        {
            std::lock_guard lk(g_m);
            lst = g_watch;
        }
        for (auto &sym: lst) {
            CoinCfg cfg;
            {
                std::lock_guard lk(g_m);
                cfg = g_cfg[sym];
            }
            auto ex = getEx(cfg.ex);
            if (!ex) continue;
            int need = std::max({cfg.maPeriod, cfg.rsiPeriod, cfg.bbPeriod,
                                 cfg.emaSlow}) +
                       2;
            auto kl = ex->klines(sym, cfg.tf, need);
            if (kl.size() < need) continue;
            std::vector<double> cl;
            for (auto &c: kl) cl.push_back(c[4]);
            double price = ex->price(sym);
            double m = ma(cl, cfg.maPeriod);
            double rs = rsi(cl, cfg.rsiPeriod);
            bool buy = false, sell = false;
            switch (cfg.algo) {
                case ALG_NONE:
                    break;
                case ALG_RSI_MA:
                    buy = (rs < cfg.oversold && price > m);
                    sell = (rs > cfg.overbought && price < m);
                    break;
                case ALG_RSI:
                    buy = rs < cfg.oversold;
                    sell = rs > cfg.overbought;
                    break;
                case ALG_BB_MA: {
                    double bbMean = ma(cl, cfg.bbPeriod);
                    double sd = stdev(cl, cfg.bbPeriod, bbMean);
                    double bbL = bbMean - cfg.bbK * sd;
                    double bbU = bbMean + cfg.bbK * sd;
                    buy = price < bbL && price > m;
                    sell = price > bbU && price < m;
                }
                    break;
                case ALG_MACD_X: {
                    auto pr = g_prevMacd[sym];
                    auto cur = macd(cl, cfg.emaFast, cfg.emaSlow, cfg.emaSig);
                    buy = (pr.first < pr.second && cur.first > cur.second);
                    sell = (pr.first > pr.second && cur.first < cur.second);
                    g_prevMacd[sym] = cur;
                }
                    break;
            }
            if (buy || sell) {
                std::string side = buy ? "BUY" : "SELL";
                ex->order(side, buy ? "LONG" : "SHORT", sym, cfg.qty);
                auto t = std::chrono::system_clock::to_time_t(
                        std::chrono::system_clock::now());
                std::ostringstream s;
                s << std::put_time(std::localtime(&t), "%F %T") << " " << side
                  << " " << sym;
                spdlog::info(s.str());
                {
                    std::lock_guard lk(g_m);
                    g_signals.push_back(s.str());
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

//static bool parseJson(const std::string &b, Json::Value &o) {
//    return Json::parseFromStream(Json::CharReaderBuilder{},
//                                 std::istringstream(b), &o, nullptr);
//}

/* ───────── HTTP‑API ───────── */
int main() {
    auto file_sink =
            std::make_shared<spdlog::sinks::basic_file_sink_mt>("backend.log", true);
    auto console_sink =
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
            "bot", spdlog::sinks_init_list{file_sink, console_sink}));

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* регистрация бирж */
    g_exchanges["BINANCE"] = std::make_unique<BinanceEx>();
    g_exchanges["FINANDY"] = std::make_unique<FinandyEx>();
    g_exchanges["BYBIT"] = std::make_unique<BybitEx>();
    g_exchanges["OKX"] = std::make_unique<OkxEx>();

    std::thread(trader).detach();

    Server svr;

    /* ───── symbols list ───── */
    svr.Get("/symbols", [](const Request &, Response &res) {
        Json::Value a;
        {
            std::lock_guard lk(g_m);
            for (auto &s: g_watch) a.append(s);
        }
        res.set_content(a.toStyledString(), "application/json");
    });

    svr.Post("/symbols", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body, j)) {
            res.status = 400;
            return;
        }
        std::string sym = j["symbol"].asString();
        if (sym.empty()) {
            res.status = 400;
            return;
        }
        {
            std::lock_guard lk(g_m);
            if (std::find(g_watch.begin(), g_watch.end(), sym) ==
                g_watch.end())
                g_watch.push_back(sym);
            g_cfg[sym] = {j.get("ex", "BINANCE").asString(),
                          j.get("tf", "1m").asString(),
                          j.get("algo", ALG_NONE).asInt(),
                          j.get("rsiPeriod", 14).asInt(),
                          j.get("maPeriod", 50).asInt(),
                          j.get("oversold", 30.0).asDouble(),
                          j.get("overbought", 70.0).asDouble(),
                          j.get("qty", 0.001).asDouble(),
                          j.get("bbPeriod", 20).asInt(),
                          j.get("bbK", 2.0).asDouble(),
                          j.get("emaFast", 12).asInt(),
                          j.get("emaSlow", 26).asInt(),
                          j.get("emaSig", 9).asInt()};
        }
        res.status = 201;
    });

    /* ───── price ───── */
    svr.Get("/price", [](const Request &req, Response &res) {
        if (!req.has_param("symbol")) {
            res.status = 400;
            return;
        }
        auto sym = req.get_param_value("symbol");
        CoinCfg cfg;
        {
            std::lock_guard lk(g_m);
            cfg = g_cfg[sym];
        }
        auto ex = getEx(cfg.ex);
        if (!ex) {
            res.status = 500;
            return;
        }
        Json::Value o;
        o["price"] = ex->price(sym);
        res.set_content(o.toStyledString(), "application/json");
    });

    /* ───── klines ───── */
    svr.Get("/klines", [](const Request &req, Response &res) {
        auto sym = req.get_param_value("symbol");
        auto tf = req.get_param_value("tf");
        int lim = std::stoi(req.get_param_value("limit"));
        CoinCfg cfg;
        {
            std::lock_guard lk(g_m);
            cfg = g_cfg[sym];
        }
        auto ex = getEx(cfg.ex);
        auto kl = ex ? ex->klines(sym, tf, lim)
                     : std::vector<std::vector<double>>{};
        Json::Value a;
        std::vector<double> cl;
        for (auto &c: kl) {
            cl.push_back(c[4]);
            double meanMA = cl.size() >= cfg.maPeriod ? ma(cl, cfg.maPeriod) : 0;
            double curRSI = cl.size() >= cfg.rsiPeriod ? rsi(cl, cfg.rsiPeriod) : 0;
            double bbU = 0, bbL = 0;
            if (cl.size() >= cfg.bbPeriod) {
                double m = ma(cl, cfg.bbPeriod);
                double sd = stdev(cl, cfg.bbPeriod, m);
                bbU = m + cfg.bbK * sd;
                bbL = m - cfg.bbK * sd;
            }
            auto [macdL, macdS] = macd(cl, cfg.emaFast, cfg.emaSlow, cfg.emaSig);
            Json::Value row;
            row.append(c[0]);
            row.append(c[4]);
            row.append(meanMA);
            row.append(curRSI);
            row.append(bbU);
            row.append(bbL);
            row.append(macdL);
            row.append(macdS);
            a.append(row);
        }
        res.set_content(a.toStyledString(), "application/json");
    });

    /* ───── config update ───── */
    svr.Post("/config", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body, j)) {
            res.status = 400;
            return;
        }
        std::string sym = j["symbol"].asString();
        if (sym.empty()) {
            res.status = 400;
            return;
        }
        std::lock_guard lk(g_m);
        auto &c = g_cfg[sym];
        c.algo = j.get("algo", c.algo).asInt();
        c.rsiPeriod = j.get("rsiPeriod", c.rsiPeriod).asInt();
        c.maPeriod = j.get("maPeriod", c.maPeriod).asInt();
        c.oversold = j.get("oversold", c.oversold).asDouble();
        c.overbought = j.get("overbought", c.overbought).asDouble();
        c.qty = j.get("qty", c.qty).asDouble();
        res.status = 204;
    });

    /* ───── signals ───── */
    svr.Get("/signals", [](const Request &, Response &res) {
        Json::Value a;
        {
            std::lock_guard lk(g_m);
            for (auto &s: g_signals) a.append(s);
        }
        res.set_content(a.toStyledString(), "application/json");
    });

    /* ───── keys ───── */
    svr.Post("/settings/keys", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body, j)) {
            res.status = 400;
            return;
        }
        auto ex = j["exchange"].asString();
        g_creds[ex] = {j["key"].asString(), j["secret"].asString()};
        if (auto e = getEx(ex)) e->setCreds(g_creds[ex]);
        res.status = 204;
    });

    /* ───── telegram ───── */
    svr.Post("/settings/telegram", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body, j)) {
            res.status = 400;
            return;
        }
        g_tgToken = j["token"].asString();
        g_tgChat = j["chat_id"].asString();
        res.status = 204;
    });

    /* ───── Finandy JSON upload ───── */
    svr.Post("/settings/finandy/json", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body, j)) {
            res.status = 400;
            return;
        }
        if (auto ex = dynamic_cast<FinandyEx *>(getEx("FINANDY")))
            ex->setJson(j);
        res.status = 204;
    });

    /* ───── start ───── */
    svr.listen("0.0.0.0", 8080);
    curl_global_cleanup();
    return 0;
}
