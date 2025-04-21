#include "util.h"
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>

size_t writeCb(void *p, size_t s, size_t n, void *ud) {
    ((std::string *) ud)->append((char *) p, s * n);
    return s * n;
}

bool parseJsonStr(const std::string &raw, Json::Value &out) {
    Json::CharReaderBuilder b;
    std::string errs;
    std::istringstream is(raw);
    return Json::parseFromStream(b, is, &out, &errs);
}

std::string httpGet(const std::string &url) {
    CURL *c = curl_easy_init();
    if (!c) return {};
    std::string resp;
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 5L);
    curl_easy_perform(c);
    curl_easy_cleanup(c);
    return resp;
}

std::string httpPost(const std::string &url,
                     const std::string &body,
                     const std::string &contentType,
                     struct curl_slist *headers) {
    CURL *c = curl_easy_init();
    if (!c) return {};
    std::string resp;
    struct curl_slist *hdrs = headers;
    if (!hdrs) hdrs = curl_slist_append(nullptr, ("Content-Type: " + contentType).c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 5L);
    curl_easy_perform(c);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    return resp;
}

std::string hmacSha256(const std::string &key, const std::string &data) {
    unsigned char *h = HMAC(EVP_sha256(),
                            key.data(), key.size(),
                            (unsigned char *) data.data(), data.size(),
                            nullptr, nullptr);
    std::ostringstream o;
    for (int i = 0; i < 32; ++i)
        o << std::hex << std::setw(2) << std::setfill('0') << (int) h[i];
    return o.str();
}

void tgSend(const std::string &message) {
    extern std::string g_tgToken, g_tgChat;
    if (g_tgToken.empty()) return;
    Json::Value p;
    p["chat_id"] = g_tgChat;
    p["text"] = message;
    httpPost("https://api.telegram.org/bot" + g_tgToken + "/sendMessage",
             p.toStyledString());
}
