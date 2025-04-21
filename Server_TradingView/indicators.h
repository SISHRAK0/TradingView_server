#ifndef TRADING_BOT_INDICATORS_H
#define TRADING_BOT_INDICATORS_H

#pragma once

#include <vector>
#include <utility>

double ma(const std::vector<double> &prices, int period);

double rsi(const std::vector<double> &prices, int period);

double ema(const std::vector<double> &prices, int period);

std::pair<double, double> macd(const std::vector<double> &prices,
                               int fast, int slow, int signal);

double stdev(const std::vector<double> &prices, int period, double mean);


#endif //TRADING_BOT_INDICATORS_H
