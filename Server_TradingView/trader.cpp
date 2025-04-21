#include "trader.h"
#include "exchange.h"
#include "config.h"
#include "indicators.h"
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>

static void traderLoop() {
    spdlog::info("Trader started");
    while (true) {
        std::vector<std::string> symbols;
        {
            std::lock_guard<std::mutex> lk(g_m);
            symbols = g_watch;
        }
        for (auto &sym: symbols) {
            CoinCfg cfg;
            {
                std::lock_guard<std::mutex> lk(g_m);
                cfg = g_cfg[sym];
            }
            auto ex = getEx(cfg.ex);
            if (!ex) continue;
            int need = std::max({cfg.maPeriod, cfg.rsiPeriod, cfg.bbPeriod, cfg.emaSlow}) + 2;
            auto kl = ex->klines(sym, cfg.tf, need);
            if ((int) kl.size() < need) continue;
            std::vector<double> closes;
            for (auto &c: kl) closes.push_back(c[4]);
            double price = ex->price(sym);

            double m = ma(closes, cfg.maPeriod);
            double rs = rsi(closes, cfg.rsiPeriod);
            bool buy = false, sell = false;

            switch (cfg.algo) {
                case ALG_RSI_MA:
                    buy = rs < cfg.oversold && price > m;
                    sell = rs > cfg.overbought && price < m;
                    break;
                case ALG_RSI:
                    buy = rs < cfg.oversold;
                    sell = rs > cfg.overbought;
                    break;
                case ALG_BB_MA: {
                    double mm = ma(closes, cfg.bbPeriod);
                    double sd = stdev(closes, cfg.bbPeriod, mm);
                    double bbL = mm - cfg.bbK * sd;
                    double bbU = mm + cfg.bbK * sd;
                    buy = price < bbL && price > m;
                    sell = price > bbU && price < m;
                    break;
                }
                case ALG_MACD_X: {
                    static std::unordered_map<std::string, std::pair<double, double>> prev;
                    auto cur = macd(closes, cfg.emaFast, cfg.emaSlow, cfg.emaSig);
                    auto pr = prev[sym];
                    buy = (pr.first < pr.second && cur.first > cur.second);
                    sell = (pr.first > pr.second && cur.first < cur.second);
                    prev[sym] = cur;
                    break;
                }
                default:
                    break;
            }

            if (buy || sell) {
                std::string side = buy ? "BUY" : "SELL";
                ex->order(side, buy ? "LONG" : "SHORT", sym, cfg.qty);
                auto now = std::chrono::system_clock::now();
                auto t = std::chrono::system_clock::to_time_t(now);
                char buf[64];
                strftime(buf, sizeof(buf), "%F %T", std::localtime(&t));
                std::ostringstream msg;
                msg << buf << " " << side << " " << sym;
                spdlog::info(msg.str());
                {
                    std::lock_guard<std::mutex> lk(g_m);
                    g_signals.push_back(msg.str());
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void startTraderThread() {
    std::thread(traderLoop).detach();
}
