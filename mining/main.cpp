#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <json/json.h>

// ====== CONFIGURATION ======
const std::string POOL_HOST = "pool.example.com";
const int POOL_PORT = 3333;
const std::string USERNAME = "username.worker";
const std::string PASSWORD = "password";
// ============================

std::mutex job_mutex;
struct Job {
    std::string job_id;
    std::string prev_hash;
    std::string coinb1;
    std::string coinb2;
    std::vector<std::string> merkle_branch;
    uint32_t version;
    uint32_t bits;
    uint32_t ntime;
} current_job;
bool has_job = false;

// Helper: hex string to bytes
std::vector<unsigned char> hexToBytes(const std::string &hex) {
    std::vector<unsigned char> bytes;
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        unsigned int byte;
        std::stringstream ss(hex.substr(i, 2));
        ss >> std::hex >> byte;
        bytes.push_back(static_cast<unsigned char>(byte));
    }
    return bytes;
}

// Helper: bytes to hex string
std::string bytesToHex(const unsigned char* bytes, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << (int)bytes[i];
    }
    return ss.str();
}

// Double SHA256
std::vector<unsigned char> doubleSHA256(const std::vector<unsigned char> &data) {
    unsigned char hash1[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash1);
    unsigned char hash2[SHA256_DIGEST_LENGTH];
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    return {hash2, hash2 + SHA256_DIGEST_LENGTH};
}

// Convert compact bits to target
__uint128_t bitsToTarget(uint32_t bits) {
    uint32_t exp = bits >> 24;
    uint32_t mant = bits & 0x007fffff;
    __uint128_t target = mant;
    int shift = 8 * (exp - 3);
    target <<= shift;
    return target;
}

// Build block header
std::vector<unsigned char> buildHeader(const Job &job, uint32_t nonce, const std::string &ex1, const std::string &ex2) {
    // Coinbase
    std::vector<unsigned char> coinbase;
    auto b1 = hexToBytes(job.coinb1);
    auto b2 = hexToBytes(job.coinb2);
    auto e1 = hexToBytes(ex1);
    auto e2 = hexToBytes(ex2);
    coinbase.insert(coinbase.end(), b1.begin(), b1.end());
    coinbase.insert(coinbase.end(), e1.begin(), e1.end());
    coinbase.insert(coinbase.end(), e2.begin(), e2.end());
    coinbase.insert(coinbase.end(), b2.begin(), b2.end());

    auto coin_hash = doubleSHA256(coinbase);

    // Merkle root
    std::vector<unsigned char> merkle = coin_hash;
    for (auto &h : job.merkle_branch) {
        auto bh = hexToBytes(h);
        std::vector<unsigned char> concat(merkle);
        concat.insert(concat.end(), bh.rbegin(), bh.rend());
        merkle = doubleSHA256(concat);
    }
    // Reverse for little-endian
    std::reverse(merkle.begin(), merkle.end());

    // Header buffer
    std::vector<unsigned char> header;
    // version
    header.resize(4);
    *reinterpret_cast<uint32_t*>(header.data()) = job.version;
    // prev_hash LE
    auto prev = hexToBytes(job.prev_hash);
    header.insert(header.end(), prev.rbegin(), prev.rend());
    // merkle root
    header.insert(header.end(), merkle.begin(), merkle.end());
    // time
    uint32_t t = job.ntime;
    header.insert(header.end(), reinterpret_cast<unsigned char*>(&t), reinterpret_cast<unsigned char*>(&t) + 4);
    // bits
    header.insert(header.end(), reinterpret_cast<unsigned char*>(job.bits), reinterpret_cast<unsigned char*>(job.bits) + 4);
    // nonce
    header.insert(header.end(), reinterpret_cast<unsigned char*>(&nonce), reinterpret_cast<unsigned char*>(&nonce) + 4);

    return header;
}

// TCP send
void sendJson(int sock, const Json::Value &j) {
    Json::StreamWriterBuilder w;
    std::string out = Json::writeString(w, j) + "\n";
    send(sock, out.c_str(), out.size(), 0);
}

