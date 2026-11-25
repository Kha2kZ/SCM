#define SDL_MAIN_HANDLED
#include <iostream>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <windows.h>
#include <conio.h>

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
#define COLOR_MAGENTA "\033[35m"

const std::string SPEED_TOKEN = "tk_756e6e696d697465647370656564";
const std::string HACKER_TOKEN = "KHAI_HACKER_ADMIN_2026";
const std::string POOL_TOKEN = "tk_706f6f6c6d696e696e67";
const long long DEFAULT_CHUNK_STEP = 10000000000LL;

struct AppPaths {
    std::string baseDir;
    std::string modsDir;
    std::string saveFile;
    std::string speedMod;
    std::string hackerMod;
    std::string poolMod;
};

struct PoolJob {
    bool valid = false;
    bool unauthorized = false;
    std::string message = "";
    long long workerId = 0;
    long long chunkMin = 0;
    long long chunkMax = DEFAULT_CHUNK_STEP;
    int blockIndex = 1;
    int difficulty = 3;
    int workEpoch = 1;
    std::string previousHash = "";
    double unclaimedPool = 0.0;
};

inline void rawPrint(const std::string& text) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE && !text.empty()) {
        DWORD written = 0;
        WriteConsoleA(hOut, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
    }
}

inline void flushOut() {
    std::cout.flush();
    std::fflush(stdout);
}

inline void discardLine() {
    std::cin.clear();
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
}

inline void waitEnter(const std::string& msg = "Press [ENTER] to return to menu...") {
    std::cout << msg;
    flushOut();
    std::cin.clear();
    std::string line;
    std::getline(std::cin, line);
}

std::string getEnvWindows(const char* name) {
    DWORD needed = GetEnvironmentVariableA(name, nullptr, 0);
    if (needed == 0) return "";
    std::string buf(needed, '\0');
    if (GetEnvironmentVariableA(name, &buf[0], needed) == 0) return "";
    if (!buf.empty() && buf.back() == '\0') buf.pop_back();
    return buf;
}

AppPaths getAppPaths() {
    std::string profileDir = getEnvWindows("USERPROFILE");
    if (profileDir.empty()) {
        profileDir = getEnvWindows("HOMEDRIVE") + getEnvWindows("HOMEPATH");
    }
    AppPaths p;
    p.baseDir = profileDir + "\\Desktop\\crypto_miner";
    p.modsDir = p.baseDir + "\\mods";
    p.saveFile = p.baseDir + "\\financial_report.json";
    p.speedMod = p.modsDir + "\\unlimited_speed.mod";
    p.hackerMod = p.modsDir + "\\hacker.mod";
    p.poolMod = p.modsDir + "\\pool_mining.mod";
    return p;
}

bool verifyExeInBaseDir(const std::string& baseDir) {
    char exeBuf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return false;
    try {
        auto exePath = std::filesystem::weakly_canonical(std::filesystem::path(exeBuf));
        auto dirPath = std::filesystem::weakly_canonical(std::filesystem::path(baseDir));
        return exePath.parent_path() == dirPath;
    } catch (...) {
        return false;
    }
}

void enableAnsi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

void clearScreen() {
    flushOut();
    system("cls");
    flushOut();
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
    localtime_s(&tmBuf, &t);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%H:%M:%S");
    return oss.str();
}

inline std::string trimClean(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '\"'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '\''), s.end());
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

inline int toInt(const std::string& s, int defVal = 0) {
    std::string c = trimClean(s);
    if (c.empty()) return defVal;
    try { return std::stoi(c); } catch (...) { return defVal; }
}

inline long long toLL(const std::string& s, long long defVal = 0) {
    std::string c = trimClean(s);
    if (c.empty()) return defVal;
    try { return std::stoll(c); } catch (...) { return defVal; }
}

inline unsigned long long toULL(const std::string& s, unsigned long long defVal = 0) {
    std::string c = trimClean(s);
    if (c.empty()) return defVal;
    try { return std::stoull(c); } catch (...) { return defVal; }
}

inline double toDouble(const std::string& s, double defVal = 0.0) {
    std::string c = trimClean(s);
    if (c.empty()) return defVal;
    try { return std::stod(c); } catch (...) { return defVal; }
}

std::string runCmd(const std::string& cmd) {
    std::string out;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        out += buf;
    }
    _pclose(pipe);
    return out;
}

std::string httpGet(const std::string& url, int timeoutSec = 6) {
    return runCmd("curl.exe -s -L -m " + std::to_string(timeoutSec) + " \"" + url + "\"");
}

std::string httpPostJson(const std::string& url, const std::string& worker, const std::string& token, int timeoutSec = 8) {
    std::string body = "{\"worker\":\"" + worker + "\",\"token\":\"" + token + "\"}";
    std::string escapedBody;
    escapedBody.reserve(body.size() * 2);
    for (char c : body) {
        if (c == '"') escapedBody += "\\\"";
        else escapedBody += c;
    }
    return runCmd("curl.exe -s -L -m " + std::to_string(timeoutSec)
                  + " -X POST -H \"Content-Type: application/json\" -d \""
                  + escapedBody + "\" \"" + url + "\"");
}

