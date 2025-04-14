#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <curl/curl.h>

// JsonCpp для работы с JSON
#include "json/json.h"
// spdlog для логирования
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// ANSI-коды для цветного вывода в терминале
#define RESET   "\033[0m"
#define BOLDRED "\033[1m\033[31m"
#define BOLDGREEN "\033[1m\033[32m"
#define BOLDYELLOW "\033[1m\033[33m"

// ================================
// Глобальные переменные и константы
// ================================
std::string BINANCE_API_KEY;    // не используются для публичных запросов
std::string BINANCE_SECRET_KEY;
std::string TELEGRAM_BOT_TOKEN;
std::string TELEGRAM_CHAT_ID;

// URL для отправки запроса на Finandy
const std::string FINANDY_URL = "https://hook.finandy.com/wwsYwdfipGXpqCE0rlUK";

// Список монет (Futures), за которыми следим
std::vector<std::string> userCoins;
std::mutex coins_mutex;

// Параметры по умолчанию для торговых алгоритмов
const int DEFAULT_RSI_PERIOD = 14;
const int DEFAULT_MA_PERIOD = 50;
const double DEFAULT_RSI_OVERSOLD = 30.0;
const double DEFAULT_RSI_OVERBOUGHT = 70.0;

// Структура конфигурации для монеты
struct CoinConfig {
    std::string timeframe = "1m"; // интервал для исторических данных (например, "1m", "5m", "15m", "1h")
    int rsiPeriod = DEFAULT_RSI_PERIOD;
    int maPeriod = DEFAULT_MA_PERIOD;
    double rsiOversold = DEFAULT_RSI_OVERSOLD;
    double rsiOverbought = DEFAULT_RSI_OVERBOUGHT;
    int algorithm = 0; // 0 - RSI+MA, 1 - только RSI
};

// Хранение конфигураций для монет (по символу)
std::unordered_map<std::string, CoinConfig> coinConfigs;

// Для хранения "открытых позиций" (пример простейшего хранения)
std::unordered_map<std::string, std::string> positions;

