#include "backtester.h"
#include <QtNetwork>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Backtester::Backtester(double com, double bal, bool fut, double lev, int64_t from, int64_t to, StrategyManager *sm)
        : commission_(com), balance_(bal), position_(0), futures_(fut), leverage_(lev), from_(from), to_(to),
          stratMgr_(sm) {}

bool Backtester::fetch_data(const QString &symbol, const QString &interval) {
    QString url = QString("https://api.binance.com/api/v3/klines?symbol=%1&interval=%2&limit=1000").arg(symbol).arg(
            interval);
    QNetworkAccessManager mgr;
    QEventLoop loop;
    auto *rep = mgr.get(QNetworkRequest(QUrl(url)));
    QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (rep->error() != QNetworkReply::NoError) return false;
    auto arr = json::parse(rep->readAll().toStdString());
    ticks_.clear();
    for (auto &e: arr) {
        int64_t ts = e[0];
        if (ts < from_ || ts > to_) continue;
        double c = std::stod(e[4].get<std::string>());
        ticks_.push_back({ts, c, std::stod(e[5].get<std::string>())});
    }
    return !ticks_.empty();
}

double Backtester::run() {
    trades_.clear();
    std::vector<double> prices;
    for (size_t i = 0; i < ticks_.size(); ++i) {
        prices.push_back(ticks_[i].price);
        int sig = stratMgr_->user_signal(prices);
        qDebug() << "Bar" << i << "price=" << ticks_[i].price << "sig=" << sig;
        double prev = (i ? ticks_[i - 1].price : prices[0]);
        if (sig == 1 && position_ <= 0) {
            double qty = balance_ * leverage_ / ticks_[i].price;
            double cost = qty * ticks_[i].price, fee = cost * commission_;
            balance_ -= cost + fee;
            position_ = qty;
            trades_.push_back({ticks_[i].timestamp, true, ticks_[i].price, qty, 0});
        } else if (sig == 0 && position_ > 0) {
            double rev = position_ * ticks_[i].price, fee = rev * commission_;
            balance_ += rev - fee;
            double pnl = (ticks_[i].price - prev) * position_ - fee;
            trades_.push_back({ticks_[i].timestamp, false, ticks_[i].price, position_, pnl});
            position_ = 0;
        }
    }
    return balance_ + position_ * ticks_.back().price;
}

const std::vector<Trade> &Backtester::trades() const { return trades_; }

