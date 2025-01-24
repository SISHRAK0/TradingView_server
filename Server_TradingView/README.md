
# TradingView Server with Telegram Integration

This project is a cryptocurrency trading bot that allows users to place buy and sell orders on Binance. It also sends notifications about executed orders to a specified Telegram chat. The bot retrieves live cryptocurrency prices and maintains a record of open positions.

## Features

- Buy and sell cryptocurrencies on Binance using market orders.
- Get real-time cryptocurrency prices.
- Monitor open positions.
- Receive notifications of trade execution on Telegram.
- Integrated logging with `spdlog` for tracking system events and errors.

## Requirements

### Dependencies

- **C++17 or later**: Ensure that your system has a C++ compiler that supports C++17.
- **Binance API and WebSocket Library**: `binacpp` is used to interact with the Binance API.
- **spdlog**: A fast C++ logging library.
- **cURL**: Used to send HTTP requests to the Telegram API.
- **jsoncpp**: Used to parse and handle JSON data.

You can install these dependencies via your system's package manager or build them from source:

### Installing dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install libcurl4-openssl-dev libjsoncpp-dev
```

### Installing `spdlog`

```bash
git clone https://github.com/gabime/spdlog.git
cd spdlog
mkdir build && cd build
cmake ..
make -j
sudo make install
```

### Installing `jsoncpp`

```bash
sudo apt-get install libjsoncpp-dev
```

### Binance API and WebSocket Library (`binacpp`)

Download and integrate the `binacpp` library from the official repository: https://github.com/binance-exchange/binacpp-api

## Setup

1. **Clone the project repository**:

    ```bash
    git clone <repository-url>
    cd <repository-folder>
    ```

2. **Compile the project**:

   Make sure all dependencies are installed, then compile the project using your favorite C++ build system (e.g., `g++` or `CMake`).

   Example using `g++`:

    ```bash
    g++ -std=c++17 -o binance_bot main.cpp -lcurl -ljsoncpp -lpthread -lspdlog
    ```

3. **Configure API Keys**:

   You'll need the following credentials:
    - Binance API Key
    - Binance Secret Key
    - Telegram Bot Token (You can create a Telegram bot via [BotFather](https://core.telegram.org/bots#botfather))
    - Telegram Chat ID (This can be retrieved by sending a message to the bot and using a Telegram bot API to get the chat ID)

## How to Run

1. Run the compiled program:

    ```bash
    ./binance_bot
    ```

2. When prompted, input your:
    - Binance API Key
    - Binance Secret Key
    - Telegram Bot Token
    - Telegram Chat ID

3. Once the bot is running, you can use the following commands:

    - `help`: Display available commands.
    - `price <symbol>`: Display the current price of a coin (e.g., `price BTCUSDT`).
    - `buy <symbol> <amount>`: Buy a specific amount of a coin (e.g., `buy BTCUSDT 0.001`).
    - `sell <symbol> <amount>`: Sell a specific amount of a coin (e.g., `sell BTCUSDT 0.001`).
    - `positions`: Show all open positions.
    - `exit`: Exit the application.

## Logging

Logs are stored in the `logs/application.log` file. They track events such as successful orders, price queries, and any errors that occur during execution.

## License

This project is open-source under the MIT License.