// ================================
// Функция отправки сообщений в Telegram
// ================================
void sendTelegramMessage(const std::string &message) {
    CURL *curl = curl_easy_init();
    if(!curl) return;

    std::string url = "https://api.telegram.org/bot" + TELEGRAM_BOT_TOKEN + "/sendMessage";
    Json::Value payload;
    payload["chat_id"] = TELEGRAM_CHAT_ID;
    payload["text"] = message;
    payload["parse_mode"] = "HTML";
    std::string post_fields = payload.toStyledString();

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        spdlog::error("Ошибка отправки сообщения в Telegram: {}", curl_easy_strerror(res));
    } else {
        spdlog::info("Сообщение отправлено в Telegram: {}", message);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// ================================
// Общая callback-функция для libcurl
// ================================
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ================================
// Запрос GET к URL с использованием libcurl
// ================================
std::string httpGet(const std::string &url) {
    CURL *curl = curl_easy_init();
    if(!curl) return "";

    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        spdlog::error("Ошибка запроса {}: {}", url, curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
    return readBuffer;
}

// ================================
// Получение текущей цены (ticker) для символа с Binance Futures
// ================================
double getFuturesPrice(const std::string &symbol) {
    std::string url = "https://fapi.binance.com/fapi/v1/ticker/price?symbol=" + symbol;
    std::string response = httpGet(url);
    if(response.empty()) {
        spdlog::error("Пустой ответ при получении цены для {}", symbol);
        return 0.0;
    }
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream iss(response);
    if(!Json::parseFromStream(builder, iss, &root, &errs)) {
        spdlog::error("Ошибка парсинга JSON цены для {}: {}", symbol, errs);
        return 0.0;
    }
    try {
        double price = std::stod(root["price"].asString());
        spdlog::info("Текущая цена {}: {:.6f}", symbol, price);
        return price;
    } catch(...) {
        spdlog::error("Ошибка преобразования цены для {}", symbol);
        return 0.0;
    }
}

// ================================
// Получение исторических цен (закрытия свечей) для символа с Binance Futures
// ================================
std::vector<double> getHistoricalPrices(const std::string &symbol, const std::string &interval, int limit) {
    std::vector<double> closingPrices;
    std::string url = "https://fapi.binance.com/fapi/v1/klines?symbol=" + symbol +
                      "&interval=" + interval + "&limit=" + std::to_string(limit);
    std::string response = httpGet(url);
    if(response.empty()) {
        spdlog::error("Пустой ответ при получении исторических данных для {}", symbol);
        return closingPrices;
    }
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream iss(response);
    if(!Json::parseFromStream(builder, iss, &root, &errs)) {
        spdlog::error("Ошибка парсинга исторических данных для {}: {}", symbol, errs);
        return closingPrices;
    }
    // Каждый элемент массива соответствует свечке.
    // Считаем, что элемент с индексом 4 – цена закрытия
    for (const auto& candle : root) {
        if(candle.isArray() && candle.size() > 4) {
            try {
                double closePrice = std::stod(candle[4].asString());
                closingPrices.push_back(closePrice);
            } catch(...) {
                spdlog::error("Ошибка преобразования цены закрытия для {}", symbol);
            }
        }
    }
    return closingPrices;
}

// ================================
// Функция отправки запроса ордера на Finandy
// ================================
void sendOrderToFinandy(const std::string &side, const std::string &positionSide, const std::string &symbol) {
    CURL *curl = curl_easy_init();
    if(!curl) return;
    Json::Value payload;
    payload["name"] = "Hook 287115";
    payload["secret"] = "qyo3fp9spxm";
    payload["side"] = side;
    payload["positionSide"] = positionSide;
    payload["symbol"] = symbol;

    payload["open"]["enabled"] = true;
    payload["open"]["amountType"] = "sumUsd";
    payload["open"]["amount"] = "100";
    payload["open"]["leverage"] = "6";
    payload["open"]["price"] = "";

    payload["sl"]["enabled"] = true;
    payload["sl"]["ofsMode"] = "pos";
    payload["sl"]["ofs"] = 2;
    payload["sl"]["orderType"] = "stop-market";

    payload["slx"]["enabled"] = true;
    payload["slx"]["mode"] = "tp";
    payload["slx"]["tpNum"] = 1;
    payload["slx"]["always"] = true;
    payload["slx"]["orderType"] = "stop-market";
    payload["slx"]["checkProfit"] = true;

    payload["tp"]["enabled"] = true;
    payload["tp"]["orderType"] = "limit";
    payload["tp"]["adjust"] = true;

    payload["close"]["upend"]["type"] = "openAmount";
    payload["close"]["upend"]["amount"] = "";
    payload["close"]["enabled"] = true;
    payload["close"]["action"] = "upend";

    payload["dca"]["enabled"] = true;
    payload["dca"]["amountType"] = "sumUsd";
    payload["dca"]["amount"] = "100";

    std::string post_fields = payload.toStyledString();

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, FINANDY_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        spdlog::error("Ошибка отправки запроса в Finandy: {}", curl_easy_strerror(res));
    } else {
        spdlog::info("Запрос успешно отправлен в Finandy: {}", post_fields);
        sendTelegramMessage("Запрос в Finandy: " + post_fields);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// ================================
// Расчёт скользящей средней (SMA)
// ================================
double computeMA(const std::vector<double>& prices, int period) {
    if(prices.size() < static_cast<size_t>(period)) return 0.0;
    double sum = 0.0;
    for (size_t i = prices.size() - period; i < prices.size(); ++i)
        sum += prices[i];
    return sum / period;
}

// ================================
// Расчёт индекса относительной силы (RSI)
// ================================
double computeRSI(const std::vector<double>& prices, int period) {
    if(prices.size() <= static_cast<size_t>(period)) return 0.0;
    double gain = 0.0, loss = 0.0;
    for (size_t i = prices.size() - period; i < prices.size() - 1; ++i) {
        double change = prices[i+1] - prices[i];
        if(change > 0)
            gain += change;
        else
            loss -= change;
    }
    double avgGain = gain / period;
    double avgLoss = loss / period;
    if(avgLoss == 0.0) return 100.0;
    double rs = avgGain / avgLoss;
    return 100.0 - (100.0 / (1 + rs));
}

// ================================
// Оценка торгового сигнала для монеты с учетом её конфигурации
// ================================
void evaluateTradingSignal(const std::string &symbol) {
    // Получаем конфигурацию для монеты (если не задана, берём значение по умолчанию)
    CoinConfig config;
    if(coinConfigs.find(symbol) != coinConfigs.end())
        config = coinConfigs[symbol];

    int historyLimit = std::max(config.rsiPeriod, config.maPeriod) + 1;
    auto prices = getHistoricalPrices(symbol, config.timeframe, historyLimit);
    if(prices.size() < static_cast<size_t>(historyLimit)) {
        spdlog::error("Недостаточно данных для анализа {}", symbol);
        return;
    }
    double currentPrice = getFuturesPrice(symbol);
    double ma = computeMA(prices, config.maPeriod);
    double rsi = computeRSI(prices, config.rsiPeriod);

    spdlog::info("Анализ {}: Цена = {:.6f}, MA = {:.6f}, RSI = {:.2f}", symbol, currentPrice, ma, rsi);
    std::cout << BOLDYELLOW << "Анализ " << symbol
              << ": Цена = " << currentPrice << ", MA = " << ma << ", RSI = "
              << rsi << RESET << std::endl;

    // Выбор алгоритма
    if(config.algorithm == 0) { // Алгоритм 0: RSI + MA
        if(rsi < config.rsiOversold && currentPrice > ma) {
            spdlog::info("Сигнал к покупке для {}", symbol);
            std::cout << BOLDGREEN << "Сигнал к покупке для " << symbol << RESET << std::endl;
            sendOrderToFinandy("BUY", "LONG", symbol);
            positions[symbol] = "LONG";
        } else if(rsi > config.rsiOverbought && currentPrice < ma) {
            spdlog::info("Сигнал к продаже для {}", symbol);
            std::cout << BOLDRED << "Сигнал к продаже для " << symbol << RESET << std::endl;
            sendOrderToFinandy("SELL", "SHORT", symbol);
            positions[symbol] = "SHORT";
        } else {
            spdlog::info("Нет подходящего сигнала для {}", symbol);
        }
    } else if(config.algorithm == 1) { // Алгоритм 1: только RSI
        if(rsi < config.rsiOversold) {
            spdlog::info("Сигнал к покупке (RSI) для {}", symbol);
            std::cout << BOLDGREEN << "Сигнал к покупке для " << symbol << RESET << std::endl;
            sendOrderToFinandy("BUY", "LONG", symbol);
            positions[symbol] = "LONG";
        } else if(rsi > config.rsiOverbought) {
            spdlog::info("Сигнал к продаже (RSI) для {}", symbol);
            std::cout << BOLDRED << "Сигнал к продаже для " << symbol << RESET << std::endl;
            sendOrderToFinandy("SELL", "SHORT", symbol);
            positions[symbol] = "SHORT";
        } else {
            spdlog::info("Нет подходящего сигнала для {}", symbol);
        }
    }
}

// ================================
// Торговая логика, работающая в отдельном потоке
// ================================
void tradingLogic() {
    while(true) {
        {
            std::lock_guard<std::mutex> lock(coins_mutex);
            for(const auto &symbol : userCoins) {
                evaluateTradingSignal(symbol);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

// ================================
// Вывод справки (меню команд) в терминале
// ================================
void display_help() {
    std::cout << "\nДоступные команды:\n"
              << "  help                                  - Показать справку\n"
              << "  price <symbol>                        - Показать текущую цену монеты (например, BTCUSDT)\n"
              << "  add <symbol>                          - Добавить монету в список отслеживания\n"
              << "  list                                  - Показать список монет\n"
              << "  positions                           - Показать открытые позиции\n"
              << "  config <symbol> <param> <value>       - Настроить монету\n"
              << "       параметры: timeframe, algorithm, rsi, ma, oversold, overbought\n"
              << "  exit                                  - Выйти из программы\n"
              << std::endl;
}

// ================================
// Обработка пользовательских команд из терминала
// ================================
void process_command(const std::string &command_line) {
    std::istringstream iss(command_line);
    std::string command;
    iss >> command;

    if(command == "help") {
        display_help();
    } else if(command == "price") {
        std::string symbol;
        iss >> symbol;
        if(!symbol.empty()) {
            double price = getFuturesPrice(symbol);
            std::cout << "Текущая цена " << symbol << ": " << price << std::endl;
        } else {
            std::cout << BOLDRED << "Укажите символ. Пример: price BTCUSDT" << RESET << std::endl;
        }
    } else if(command == "add") {
        std::string symbol;
        iss >> symbol;
        if(!symbol.empty()) {
            std::lock_guard<std::mutex> lock(coins_mutex);
            userCoins.push_back(symbol);
            // Если для монеты ещё не задана конфигурация, добавляем значение по умолчанию
            if(coinConfigs.find(symbol) == coinConfigs.end())
                coinConfigs[symbol] = CoinConfig();
            std::cout << BOLDGREEN << "Добавлена монета: " << symbol << RESET << std::endl;
        } else {
            std::cout << BOLDRED << "Укажите символ. Пример: add BTCUSDT" << RESET << std::endl;
        }
    } else if(command == "list") {
        std::cout << "Монеты для отслеживания:" << std::endl;
        std::lock_guard<std::mutex> lock(coins_mutex);
        for(const auto &symbol : userCoins)
            std::cout << "  - " << symbol << std::endl;
    } else if(command == "positions") {
        std::cout << "Открытые позиции:" << std::endl;
        for(const auto &pos : positions)
            std::cout << "  " << pos.first << " : " << pos.second << std::endl;
    } else if(command == "config") {
        // Формат: config <symbol> <param> <value>
        std::string symbol, param, value;
        iss >> symbol >> param >> value;
        if(symbol.empty() || param.empty() || value.empty()) {
            std::cout << BOLDRED << "Неверный формат. Пример: config BTCUSDT timeframe 5m" << RESET << std::endl;
            return;
        }
        // Если конфигурация для монеты не существует, создаём её
        if(coinConfigs.find(symbol) == coinConfigs.end())
            coinConfigs[symbol] = CoinConfig();
        CoinConfig &config = coinConfigs[symbol];
        if(param == "timeframe") {
            config.timeframe = value;
            std::cout << "Установлен таймфрейм для " << symbol << " на " << value << std::endl;
        } else if(param == "algorithm") {
            config.algorithm = std::stoi(value);
            std::cout << "Установлен алгоритм " << config.algorithm << " для " << symbol << std::endl;
        } else if(param == "rsi") {
            config.rsiPeriod = std::stoi(value);
            std::cout << "Установлен период RSI для " << symbol << " на " << config.rsiPeriod << std::endl;
        } else if(param == "ma") {
            config.maPeriod = std::stoi(value);
            std::cout << "Установлен период MA для " << symbol << " на " << config.maPeriod << std::endl;
        } else if(param == "oversold") {
            config.rsiOversold = std::stod(value);
            std::cout << "Установлен порог перепроданности для " << symbol << " на " << config.rsiOversold << std::endl;
        } else if(param == "overbought") {
            config.rsiOverbought = std::stod(value);
            std::cout << "Установлен порог перекупленности для " << symbol << " на " << config.rsiOverbought << std::endl;
        } else {
            std::cout << BOLDRED << "Неизвестный параметр " << param << ". Попробуйте timeframe, algorithm, rsi, ma, oversold или overbought." << RESET << std::endl;
        }
    } else if(command == "exit") {
        std::cout << "Выход из программы..." << std::endl;
        exit(0);
    } else {
        std::cout << BOLDRED << "Неизвестная команда. Введите 'help' для справки." << RESET << std::endl;
    }
}

// ================================
// Главная функция
// ================================
int main() {
    // Логирование: вывод в файл и консоль
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/application.log", true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
    spdlog::info("Запуск программы...");

    // Запрос API-ключей (если есть) и данных для Telegram
    std::cout << "Введите Binance API Key (если есть, иначе Enter): ";
    std::getline(std::cin, BINANCE_API_KEY);
    std::cout << "Введите Binance Secret Key (если есть, иначе Enter): ";
    std::getline(std::cin, BINANCE_SECRET_KEY);
    std::cout << "Введите Telegram Bot Token: ";
    std::getline(std::cin, TELEGRAM_BOT_TOKEN);
    std::cout << "Введите Telegram Chat ID: ";
    std::getline(std::cin, TELEGRAM_CHAT_ID);

    // Инициализация libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Запускаем торговую логику в отдельном потоке
    std::thread tradingThread(tradingLogic);

    // Основной цикл команд в терминале
    display_help();
    std::string line;
    while(true) {
        std::cout << "\n> ";
        std::getline(std::cin, line);
        if(!line.empty())
            process_command(line);
    }

    tradingThread.join();
    curl_global_cleanup();
    return 0;
}
