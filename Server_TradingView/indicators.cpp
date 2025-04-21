#include "indicators.h"
#include <cmath>

double ma(const std::vector<double> &p, int per) {
    if ((int) p.size() < per) return 0.0;
    double s = 0;
    for (int i = p.size() - per; i < (int) p.size(); ++i) s += p[i];
    return s / per;
}

double rsi(const std::vector<double> &p, int per) {
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

double ema(const std::vector<double> &p, int per) {
    if ((int) p.size() < per) return 0.0;
    double k = 2.0 / (per + 1);
    double e = ma({p.end() - per, p.end()}, per);
    for (int i = p.size() - per + 1; i < (int) p.size(); ++i) {
        e = p[i] * k + e * (1 - k);
    }
    return e;
}

std::pair<double, double> macd(const std::vector<double> &p, int f, int s, int sig) {
    double fast = ema(p, f), slow = ema(p, s);
    double line = fast - slow;
    std::vector<double> tmp(sig, line);
    double sigl = ema(tmp, sig);
    return {line, sigl};
}

double stdev(const std::vector<double> &p, int per, double m) {
    if ((int) p.size() < per) return 0.0;
    double sum = 0;
    for (int i = p.size() - per; i < (int) p.size(); ++i) {
        double d = p[i] - m;
        sum += d * d;
    }
    return std::sqrt(sum / per);
}
