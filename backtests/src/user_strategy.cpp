// Include necessary headers
#include <vector>

extern "C" int signal(const std::vector<double>& prices) {
    // Return 1 to BUY, 0 to SELL, -1 to HOLD
    if (prices.size() >= 20) {
        double sum = 0;
        for (size_t i = prices.size()-20; i < prices.size(); ++i) sum += prices[i];
        double ma20 = sum / 20;
        return prices.back() > ma20 ? 1 : 0;
    }
    return -1;
}