std::string httpPostSubmit(const std::string& url, const std::string& worker, const std::string& token, int blockIdx, long long nonce, double durationSec, int timeoutSec = 8) {
    std::string body = "{\"worker\":\"" + worker + "\",\"token\":\"" + token
                     + "\",\"block_index\":" + std::to_string(blockIdx)
                     + ",\"winning_nonce\":" + std::to_string(nonce)
                     + ",\"duration_sec\":" + std::to_string(durationSec) + "}";
    std::string escapedBody;
    escapedBody.reserve(body.size() * 2);
    for (char c : body) {
        if (c == '"') escapedBody += "\\\"";
        else escapedBody += c;
    }
    return runCmd("curl.exe -s -L -m " + std::to_string(timeoutSec)
                  + " -X POST -H \"Content-Type: application/json\" -d \""
                  + escapedBody + "\" \"" + url + "\"");
}

std::string extractJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos < json.length() && json[pos] == '\"') {
        pos++;
        size_t endPos = pos;
        while (endPos < json.length() && json[endPos] != '\"') {
            if (json[endPos] == '\\' && endPos + 1 < json.length()) endPos++;
            endPos++;
        }
        return json.substr(pos, endPos - pos);
    }
    size_t endPos = json.find_first_of(",}\r\n", pos);
    if (endPos == std::string::npos) endPos = json.length();
    return trimClean(json.substr(pos, endPos - pos));
}

inline uint32_t computeASICStep32(uint32_t state, uint32_t nonceChunk) {
    state ^= nonceChunk;
    state = (state << 13) | (state >> 19);
    state ^= 0x5bd1e995UL;
    state += (state << 5);
    state ^= (state >> 15);
    state += (state << 3);
    state ^= (state >> 7);
    return state;
}

inline int clz32(uint32_t x) {
    if (x == 0) return 32;
    unsigned long idx = 0;
    _BitScanReverse(&idx, x);
    return static_cast<int>(31 - idx);
}

inline bool checkDifficultyMatch(uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4, int diff) {
    if (diff <= 8) {
        if (diff == 8) return p1 == 0;
        return p1 == 0 ? true : (clz32(p1) >= diff * 4);
    } else if (diff <= 16) {
        if (p1 != 0) return false;
        int rem = diff - 8;
        if (rem == 8) return p2 == 0;
        return p2 == 0 ? true : (clz32(p2) >= rem * 4);
    } else if (diff <= 24) {
        if (p1 != 0 || p2 != 0) return false;
        int rem = diff - 16;
        if (rem == 8) return p3 == 0;
        return p3 == 0 ? true : (clz32(p3) >= rem * 4);
    } else {
        if (p1 != 0 || p2 != 0 || p3 != 0) return false;
        int rem = diff - 24;
        if (rem == 8) return p4 == 0;
        return p4 == 0 ? true : (clz32(p4) >= rem * 4);
    }
}

std::string formatHashToString(uint32_t s1, uint32_t s2) {
    const char* hex = "0123456789abcdef";
    char out[33];
    uint32_t parts[4] = { s1 ^ (s2 >> 3), s2 ^ (s1 << 3), s1 * 3, s2 * 5 };
    for (int k = 0; k < 4; ++k) {
        uint32_t v = parts[k];
        for (int i = 7; i >= 0; --i) {
            out[k * 8 + i] = hex[v & 0x0F];
            v >>= 4;
        }
    }
    out[32] = '\0';
    return std::string(out);
}

std::string simulateHash(const std::string& input) {
    uint32_t s1 = 2166136261UL, s2 = 3382945747UL;
    for (char c : input) {
        uint32_t val = static_cast<uint32_t>(c);
        s1 = computeASICStep32(s1, val);
        s2 = computeASICStep32(s2, val * 31);
    }
    s1 ^= (s2 << 11);
    s2 ^= (s1 >> 13);
    return formatHashToString(s1, s2);
}

bool verifyMod(const std::string& path, const std::string& expectedToken) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    std::getline(f, line);
    return trimClean(line) == expectedToken;
}

bool pressedEnter() {
    while (_kbhit()) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') return true;
    }
    return false;
}

unsigned long long calcChecksum(double balance, double poolPending, int blockIdx, int diff, bool banned, long long hashrate, bool speedMod, bool hackerMod, bool inspectWait, bool authed) {
    unsigned long long salt = 0x53414C54ULL;
    long long balSats = static_cast<long long>(balance * 100000000.0 + 0.5);
    long long poolSats = static_cast<long long>(poolPending * 100000000.0 + 0.5);
    return (balSats ^ (poolSats * 13) ^ (blockIdx * 31) ^ (diff * 17) ^ (hashrate * 7)
            ^ (speedMod ? 0x12345678ULL : 0)
            ^ (hackerMod ? 0xABCDEF01ULL : 0)
            ^ (inspectWait ? 0x99999999ULL : 0)
            ^ (authed ? 0x2468ACE0ULL : 0)
            ^ (banned ? 0xFFFFFFFFFFFFFFFFULL : 0)) + salt;
}

