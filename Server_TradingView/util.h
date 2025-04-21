#ifndef TRADING_BOT_UTIL_H
#define TRADING_BOT_UTIL_H
#pragma once

#include <string>
#include <json/json.h>

// CURL callback
size_t writeCb(void *p, size_t s, size_t n, void *ud);

bool parseJsonStr(const std::string &raw, Json::Value &out);

// HTTP GET / POST (JSON или x-www-form-urlencoded)
std::string httpGet(const std::string &url);

std::string httpPost(const std::string &url,
                     const std::string &body,
                     const std::string &contentType = "application/json",
                     struct curl_slist *headers = nullptr);

// HMAC‑SHA256
std::string hmacSha256(const std::string &key, const std::string &data);

void tgSend(const std::string &message);


#endif //TRADING_BOT_UTIL_H