// Stratum handler
void stratumThread() {
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(POOL_HOST.c_str(), std::to_string(POOL_PORT).c_str(), &hints, &res);
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(sock, res->ai_addr, res->ai_addrlen);

    int id = 1;
    // Subscribe
    Json::Value sub;
    sub["id"] = id++;
    sub["method"] = "mining.subscribe";
    sub["params"] = Json::Value(Json::arrayValue);
    sendJson(sock, sub);

    // Authorize
    Json::Value auth;
    auth["id"] = id++;
    auth["method"] = "mining.authorize";
    auth["params"] = Json::arrayValue;
    auth["params"].append(USERNAME);
    auth["params"].append(PASSWORD);
    sendJson(sock, auth);

    char buf[1024];
    std::string extranonce1;
    int extranonce2_size;
    while (true) {
        int len = recv(sock, buf, sizeof(buf)-1, 0);
        if (len <= 0) break;
        buf[len] = '\0';
        std::istringstream ss(buf);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            Json::CharReaderBuilder r;
            Json::Value resp;
            std::string errs;
            Json::parseFromStream(r, ss, &resp, &errs);

            if (resp.isMember("result") && resp["result"].isArray()) {
                extranonce1 = resp["result"][1].asString();
                extranonce2_size = resp["result"][2].asInt();
                std::cout << "Subscribed: ex1=" << extranonce1 << " size=" << extranonce2_size << std::endl;
            }
            if (resp.isMember("method") && resp["method"].asString() == "mining.notify") {
                auto &p = resp["params"];
                Job j;
                j.job_id = p[0].asString();
                j.prev_hash = p[1].asString();
                j.coinb1 = p[2].asString();
                j.coinb2 = p[3].asString();
                for (auto &m : p[4]) j.merkle_branch.push_back(m.asString());
                j.version = std::stoul(p[5].asString(), nullptr, 16);
                j.bits = std::stoul(p[6].asString(), nullptr, 16);
                j.ntime = std::stoul(p[7].asString(), nullptr, 16);
                {
                    std::lock_guard<std::mutex> lock(job_mutex);
                    current_job = j;
                    has_job = true;
                }
                std::cout << "New job: " << j.job_id << std::endl;
            }
        }
    }
    close(sock);
}

// Miner thread
void minerThread() {
    while (true) {
        if (!has_job) { std::this_thread::sleep_for(std::chrono::seconds(1)); continue; }
        Job job;
        std::string ex1;
        int ex2_size;
        {
            std::lock_guard<std::mutex> lock(job_mutex);
            job = current_job;
        }
        __uint128_t target = bitsToTarget(job.bits);

        while (has_job) {
            // generate extranonce2
            uint64_t tstamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            std::stringstream ss;
            ss << std::hex << tstamp;
            std::string ex2 = ss.str().substr(0, ex2_size*2);

            for (uint32_t nonce = 0; nonce < 0xFFFFFFFF; ++nonce) {
                auto header = buildHeader(job, nonce, ex1, ex2);
                auto hash = doubleSHA256(header);
                // big-endian int compare
                __uint128_t hval = 0;
                for (int i = 0; i < 16; ++i) hval = (hval << 8) | hash[i];
                if (hval < target) {
                    std::cout << "Share found! nonce=" << nonce << std::endl;
                    // submit share
                    struct addrinfo hints{}, *res;
                    hints.ai_family = AF_INET;
                    hints.ai_socktype = SOCK_STREAM;
                    getaddrinfo(POOL_HOST.c_str(), std::to_string(POOL_PORT).c_str(), &hints, &res);
                    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                    connect(sock, res->ai_addr, res->ai_addrlen);

                    Json::Value subm;
                    subm["id"] = 100;
                    subm["method"] = "mining.submit";
                    subm["params"] = Json::arrayValue;
                    subm["params"].append(USERNAME);
                    subm["params"].append(job.job_id);
                    subm["params"].append(ex1);
                    subm["params"].append(ex2);
                    std::stringstream ssTime;
                    ssTime << std::hex << job.ntime;
                    subm["params"].append("0x" + ssTime.str());

                    std::stringstream ssNonce;
                    ssNonce << std::hex << nonce;
                    subm["params"].append("0x" + ssNonce.str());
                    sendJson(sock, subm);
                    close(sock);
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

int main() {
    std::thread st(stratumThread);
    std::thread mt(minerThread);
    st.join();
    mt.join();
    return 0;
}