void saveState(const std::string& filePath, double balance, double poolPending, int blockIdx, int diff, bool autoMine, bool autoDiff, long long hashrate, long long outRate, bool banned, bool speedMod, bool hackerMod, bool inspectWait, const std::string& workerName, const std::string& workerToken, bool authed, std::string poolUrl) {
    if (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
    unsigned long long csum = calcChecksum(balance, poolPending, blockIdx, diff, banned, hashrate, speedMod, hackerMod, inspectWait, authed);
    std::ofstream out(filePath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return;
    out << "{\n"
        << "  \"worker_name\": \"" << workerName << "\",\n"
        << "  \"worker_token\": \"" << workerToken << "\",\n"
        << "  \"authed\": " << (authed ? "true" : "false") << ",\n"
        << "  \"pool_url\": \"" << poolUrl << "\",\n"
        << "  \"wallet_balance\": " << std::fixed << std::setprecision(8) << balance << ",\n"
        << "  \"pool_unclaimed\": " << std::fixed << std::setprecision(8) << poolPending << ",\n"
        << "  \"block_index\": " << blockIdx << ",\n"
        << "  \"difficulty\": " << diff << ",\n"
        << "  \"target_hashrate\": " << hashrate << ",\n"
        << "  \"output_rate\": " << outRate << ",\n"
        << "  \"auto_mine_next\": " << (autoMine ? "true" : "false") << ",\n"
        << "  \"inspection_wait\": " << (inspectWait ? "true" : "false") << ",\n"
        << "  \"auto_difficulty\": " << (autoDiff ? "true" : "false") << ",\n"
        << "  \"is_banned\": " << (banned ? "true" : "false") << ",\n"
        << "  \"security_checksum\": " << csum << "\n"
        << "}\n";
    out.flush();
}

void loadState(const std::string& filePath, double& balance, double& poolPending, int& blockIdx, int& diff, bool& autoMine, bool& autoDiff, long long& hashrate, long long& outRate, bool& banned, bool speedMod, bool hackerMod, bool& inspectWait, std::string& workerName, std::string& workerToken, bool& authed, std::string& poolUrl) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        saveState(filePath, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
        return;
    }
    std::string line;
    unsigned long long loadedCsum = 0;
    bool hasCsum = false;

    auto getVal = [&](const std::string& key) -> std::string {
        std::string k = "\"" + key + "\"";
        size_t p = line.find(k);
        if (p == std::string::npos) return "";
        size_t c = line.find(':', p);
        if (c == std::string::npos) return "";
        std::string raw = line.substr(c + 1);
        size_t a = raw.find_first_not_of(" \t\r\n\"");
        if (a == std::string::npos) return "";
        size_t b = raw.find_last_not_of(" \t\r\n\",");
        return raw.substr(a, b - a + 1);
    };

    while (std::getline(in, line)) {
        std::string v;
        if ((v = getVal("wallet_balance")) != "") balance = toDouble(v, balance);
        else if ((v = getVal("worker_name")) != "") workerName = v;
        else if ((v = getVal("worker_token")) != "") workerToken = v;
        else if ((v = getVal("authed")) != "") authed = (v == "true");
        else if ((v = getVal("pool_url")) != "") poolUrl = v;
        else if ((v = getVal("pool_unclaimed")) != "") poolPending = toDouble(v, poolPending);
        else if ((v = getVal("block_index")) != "") blockIdx = toInt(v, blockIdx);
        else if ((v = getVal("difficulty")) != "") diff = toInt(v, diff);
        else if ((v = getVal("target_hashrate")) != "") hashrate = toLL(v, hashrate);
        else if ((v = getVal("output_rate")) != "") outRate = toLL(v, outRate);
        else if ((v = getVal("auto_mine_next")) != "") autoMine = (v == "true");
        else if ((v = getVal("inspection_wait")) != "") inspectWait = (v == "true");
        else if ((v = getVal("auto_difficulty")) != "") autoDiff = (v == "true");
        else if ((v = getVal("is_banned")) != "") banned = (v == "true");
        else if ((v = getVal("security_checksum")) != "") {
            loadedCsum = toULL(v, 0);
            hasCsum = true;
        }
    }

    if (hasCsum && loadedCsum != calcChecksum(balance, poolPending, blockIdx, diff, banned, hashrate, speedMod, hackerMod, inspectWait, authed)) {
        banned = true;
        saveState(filePath, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
    }
}

PoolJob requestPoolJob(const std::string& poolUrl, const std::string& worker, const std::string& token, int fbBlock, int fbDiff, double fbPool, bool exhausted) {
    PoolJob job;
    std::string url = poolUrl + "/request_job?worker=" + worker + "&token=" + token + "&exhausted=" + (exhausted ? "1" : "0");
    std::string res = httpGet(url, 8);
    if (res.empty()) return job;

    std::string status = trimClean(extractJsonValue(res, "status"));
    if (status == "unauthorized") {
        job.unauthorized = true;
        job.message = trimClean(extractJsonValue(res, "message"));
        if (job.message.empty()) job.message = "Could not authorize the user, please try again.";
        return job;
    }

    if (res.find("chunk_min") == std::string::npos && res.find("worker_id") == std::string::npos) {
        return job;
    }

    job.valid = true;
    job.workerId = toLL(extractJsonValue(res, "worker_id"), 0);
    job.chunkMin = toLL(extractJsonValue(res, "chunk_min"), 0);
    long long step = toLL(extractJsonValue(res, "chunk_size"), DEFAULT_CHUNK_STEP);
    if (step <= 0) step = DEFAULT_CHUNK_STEP;
    job.chunkMax = toLL(extractJsonValue(res, "chunk_max"), job.chunkMin + step);
    if (job.chunkMax <= job.chunkMin) job.chunkMax = job.chunkMin + step;

    job.blockIndex = toInt(extractJsonValue(res, "block_index"), fbBlock);
    job.difficulty = toInt(extractJsonValue(res, "difficulty"), fbDiff);
    job.workEpoch = toInt(extractJsonValue(res, "work_epoch"), 1);
    job.previousHash = extractJsonValue(res, "previous_hash");

    std::string balStr = extractJsonValue(res, "unclaimed_pool");
    if (balStr.empty()) balStr = extractJsonValue(res, "pool_balance");
    job.unclaimedPool = toDouble(balStr, fbPool);
    return job;
}

void runLocalMining(const std::string& saveFile, double& balance, double poolPending, int& blockIdx, int& diff, bool autoMine, bool autoDiff, long long hashrate, long long outRate, bool banned, bool speedMod, bool hackerMod, bool inspectWait, const std::string& workerName, const std::string& workerToken, bool authed, const std::string& poolUrl) {
    bool running = true;
    while (running) {
        clearScreen();
        int threads = (std::max)(1, static_cast<int>(std::thread::hardware_concurrency()));
        std::cout << "[SYSTEM] Solo Engine (" << threads << " threads) | Block #" << blockIdx << " | Diff: " << diff << "/20\n"
                  << "[SYSTEM] Press [ENTER] to abort.\n\n";
        flushOut();

        std::atomic<bool> stop(false), found(false);
        std::atomic<long long> globalNonce(0), nextLog(outRate);
        long long winNonce = -1;
        std::string winHash;
        std::mutex ioMutex;

        std::string prevHash = simulateHash(std::to_string(GetTickCount64()));
        std::string prefix = prevHash + ":Block#" + std::to_string(blockIdx) + ":Nonce=";
        uint32_t b1 = 2166136261UL, b2 = 3382945747UL;
        for (char c : prefix) {
            uint32_t v = static_cast<uint32_t>(c);
            b1 = computeASICStep32(b1, v);
            b2 = computeASICStep32(b2, v * 31);
        }

        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t) {
            pool.emplace_back([&, b1, b2]() {
                const long long batch = 50000;
                while (!stop && !found) {
                    long long startN = globalNonce.fetch_add(batch, std::memory_order_relaxed);
                    long long endN = startN + batch;
                    for (long long n = startN; n < endN && !stop && !found; ++n) {
                        uint32_t lo = static_cast<uint32_t>(n & 0xFFFFFFFF);
                        uint32_t hi = static_cast<uint32_t>((n >> 32) & 0xFFFFFFFF);
                        uint32_t s1 = computeASICStep32(b1, lo);
                        uint32_t s2 = computeASICStep32(b2, hi ^ lo ^ 0x9e3779b9UL);
                        s1 ^= (s2 << 11);
                        s2 ^= (s1 >> 13);

                        if (checkDifficultyMatch(s1 ^ (s2 >> 3), s2 ^ (s1 << 3), s1 * 3, s2 * 5, diff)) {
                            bool exp = false;
                            if (found.compare_exchange_strong(exp, true)) {
                                winNonce = n;
                                winHash = formatHashToString(s1, s2);
                            }
                            break;
                        }
                        long long logAt = nextLog.load(std::memory_order_relaxed);
                        if (n >= logAt && nextLog.compare_exchange_strong(logAt, logAt + outRate)) {
                            std::lock_guard<std::mutex> lk(ioMutex);
                            std::cout << "nonce=" << COLOR_YELLOW << logAt << COLOR_RESET
                                      << " hash=" << COLOR_RED << formatHashToString(s1, s2) << COLOR_RESET << "\n";
                            flushOut();
                        }
                    }
                }
            });
        }

        while (!found && !stop) {
            if (pressedEnter()) { stop = true; break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        stop = true;
        for (auto& th : pool) if (th.joinable()) th.join();

        if (found) {
            double sec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
            double reward = 0.000005 * std::pow(1.6, diff - 1);
            balance += reward;
            blockIdx++;
            if (autoDiff) {
                if (sec < 0.8) diff = (std::min)(20, diff + 4);
                else if (sec < 2.5) diff = (std::min)(20, diff + 2);
                else if (sec < 7.0) diff = (std::min)(20, diff + 1);
                else if (sec > 22.0 && diff > 3) diff = (std::max)(3, diff - 2);
            }
            std::cout << "\n[" << getCurrentTime() << "] MATCHED nonce=" << COLOR_YELLOW << winNonce << COLOR_RESET
                      << " hash=" << COLOR_GREEN << winHash << COLOR_RESET
                      << " +" << std::fixed << std::setprecision(8) << reward << " BTC\n";
            flushOut();
            saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            if (inspectWait) std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!autoMine) { waitEnter(); running = false; }
        } else {
            std::cout << "\n[ABORT] Returning to menu...\n";
            flushOut();
            saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            running = false;
        }
    }
}

void runPoolMining(const std::string& saveFile, double& balance, double& poolPending, int& blockIdx, int& diff, bool autoMine, bool autoDiff, long long hashrate, long long outRate, bool banned, bool speedMod, bool hackerMod, bool inspectWait, const std::string& workerName, const std::string& workerToken, bool& authed, std::string& poolUrl) {
    if (!authed || workerToken.empty()) {
        std::cout << "\n" << COLOR_RED << "Could not authorize the user, please try again.\n" << COLOR_RESET;
        flushOut();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }
    if (poolUrl.empty()) {
        std::cout << "\nEnter Pool URL (e.g., https://pool.wispbyte.org): ";
        flushOut();
        if (!(std::cin >> poolUrl)) { discardLine(); return; }
        discardLine();
        if (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
        saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
    }

    bool running = true;
    bool nextExhausted = false;
    while (running) {
        clearScreen();
        int threads = (std::max)(1, static_cast<int>(std::thread::hardware_concurrency()));
        PoolJob job = requestPoolJob(poolUrl, workerName, workerToken, blockIdx, diff, poolPending, nextExhausted);
        nextExhausted = false;

        if (job.unauthorized) {
            std::cout << COLOR_RED << job.message << "\n" << COLOR_RESET;
            flushOut();
            authed = false;
            saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            break;
        }
        if (!job.valid) {
            std::cout << COLOR_RED << "[ERROR] Unreachable pool: " << poolUrl << "\n" << COLOR_RESET;
            flushOut();
            if (pressedEnter()) break;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }

        long long cMin = job.chunkMin, cMax = job.chunkMax;
        int pBlock = job.blockIndex, pDiff = job.difficulty, pEpoch = job.workEpoch;
        poolPending = job.unclaimedPool;

        std::cout << "[POOL] ID:" << job.workerId << " | Chunk:[" << cMin << ".." << cMax << ") | Block #" << pBlock
                  << " (Epoch #" << pEpoch << ") | Diff:" << pDiff << "/20 | Unclaimed:" << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPending << " BTC" << COLOR_RESET << "\n"
                  << "[SYSTEM] Press [ENTER] to abort.\n\n";
        flushOut();

        std::atomic<bool> stop(false), found(false), exhausted(false);
        std::atomic<long long> globalNonce(cMin), nextLog(cMin + outRate);
        long long winNonce = -1;
        std::string winHash;
        std::mutex ioMutex;

        std::string prefix = job.previousHash + ":Block#" + std::to_string(pBlock) + ":Nonce=";
        uint32_t b1 = 2166136261UL, b2 = 3382945747UL;
        for (char c : prefix) {
            uint32_t v = static_cast<uint32_t>(c);
            b1 = computeASICStep32(b1, v);
            b2 = computeASICStep32(b2, v * 31);
        }

        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t) {
            pool.emplace_back([&, b1, b2, cMax]() {
                const long long batch = 50000;
                while (!stop && !found && !exhausted.load(std::memory_order_relaxed)) {
                    long long startN = globalNonce.fetch_add(batch, std::memory_order_relaxed);
                    if (startN >= cMax) { exhausted.store(true, std::memory_order_relaxed); break; }
                    long long endN = (std::min)(startN + batch, cMax);
                    for (long long n = startN; n < endN && !stop && !found; ++n) {
                        uint32_t lo = static_cast<uint32_t>(n & 0xFFFFFFFF);
                        uint32_t hi = static_cast<uint32_t>((n >> 32) & 0xFFFFFFFF);
                        uint32_t s1 = computeASICStep32(b1, lo);
                        uint32_t s2 = computeASICStep32(b2, hi ^ lo ^ 0x9e3779b9UL);
                        s1 ^= (s2 << 11);
                        s2 ^= (s1 >> 13);

                        if (checkDifficultyMatch(s1 ^ (s2 >> 3), s2 ^ (s1 << 3), s1 * 3, s2 * 5, pDiff)) {
                            bool exp = false;
                            if (found.compare_exchange_strong(exp, true)) {
                                winNonce = n;
                                winHash = formatHashToString(s1, s2);
                            }
                            break;
                        }
                        long long logAt = nextLog.load(std::memory_order_relaxed);
                        if (n >= logAt && nextLog.compare_exchange_strong(logAt, logAt + outRate)) {
                            std::lock_guard<std::mutex> lk(ioMutex);
                            std::cout << "nonce=" << COLOR_YELLOW << logAt << COLOR_RESET
                                      << " hash=" << COLOR_RED << formatHashToString(s1, s2) << COLOR_RESET << "\n";
                            flushOut();
                        }
                    }
                }
            });
        }

        auto lastPoll = std::chrono::steady_clock::now();
        bool stateChanged = false;
        while (!found && !stop && !exhausted.load(std::memory_order_relaxed)) {
            if (pressedEnter()) { stop = true; break; }
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPoll).count() >= 1) {
                lastPoll = now;
                std::string st = httpGet(poolUrl + "/?worker=" + workerName, 2);
                if (!st.empty()) {
                    std::string rb = extractJsonValue(st, "block");
                    if (rb.empty()) rb = extractJsonValue(st, "block_index");
                    if (toInt(rb, pBlock) != pBlock
                        || toInt(extractJsonValue(st, "difficulty"), pDiff) != pDiff
                        || toInt(extractJsonValue(st, "work_epoch"), pEpoch) != pEpoch) {
                        stateChanged = true;
                        stop = true;
                        break;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        stop = true;
        for (auto& th : pool) if (th.joinable()) th.join();

        if (stateChanged) continue;
        if (found) {
            double sec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
            std::string subRes = httpPostSubmit(poolUrl + "/submit", workerName, workerToken, pBlock, winNonce, sec, 8);
            if (trimClean(extractJsonValue(subRes, "status")) == "accepted") {
                std::string bStr = extractJsonValue(subRes, "unclaimed_pool");
                if (bStr.empty()) bStr = extractJsonValue(subRes, "pool_balance");
                poolPending = toDouble(bStr, poolPending);
                std::cout << "\n[" << getCurrentTime() << "] ACCEPTED nonce=" << COLOR_YELLOW << winNonce << COLOR_RESET
                          << " pool_unclaimed=" << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPending << " BTC" << COLOR_RESET << "\n";
            } else {
                std::cout << "\n" << COLOR_RED << "[REJECTED] " << (subRes.empty() ? "Timeout" : subRes) << COLOR_RESET << "\n";
            }
            flushOut();
            saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            if (inspectWait) std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!autoMine) { waitEnter(); running = false; }
        } else if (exhausted.load(std::memory_order_relaxed)) {
            nextExhausted = true;
        } else {
            saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            running = false;
        }
    }
}

void runStandardOptions(const std::string& saveFile, double& balance, double poolPending, int& blockIdx, int& diff, bool& autoMine, bool& autoDiff, long long& hashrate, long long& outRate, bool banned, bool speedMod, bool hackerMod, bool& inspectWait, const std::string& workerName, const std::string& workerToken, bool authed, const std::string& poolUrl) {
    while (true) {
        clearScreen();
        int idx = 1, map[10];
        std::cout << "> miner_options\n----------------------------------------\n";
        if (hackerMod) {
            std::cout << "  [" << idx << "] set_difficulty : " << diff << " [MOD]\n"; map[idx++] = 1;
            std::cout << "  [" << idx << "] auto_difficulty: " << (autoDiff ? "ON" : "OFF") << " [MOD]\n"; map[idx++] = 2;
        }
        std::cout << "  [" << idx << "] auto_mine_next : " << (autoMine ? "ON" : "OFF") << "\n"; map[idx++] = 3;
        std::cout << "  [" << idx << "] inspection_wait: " << (inspectWait ? "ON" : "OFF") << "\n"; map[idx++] = 4;
        std::cout << "  [" << idx << "] output_rate    : " << outRate << "\n"; map[idx++] = 5;
        if (speedMod) { std::cout << "  [" << idx << "] set_hashrate   : UNLIMITED [MOD]\n"; map[idx++] = 6; }
        std::cout << "  [" << idx << "] back_to_main\n----------------------------------------\nroot@miner/options# ";
        flushOut();

        int ch = 0;
        if (!(std::cin >> ch)) { discardLine(); continue; }
        discardLine();
        if (ch == idx) break;
        if (ch >= 1 && ch < idx) {
            int act = map[ch];
            if (act == 1) { std::cout << "Difficulty (1-20): "; flushOut(); if (std::cin >> diff) diff = (std::clamp)(diff, 1, 20); discardLine(); }
            else if (act == 2) autoDiff = !autoDiff;
            else if (act == 3) autoMine = !autoMine;
            else if (act == 4) inspectWait = !inspectWait;
            else if (act == 5) { std::cout << "Output rate: "; flushOut(); if (std::cin >> outRate) outRate = (std::max)(1000LL, outRate); discardLine(); }
            else if (act == 6) { std::cout << "[MOD] UNLIMITED!\n"; flushOut(); std::this_thread::sleep_for(std::chrono::seconds(1)); }
        }
        saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
    }
}

void runPoolOptions(const std::string& saveFile, double& balance, double& poolPending, int blockIdx, int diff, bool autoMine, bool autoDiff, long long hashrate, long long outRate, bool banned, bool speedMod, bool hackerMod, bool inspectWait, std::string& workerName, std::string& workerToken, bool& authed, std::string& poolUrl) {
    while (true) {
        if (!poolUrl.empty() && !workerName.empty()) {
            std::string uRes = httpGet(poolUrl + "/unclaimed?worker=" + workerName, 4);
            if (!uRes.empty()) {
                std::string b = extractJsonValue(uRes, "unclaimed_pool");
                if (b.empty()) b = extractJsonValue(uRes, "pool_balance");
                if (!b.empty()) poolPending = toDouble(b, poolPending);
            }
        }
        clearScreen();
        std::cout << "> pool_options\n"
                  << "----------------------------------------\n"
                  << "  [1] set_worker" << (authed ? " (re-auth/new)" : "") << "\n"
                  << "  [2] set_pool_url : " << (poolUrl.empty() ? "None" : poolUrl) << "\n"
                  << "  [3] claim_pool\n"
                  << "  [4] back_to_main\n"
                  << "----------------------------------------\n"
                  << "root@miner/pool_options# ";
        flushOut();

        int ch = 0;
        if (!(std::cin >> ch)) { discardLine(); continue; }
        discardLine();

        if (ch == 4) break;
        if (ch == 1) {
            if (poolUrl.empty()) {
                std::cout << COLOR_RED << "[ERROR] Set Pool URL first!\n" << COLOR_RESET;
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            std::cout << "\n  [1] Create\n  [2] Auth\nSelect: ";
            flushOut();
            int mode = 0;
            if (!(std::cin >> mode)) { discardLine(); continue; }
            discardLine();

            if (mode == 1) {
                std::string name, tok;
                std::cout << "Set name: ";
                flushOut();
                if (!(std::cin >> name)) { discardLine(); continue; }
                discardLine();

                std::string chk = httpGet(poolUrl + "/check_name?worker=" + name, 5);
                if (chk.empty() || chk.find("\"available\"") == std::string::npos) {
                    std::cout << COLOR_RED << "[ERROR] Cannot reach/parse pool at " << poolUrl << "\n" << COLOR_RESET;
                    flushOut();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }
                if (trimClean(extractJsonValue(chk, "available")) != "true") {
                    std::cout << COLOR_RED << "Name is already taken by another worker.\n" << COLOR_RESET;
                    flushOut();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }

                std::cout << "Set token: ";
                flushOut();
                if (!(std::cin >> tok)) { discardLine(); continue; }
                discardLine();

                std::string reg = httpPostJson(poolUrl + "/register_worker", name, tok, 8);
                std::string st = trimClean(extractJsonValue(reg, "status"));
                if (st == "ok") {
                    workerName = name;
                    workerToken = tok;
                    authed = true;
                    std::string b = extractJsonValue(reg, "unclaimed_pool");
                    if (b.empty()) b = extractJsonValue(reg, "pool_balance");
                    poolPending = toDouble(b, 0.0);
                    std::cout << COLOR_GREEN << "[OK] Created & authed [" << workerName << "].\n" << COLOR_RESET;
                } else if (st == "taken") {
                    std::cout << COLOR_RED << "Name is already taken by another worker.\n" << COLOR_RESET;
                } else {
                    std::cout << COLOR_RED << "[ERROR] Register failed: " << (reg.empty() ? "Empty response" : reg) << "\n" << COLOR_RESET;
                }
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            } else if (mode == 2) {
                std::string name, tok;
                std::cout << "Name: ";
                flushOut();
                if (!(std::cin >> name)) { discardLine(); continue; }
                discardLine();
                std::cout << "Token: ";
                flushOut();
                if (!(std::cin >> tok)) { discardLine(); continue; }
                discardLine();

                std::string authRes = httpPostJson(poolUrl + "/auth_worker", name, tok, 8);
                if (trimClean(extractJsonValue(authRes, "status")) == "ok") {
                    workerName = name;
                    workerToken = tok;
                    authed = true;
                    std::string b = extractJsonValue(authRes, "unclaimed_pool");
                    if (b.empty()) b = extractJsonValue(authRes, "pool_balance");
                    poolPending = toDouble(b, poolPending);
                    std::cout << COLOR_GREEN << "[OK] Authed [" << workerName << "].\n" << COLOR_RESET;
                } else {
                    std::cout << COLOR_RED << "[ERROR] Auth failed.\n" << COLOR_RESET;
                }
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        } else if (ch == 2) {
            std::cout << "Enter Pool URL: ";
            flushOut();
            if (std::cin >> poolUrl) {
                discardLine();
                if (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
            } else discardLine();
        } else if (ch == 3) {
            if (poolUrl.empty()) {
                std::cout << COLOR_RED << "[ERROR] Set Pool URL first!\n" << COLOR_RESET;
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                std::string res = httpPostJson(poolUrl + "/claim_distribution", workerName, workerToken, 8);
                std::string st = trimClean(extractJsonValue(res, "status"));
                if (st == "claimed") {
                    double claimed = toDouble(extractJsonValue(res, "claimed"), 0.0);
                    balance += claimed;
                    poolPending = 0.0;
                    std::cout << COLOR_GREEN << "[SUCCESS] Claimed +" << std::fixed << std::setprecision(8) << claimed << " BTC\n" << COLOR_RESET;
                } else if (st == "unauthorized") {
                    authed = false;
                    std::cout << COLOR_RED << "Could not authorize the user, please try again.\n" << COLOR_RESET;
                } else {
                    std::cout << COLOR_YELLOW << "[INFO] No pool balance to claim.\n" << COLOR_RESET;
                }
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
        saveState(saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
    }
}

int main() {
    enableAnsi();
    rawPrint("[BOOT] Starting CryptoMiner Windows Client...\r\n");

    AppPaths paths = getAppPaths();
    if (!verifyExeInBaseDir(paths.baseDir)) {
        clearScreen();
        std::string errMsg = "[ERROR] Place miner.exe inside:\n" + paths.baseDir + "\nbefore first run.\n";
        rawPrint(errMsg);
        std::cout << COLOR_RED << errMsg << COLOR_RESET;
        waitEnter("\nPress ENTER to exit...");
        return 1;
    }

    try { std::filesystem::create_directories(paths.modsDir); } catch (...) {}

    int diff = 3, blockIdx = 1;
    bool autoMine = true, inspectWait = true, autoDiff = true, banned = false, authed = false;
    long long hashrate = 2500000, outRate = 50000;
    double balance = 1.0, poolPending = 0.0;
    std::string workerName = "Node_01", workerToken = "", poolUrl = "";

    bool speedMod = verifyMod(paths.speedMod, SPEED_TOKEN);
    bool hackerMod = verifyMod(paths.hackerMod, HACKER_TOKEN);
    bool poolMod = verifyMod(paths.poolMod, POOL_TOKEN);

    loadState(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);

    if (banned) {
        clearScreen();
        std::cout << COLOR_RED << "access denied\nConfig compromised. Delete financial_report.json to reset.\n" << COLOR_RESET;
        waitEnter("\nPress ENTER to exit...");
        return 0;
    }

    while (true) {
        clearScreen();
        int threads = (std::max)(1, static_cast<int>(std::thread::hardware_concurrency()));
        if (!poolMod) {
            std::cout << "> miner_main_menu\n----------------------------------------\n"
                      << "  [1] start_mining\n  [2] options\n  [3] quit\n----------------------------------------\n"
                      << "wallet_balance: " << std::fixed << std::setprecision(8) << balance << " BTC\n"
                      << "current_hash  : " << (speedMod ? "UNLIMITED [RAW SPEED MOD]" : std::to_string(hashrate) + " H/s") << "\n"
                      << "cpu_threads   : " << threads << " Cores\n"
                      << "current_block : #" << blockIdx << "\n"
                      << (speedMod ? COLOR_CYAN "[MOD LOADER] unlimited_speed.mod\n" : "")
                      << (hackerMod ? COLOR_MAGENTA "[MOD LOADER] hacker.mod\n" : "")
                      << COLOR_RESET << "root@miner:~# ";
            flushOut();

            int ch = 0;
            if (!(std::cin >> ch)) { discardLine(); continue; }
            discardLine();
            if (ch == 3) { saveState(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl); break; }
            if (ch == 2) runStandardOptions(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            if (ch == 1) runLocalMining(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
        } else {
            std::cout << "> miner_main_menu\n----------------------------------------\n"
                      << "  [1] solo_mining\n  [2] pool_mining\n  [3] options\n  [4] pool_options\n  [5] quit\n----------------------------------------\n"
                      << "worker_id     : " << COLOR_CYAN << workerName << COLOR_RESET << "\n"
                      << "pool_host     : " << (poolUrl.empty() ? "Not Configured" : poolUrl) << "\n"
                      << "wallet_balance: " << std::fixed << std::setprecision(8) << balance << " BTC\n"
                      << "pool_unclaimed: " << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPending << " BTC" << COLOR_RESET << "\n"
                      << "current_hash  : " << (speedMod ? "UNLIMITED [RAW SPEED MOD]" : std::to_string(hashrate) + " H/s") << "\n"
                      << "cpu_threads   : " << threads << " Cores\n"
                      << "current_block : #" << blockIdx << "\n"
                      << (speedMod ? COLOR_CYAN "[MOD LOADER] unlimited_speed.mod\n" : "")
                      << (hackerMod ? COLOR_MAGENTA "[MOD LOADER] hacker.mod\n" : "")
                      << COLOR_YELLOW "[MOD LOADER] pool_mining.mod\n" << COLOR_RESET
                      << "root@miner:~# ";
            flushOut();

            int ch = 0;
            if (!(std::cin >> ch)) { discardLine(); continue; }
            discardLine();
            if (ch == 5) { saveState(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl); break; }
            if (ch == 4) runPoolOptions(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            if (ch == 3) runStandardOptions(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            if (ch == 2) runPoolMining(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
            if (ch == 1) runLocalMining(paths.saveFile, balance, poolPending, blockIdx, diff, autoMine, autoDiff, hashrate, outRate, banned, speedMod, hackerMod, inspectWait, workerName, workerToken, authed, poolUrl);
        }
    }
    return 0;
}
