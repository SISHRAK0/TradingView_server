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
#include <curl/curl.h>

#include "json/json.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "binacpp/src/binacpp.h"
#include "binacpp/src/binacpp_websocket.h"

using namespace std;


std::string BINANCE_API_KEY;
std::string BINANCE_SECRET_KEY;

std::string TELEGRAM_BOT_TOKEN;
std::string TELEGRAM_CHAT_ID;

std::mutex position_mutex;
std::unordered_map<std::string, Json::Value> positions;
std::unordered_map<std::string, std::chrono::system_clock::time_point> last_trade_times;

const double INITIAL_BALANCE = 15.0;  // USD
const int LEVERAGE = 10;
const double EFFECTIVE_BALANCE = INITIAL_BALANCE * LEVERAGE;
const double INITIAL_STOP_LOSS_PERCENTAGE = 0.004;  // 0.4%
const double PROFIT_THRESHOLD = 0.01;               // 1%
const double TRAILING_STOP_LOSS_PERCENTAGE = 0.005; // 0.5%
const int MIN_TRADE_INTERVAL_SECONDS = 5;
const double MAX_LOSS_PER_TRADE = 0.02;  // 2%

void send_telegram_message(const std::string &message) {
    CURL *curl;
    CURLcode res;

    std::string url = "https://api.telegram.org/bot" + TELEGRAM_BOT_TOKEN + "/sendMessage";

    curl = curl_easy_init();
    if (curl) {
        Json::Value payload;
        payload["chat_id"] = TELEGRAM_CHAT_ID;
        payload["text"] = message;
        payload["parse_mode"] = "HTML";

        std::string post_fields = payload.toStyledString();

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            spdlog::error("Не удалось отправить сообщение в Telegram: {}", curl_easy_strerror(res));
        } else {
            spdlog::info("Сообщение отправлено в Telegram: {}", message);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

Json::Value send_order_to_binance(const std::string &symbol, const std::string &side, double quantity) {
    spdlog::info("Отправка ордера на Binance: символ={}, сторона={}, количество={}", symbol, side, quantity);

    Json::Value result;
    try {
        BinaCPP::init(BINANCE_API_KEY, BINANCE_SECRET_KEY);

        BinaCPP::send_order(
                symbol.c_str(),
                side.c_str(),
                "MARKET",
                "GTC",
                quantity,
                0.0,
                "",
                0.0,
                0.0,
                5000,
                result
        );

        spdlog::info("Ордер успешно размещен: {}", result.toStyledString());
        send_telegram_message("Ордер отправлен на Binance: " + result.toStyledString());
    }
    catch (const std::exception &e) {
        spdlog::error("Ошибка при отправке ордера на Binance: {}", e.what());
        send_telegram_message("Не удалось отправить ордер на Binance: " + std::string(e.what()));
    }

    return result;
}

void display_coin_price(const std::string &symbol) {
    double price = 0.0;
    try {
        price = BinaCPP::get_price(symbol.c_str());
        if (price != 0.0) {
            spdlog::info("Курс {}: {:.6f}", symbol, price);
        } else {
            spdlog::error("Не удалось получить курс для {}", symbol);
        }
    }
    catch (const std::exception &e) {
        spdlog::error("Ошибка при получении курса для {}: {}", symbol, e.what());
    }
}

void display_help() {
    std::cout << "Доступные команды:\n";
    std::cout << "  help                  Показать эту справку\n";
    std::cout << "  price <symbol>        Показать текущий курс монеты\n";
    std::cout << "  buy <symbol> <amount> Купить указанное количество монеты\n";
    std::cout << "  sell <symbol> <amount> Продать указанное количество монеты\n";
    std::cout << "  positions             Показать открытые позиции\n";
    std::cout << "  exit                  Выйти из приложения\n";
}

void display_positions() {
    std::lock_guard<std::mutex> lock(position_mutex);
    if (positions.empty()) {
        std::cout << "Нет открытых позиций.\n";
        return;
    }
    for (const auto &pair: positions) {
        std::cout << "Символ: " << pair.first << "\n";
        std::cout << "Данные позиции: " << pair.second.toStyledString() << "\n";
    }
}

void process_command(const std::string &command_line) {
    std::istringstream iss(command_line);
    std::string command;
    iss >> command;

    if (command == "help") {
        display_help();
    } else if (command == "price") {
        std::string symbol;
        iss >> symbol;
        if (!symbol.empty()) {
            display_coin_price(symbol);
        } else {
            spdlog::error("Укажите символ монеты. Пример: price BTCUSDT");
        }
    } else if (command == "buy" || command == "sell") {
        std::string symbol;
        double amount = 0.0;
        iss >> symbol >> amount;
        if (!symbol.empty() && amount > 0.0) {
            std::string side = (command == "buy") ? "BUY" : "SELL";
            Json::Value response = send_order_to_binance(symbol, side, amount);
            {
                std::lock_guard<std::mutex> lock(position_mutex);
                positions[symbol] = response;
            }
        } else {
            spdlog::error("Укажите символ монеты и количество. Пример: buy BTCUSDT 0.001");
        }
    } else if (command == "positions") {
        display_positions();
    } else if (command == "exit") {
        std::cout << "Выход из приложения...\n";
        exit(0);
    } else {
        spdlog::error("Неизвестная команда: {}", command);
        std::cout << "Введите 'help' для списка доступных команд.\n";
    }
}

int main() {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/application.log", true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    spdlog::info("Запуск программы...");

    std::cout << "Введите Binance API Key: ";
    std::cin >> BINANCE_API_KEY;
    std::cout << "Введите Binance Secret Key: ";
    std::cin >> BINANCE_SECRET_KEY;
    std::cout << "Введите Токен Telegram бота: ";
    std::cin >> TELEGRAM_BOT_TOKEN;
    std::cout << "Введите Чат ID: ";
    std::cin >> TELEGRAM_CHAT_ID;
    BinaCPP::init(BINANCE_API_KEY, BINANCE_SECRET_KEY);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::cin.ignore();

    std::cout << "Введите 'help' для списка доступных команд.\n";

    std::string command_line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command_line);
        if (!command_line.empty()) {
            process_command(command_line);
        }
    }

}
