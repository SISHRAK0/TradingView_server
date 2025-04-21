# Crypto Trading Bot Backend

Лёгкий модульный C++17 сервер для сканирования биржевых данных и автоматического выставления ордеров.

## Сборка

```bash
g++ -std=c++17 -O2 \
    src/*.cpp \
    -lcurl -lssl -lcrypto -ljsoncpp -lpthread -lspdlog \
    -o trading_bot
```

## Модули

- **utils**
    - `writeCb`, `parseJsonStr`, `httpGet`, `httpPost`
    - `hmacSha256` — подпись HMAC-SHA256 для приватных API
    - `tgSend` — отправка уведомлений в Telegram

- **config**
    - `ApiCreds` — креды для API
    - `CoinCfg` — конфиг стратегии для каждого символа
    - Глобальные переменные:
        - `g_creds`, `g_tgToken`, `g_tgChat`
        - `g_cfg`, `g_watch`, `g_signals`

- **exchange**
    - `Exchange` (абстрактный базовый класс)
    - Конкретные реализации:
        - `BinanceEx`, `FinandyEx`, `BybitEx`, `OkxEx`
    - Фабрика `getEx(name)`

- **indicators**
    - Скользящие средние (MA/EMA)
    - RSI, MACD, Bollinger Bands (стандартное отклонение)

- **trader**
    - Поток `startTraderThread()`, который в цикле:
        1. Берёт список символов из `g_watch`
        2. Достаёт последние данные (klines + price)
        3. Считает индикаторы
        4. Определяет сигнал и вызывает `order(...)`
        5. Логирует и сохраняет сигнал в `g_signals`

- **server**  
  HTTP‑сервер на [cpp-httplib](https://github.com/yhirose/cpp-httplib).

## HTTP‑API

| Метод | Путь                              | Описание                                       | Запрос (JSON/Query)                                                                                                                                       | Код ответа |
|-------|-----------------------------------|------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------|------------|
| GET   | `/symbols`                        | Получить все отслеживаемые символы            | —                                                                                                                                                         | 200, JSON  |
| POST  | `/symbols`                        | Начать отслеживать новый символ               | `{ "symbol": "BTCUSDT", "ex": "BINANCE", "tf": "1m", "algo": 0, ... }`                                                                                      | 201        |
| GET   | `/price?symbol=<symbol>`          | Текущая цена                                  | query: `symbol`                                                                                                                                           | 200, `{ "price": 12345.67 }` |
| GET   | `/klines?symbol=<>&tf=<>&limit=<>`| K‑линии + индикаторы за последние N свечей    | query: `symbol`, `tf`, `limit`                                                                                                                             | 200, `[[time,close,MA,RSI,BBu,BBl,MACD,Signal], …]` |
| POST  | `/config`                         | Обновить параметры стратегии (algo, периоды…)  | `{ "symbol": "BTCUSDT", "algo": 1, "rsiPeriod": 14, "maPeriod": 50, "oversold": 30, "qty": 0.001 }`                                                         | 204        |
| GET   | `/signals`                        | Лог сигналов (BUY/SELL с меткой времени)      | —                                                                                                                                                         | 200, JSON  |
| POST  | `/settings/keys`                  | Задать API‑ключи для биржи                    | `{ "exchange": "BINANCE", "key": "...", "secret": "..." }`                                                                                                 | 204        |
| POST  | `/settings/telegram`              | Настроить Telegram‑уведомления                | `{ "token": "<bot-token>", "chat_id": "<chat-id>" }`                                                                                                       | 204        |
| POST  | `/settings/finandy/json`          | Загрузить JSON‑шаблон для Finandy webhook     | Любой JSON, далее передаётся в FinandyEx::order через `setJson(j)`                                                                                         | 204        |
```