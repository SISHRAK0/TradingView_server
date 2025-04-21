#include "finandy_ex.h"
#include "util.h"
#include <spdlog/spdlog.h>

void FinandyEx::setJson(const Json::Value &j) {
    tmpl_ = j;
}

bool FinandyEx::order(const std::string &side,
                      const std::string &pos,
                      const std::string &sym,
                      double qty) {
    Json::Value p = tmpl_;
    p["side"] = side;
    p["positionSide"] = pos;
    p["symbol"] = sym;
    p["quantity"] = qty;
    auto r = httpPost(
            "https://hook.finandy.com/wwsYwdfipGXpqCE0rlUK",
            p.toStyledString()
    );
    spdlog::info("Finandy response: {}", r);
    tgSend(r);
    return !r.empty();
}
