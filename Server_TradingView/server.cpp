#include "util.h"
#include <memory>
#include "config.h"
#include "exchange.h"
#include "trader.h"
#include "binance_ex.h"
#include "bybit_ex.h"
#include "okx_ex.h"
#include "finandy_ex.h"
#include "indicators.h"
#include <curl/curl.h>
#include <httplib.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using namespace httplib;

int main() {
    // Логи: файл + консоль
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("backend.log", true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
            "bot", spdlog::sinks_init_list{file_sink, console_sink}));

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Регистрируем биржи
    g_exchanges.emplace("BINANCE", std::unique_ptr<Exchange>(new BinanceEx()));
    g_exchanges.emplace("FINANDY", std::unique_ptr<Exchange>(new FinandyEx()));
    g_exchanges.emplace("BYBIT",   std::unique_ptr<Exchange>(new BybitEx()));
    g_exchanges.emplace("OKX",     std::unique_ptr<Exchange>(new OkxEx()));

    startTraderThread();

    Server svr;

    svr.Get("/symbols", [](const Request&, Response &res) {
        Json::Value a;
        std::lock_guard<std::mutex> lk(g_m);
        for (auto &s : g_watch) a.append(s);
        res.set_content(a.toStyledString(), "application/json");
    });

    svr.Post("/symbols", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body, j)) { res.status = 400; return; }
        std::string sym = j["symbol"].asString();
        if (sym.empty()) { res.status = 400; return; }
        {
            std::lock_guard<std::mutex> lk(g_m);
            if (std::find(g_watch.begin(), g_watch.end(), sym) == g_watch.end())
                g_watch.push_back(sym);
            g_cfg[sym] = {
                    j.get("ex","BINANCE").asString(),
                    j.get("tf","1m").asString(),
                    j.get("algo", ALG_NONE).asInt(),
                    j.get("rsiPeriod",14).asInt(),
                    j.get("maPeriod",50).asInt(),
                    j.get("oversold",30.0).asDouble(),
                    j.get("overbought",70.0).asDouble(),
                    j.get("qty",0.001).asDouble(),
                    j.get("bbPeriod",20).asInt(),
                    j.get("bbK",2.0).asDouble(),
                    j.get("emaFast",12).asInt(),
                    j.get("emaSlow",26).asInt(),
                    j.get("emaSig",9).asInt()
            };
        }
        res.status = 201;
    });

    svr.Get("/price", [](const Request &req, Response &res) {
        if (!req.has_param("symbol")) { res.status = 400; return; }
        auto sym = req.get_param_value("symbol");
        CoinCfg cfg;
        {
            std::lock_guard<std::mutex> lk(g_m);
            cfg = g_cfg[sym];
        }
        auto ex = getEx(cfg.ex);
        if (!ex) { res.status = 500; return; }
        Json::Value o;
        o["price"] = ex->price(sym);
        res.set_content(o.toStyledString(), "application/json");
    });

    svr.Get("/klines", [](const Request &req, Response &res) {
        auto sym = req.get_param_value("symbol");
        auto tf  = req.get_param_value("tf");
        int lim   = std::stoi(req.get_param_value("limit"));
        CoinCfg cfg;
        {
            std::lock_guard<std::mutex> lk(g_m);
            cfg = g_cfg[sym];
        }
        auto ex  = getEx(cfg.ex);
        auto kl  = ex ? ex->klines(sym, tf, lim) : std::vector<std::vector<double>>{};
        Json::Value a;
        std::vector<double> cl;
        for (auto &c : kl) {
            cl.push_back(c[4]);
            Json::Value row;
            row.append(c[0]);
            row.append(c[4]);
            row.append(cl.size()>=cfg.maPeriod ? ma(cl,cfg.maPeriod) : 0.0);
            row.append(cl.size()>=cfg.rsiPeriod? rsi(cl,cfg.rsiPeriod):0.0);
            double bbU=0, bbL=0;
            if (cl.size()>=cfg.bbPeriod) {
                double m = ma(cl,cfg.bbPeriod);
                double sd = stdev(cl,cfg.bbPeriod,m);
                bbU = m + cfg.bbK*sd;
                bbL = m - cfg.bbK*sd;
            }
            row.append(bbU);
            row.append(bbL);
            auto [ml,ms] = macd(cl,cfg.emaFast,cfg.emaSlow,cfg.emaSig);
            row.append(ml);
            row.append(ms);
            a.append(row);
        }
        res.set_content(a.toStyledString(), "application/json");
    });

    svr.Post("/config", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body,j)) { res.status=400; return; }
        std::string sym = j["symbol"].asString();
        if (sym.empty()) { res.status=400; return; }
        {
            std::lock_guard<std::mutex> lk(g_m);
            auto &c = g_cfg[sym];
            c.algo      = j.get("algo",c.algo).asInt();
            c.rsiPeriod = j.get("rsiPeriod",c.rsiPeriod).asInt();
            c.maPeriod  = j.get("maPeriod",c.maPeriod).asInt();
            c.oversold  = j.get("oversold",c.oversold).asDouble();
            c.overbought= j.get("overbought",c.overbought).asDouble();
            c.qty       = j.get("qty",c.qty).asDouble();
        }
        res.status = 204;
    });

    svr.Get("/signals", [](const Request&, Response &res) {
        Json::Value a;
        std::lock_guard<std::mutex> lk(g_m);
        for (auto &s : g_signals) a.append(s);
        res.set_content(a.toStyledString(), "application/json");
    });

    svr.Post("/settings/keys", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body,j)) { res.status=400; return; }
        auto ex = j["exchange"].asString();
        g_creds[ex] = { j["key"].asString(), j["secret"].asString() };
        if (auto e = getEx(ex)) e->setCreds(g_creds[ex]);
        res.status = 204;
    });

    svr.Post("/settings/telegram", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body,j)) { res.status=400; return; }
        g_tgToken = j["token"].asString();
        g_tgChat  = j["chat_id"].asString();
        res.status = 204;
    });

    svr.Post("/settings/finandy/json", [](const Request &req, Response &res) {
        Json::Value j;
        if (!parseJsonStr(req.body,j)) { res.status=400; return; }
        if (auto e = dynamic_cast<FinandyEx*>(getEx("FINANDY")))
            e->setJson(j);
        res.status = 204;
    });

    svr.listen("0.0.0.0", 8080);
    curl_global_cleanup();
    return 0;
}
