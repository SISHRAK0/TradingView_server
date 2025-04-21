#include "config.h"

std::unordered_map<std::string, ApiCreds> g_creds;
std::string g_tgToken, g_tgChat;

std::unordered_map<std::string, CoinCfg> g_cfg;
std::vector<std::string> g_watch;
std::mutex g_m;
std::vector<std::string> g_signals;