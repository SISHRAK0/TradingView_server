#ifndef CRYPTOBACKTESTERGUI_STRATEGYMANAGER_H
#define CRYPTOBACKTESTERGUI_STRATEGYMANAGER_H

#pragma once

#include <QString>
#include <vector>

class StrategyManager {
public:
    StrategyManager();

    bool compile(const QString &code, QString &log);

    int user_signal(const std::vector<double> &prices);

private:
    QString soPath_;
};

#endif //CRYPTOBACKTESTERGUI_STRATEGYMANAGER_H
