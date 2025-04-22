#ifndef TRADING_BOT_STRATEGY_FUNCTIONS_H
#define TRADING_BOT_STRATEGY_FUNCTIONS_H

#pragma once

#include <vector>
#include <cmath>

// Example indicators
static double ma(const std::vector<double> &p, int per) {
    if ((int) p.size() < per) return 0.0;
    double s = 0;
    for (int i = p.size() - per; i < (int) p.size(); ++i) s += p[i];
    return s / per;
}

static double rsi(const std::vector<double> &p, int per) {
    if ((int) p.size() <= per) return 0.0;
    double g = 0, l = 0;
    for (int i = p.size() - per; i < (int) p.size() - 1; ++i) {
        double d = p[i + 1] - p[i];
        if (d > 0) g += d; else l -= d;
    }
    double ag = g / per, al = l / per;
    if (al == 0) return 100.0;
    double rs = ag / al;
    return 100.0 - (100.0 / (1.0 + rs));
}

static double ema(const std::vector<double> &p, int per) {
    if ((int) p.size() < per) return 0.0;
    double k = 2.0 / (per + 1);
    double e = ma(std::vector<double>(p.end() - per, p.end()), per);
    for (int i = p.size() - per + 1; i < (int) p.size(); ++i) e = p[i] * k + e * (1 - k);
    return e;
}

static std::pair<double, double> macd(const std::vector<double> &p, int f, int s, int sig) {
    double fast = ema(p, f), slow = ema(p, s);
    double line = fast - slow;
    std::vector<double> tmp(sig, line);
    double sigl = ema(tmp, sig);
    return {line, sigl};
}

static double stdev(const std::vector<double> &p, int per, double m) {
    if ((int) p.size() < per) return 0.0;
    double sum = 0;
    for (int i = p.size() - per; i < (int) p.size(); ++i) sum += (p[i] - m) * (p[i] - m);
    return std::sqrt(sum / per);
}

// User defines this function below:
int strategy(const std::vector<double> &prices) {
    // Example: buy if price > 50-period MA, sell if below
    double cur = prices.back();
    double m = ma(prices, 50);
    if (cur > m) return 1;
    if (cur < m) return 0;
    return -1;
}

#endif //TRADING_BOT_STRATEGY_FUNCTIONS_H
