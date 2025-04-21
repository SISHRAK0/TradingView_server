#include "exchange.h"

std::unordered_map<std::string, std::unique_ptr<Exchange>> g_exchanges;

Exchange *getEx(const std::string &name) {
    auto it = g_exchanges.find(name);
    return it == g_exchanges.end() ? nullptr : it->second.get();
}