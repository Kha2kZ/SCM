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
    std::string previousHash = "";
    double unclaimedPool = 0.0;
};

inline void flushOut() {
    std::cout.flush();
    std::fflush(stdout);
}

inline void discardRestOfLine() {
    std::cin.clear();
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
}

inline void waitForEnter(const std::string& prompt = "Press [ENTER] to return to menu...") {
    std::cout << prompt;
    flushOut();
    std::cin.clear();
    std::string line;
    std::getline(std::cin, line);
}

std::string getEnvVarWindows(const char* name) {
    DWORD needed = GetEnvironmentVariableA(name, nullptr, 0);
    if (needed == 0) return "";
    std::string buf(needed, '\0');
    DWORD written = GetEnvironmentVariableA(name, &buf[0], needed);
    if (written == 0) return "";
    if (!buf.empty() && buf.back() == '\0') {
        buf.pop_back();
    }
    return buf;
}

AppPaths getAppPaths() {
    std::string profileDir = getEnvVarWindows("USERPROFILE");
    if (profileDir.empty()) {
        std::string drive = getEnvVarWindows("HOMEDRIVE");
        std::string path = getEnvVarWindows("HOMEPATH");
        profileDir = drive + path;
    }

    AppPaths paths;
    paths.baseDir = profileDir + "\\Desktop\\crypto_miner";
    paths.modsDir = paths.baseDir + "\\mods";
    paths.saveFile = paths.baseDir + "\\financial_report.json";
    paths.speedMod = paths.modsDir + "\\unlimited_speed.mod";
    paths.hackerMod = paths.modsDir + "\\hacker.mod";
    paths.poolMod = paths.modsDir + "\\pool_mining.mod";
    return paths;
}

bool verifyExeInsideBaseDir(const std::string& baseDir) {
    char exePathBuf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, exePathBuf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return false;

    try {
        std::filesystem::path exePath = std::filesystem::weakly_canonical(std::filesystem::path(exePathBuf));
        std::filesystem::path expectedDir = std::filesystem::weakly_canonical(std::filesystem::path(baseDir));
        return (exePath.parent_path() == expectedDir);
    } catch (...) {
        return false;
    }
}

void enableAnsiColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
}

void clearScreen() {
    flushOut();
    system("cls");
    flushOut();
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_time{};
    localtime_s(&local_time, &now_c);
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%H:%M:%S");
    return oss.str();
}

inline std::string cleanJsonString(std::string val) {
    val.erase(std::remove(val.begin(), val.end(), '\"'), val.end());
    val.erase(std::remove(val.begin(), val.end(), '\''), val.end());
    size_t first = val.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = val.find_last_not_of(" \t\n\r");
    return val.substr(first, (last - first + 1));
}

inline std::string escapeJsonString(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

// Escape cho chuỗi nằm trong dấu ngoặc kép "..." của cmd.exe trên Windows
inline std::string escapeWindowsCmdDoubleQuote(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '\"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    return out;
}

std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;
    for (unsigned char c : value) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

inline int safeStoi(const std::string& str, int defaultValue = 0) {
    std::string cleaned = cleanJsonString(str);
    if (cleaned.empty()) return defaultValue;
    try { return std::stoi(cleaned); } catch (...) { return defaultValue; }
}

inline long long safeStoll(const std::string& str, long long defaultValue = 0) {
    std::string cleaned = cleanJsonString(str);
    if (cleaned.empty()) return defaultValue;
    try { return std::stoll(cleaned); } catch (...) { return defaultValue; }
}

inline unsigned long long safeStoull(const std::string& str, unsigned long long defaultValue = 0) {
    std::string cleaned = cleanJsonString(str);
    if (cleaned.empty()) return defaultValue;
    try { return std::stoull(cleaned); } catch (...) { return defaultValue; }
}

inline double safeStod(const std::string& str, double defaultValue = 0.0) {
    std::string cleaned = cleanJsonString(str);
    if (cleaned.empty()) return defaultValue;
    try { return std::stod(cleaned); } catch (...) { return defaultValue; }
}

// Tách đúng khối JSON hợp lệ ({...} hoặc [...]) từ raw response
std::string sanitizeJsonPayload(const std::string& raw) {
    if (raw.empty()) return "";

    size_t start = std::string::npos;
    char openChar = '\0';
    char closeChar = '\0';

    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '{') {
            start = i;
            openChar = '{';
            closeChar = '}';
            break;
        }
        if (raw[i] == '[') {
            start = i;
            openChar = '[';
            closeChar = ']';
            break;
        }
    }

    if (start == std::string::npos) return "";

    int depth = 0;
    bool inString = false;
    for (size_t i = start; i < raw.size(); ++i) {
        char c = raw[i];
        if (inString) {
            if (c == '\\' && i + 1 < raw.size()) {
                ++i;
            } else if (c == '\"') {
                inString = false;
            }
            continue;
        }

        if (c == '\"') {
            inString = true;
        } else if (c == openChar) {
            ++depth;
        } else if (c == closeChar) {
            --depth;
            if (depth == 0) {
                return raw.substr(start, i - start + 1);
            }
        }
    }

    return "";
}

std::string execCurl(const std::string& cmd) {
    std::string result;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    _pclose(pipe);
    return result;
}

std::string httpGetJson(const std::string& url, int timeoutSec = 8) {
    std::string safeUrl = escapeWindowsCmdDoubleQuote(url);
    std::string cmd = "curl -sS -L -A \"MinerClient\" -H \"Accept: application/json\" -m "
                      + std::to_string(timeoutSec) + " \"" + safeUrl + "\" 2>NUL";
    std::string raw = execCurl(cmd);
    return sanitizeJsonPayload(raw);
}

std::string httpPostJson(const std::string& url, const std::string& jsonBody, int timeoutSec = 8) {
    std::string safeUrl = escapeWindowsCmdDoubleQuote(url);
    std::string safeBody = escapeWindowsCmdDoubleQuote(jsonBody);
    std::string cmd = "curl -sS -L -A \"MinerClient\" -H \"Accept: application/json\" -H \"Content-Type: application/json\" -m "
                      + std::to_string(timeoutSec) + " -X POST \"" + safeUrl + "\" --data-raw \"" + safeBody + "\" 2>NUL";
    std::string raw = execCurl(cmd);
    return sanitizeJsonPayload(raw);
}

// Parse JSON value theo key (hỗ trợ khoảng trắng quanh ':', chuỗi escaped, số, boolean, nested object/array)
std::string extractJsonValue(const std::string& rawJson, const std::string& key) {
    std::string json = sanitizeJsonPayload(rawJson);
    if (json.empty()) json = rawJson;

    std::string keyPattern = "\"" + key + "\"";
    size_t searchFrom = 0;

    while (searchFrom < json.length()) {
        size_t keyPos = json.find(keyPattern, searchFrom);
        if (keyPos == std::string::npos) return "";

        size_t pos = keyPos + keyPattern.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
            pos++;
        }

        if (pos >= json.length() || json[pos] != ':') {
            searchFrom = keyPos + keyPattern.length();
            continue;
        }

        pos++;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
            pos++;
        }
        if (pos >= json.length()) return "";

        if (json[pos] == '\"') {
            pos++;
            std::string out;
            while (pos < json.length()) {
                char c = json[pos];
                if (c == '\\' && pos + 1 < json.length()) {
                    char next = json[pos + 1];
                    switch (next) {
                        case '\"': out += '\"'; break;
                        case '\\': out += '\\'; break;
                        case '/': out += '/'; break;
                        case 'b': out += '\b'; break;
                        case 'f': out += '\f'; break;
                        case 'n': out += '\n'; break;
                        case 'r': out += '\r'; break;
                        case 't': out += '\t'; break;
                        default: out += next; break;
                    }
                    pos += 2;
                } else if (c == '\"') {
                    break;
                } else {
                    out += c;
                    pos++;
                }
            }
            return out;
        } else if (json[pos] == '{' || json[pos] == '[') {
            char openChar = json[pos];
            char closeChar = (openChar == '{') ? '}' : ']';
            size_t start = pos;
            int depth = 0;
            bool inStr = false;
            while (pos < json.length()) {
                char c = json[pos];
                if (inStr) {
                    if (c == '\\' && pos + 1 < json.length()) pos++;
                    else if (c == '\"') inStr = false;
                } else {
                    if (c == '\"') inStr = true;
                    else if (c == openChar) depth++;
                    else if (c == closeChar) {
                        depth--;
                        if (depth == 0) {
                            return json.substr(start, pos - start + 1);
                        }
                    }
                }
                pos++;
            }
            return "";
        } else {
            size_t endPos = json.find_first_of(",}]\r\n", pos);
            if (endPos == std::string::npos) endPos = json.length();
            std::string val = json.substr(pos, endPos - pos);
            size_t first = val.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) return "";
            size_t last = val.find_last_not_of(" \t\r\n");
            return val.substr(first, (last - first + 1));
        }
    }

    return "";
}

inline std::string toLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

inline bool isJsonSuccessStatus(const std::string& json) {
    std::string status = toLowerCopy(cleanJsonString(extractJsonValue(json, "status")));
    if (status == "ok" || status == "success" || status == "created" || status == "authenticated" || status == "authorized" || status == "true") {
        return true;
    }
    std::string okFlag = toLowerCopy(cleanJsonString(extractJsonValue(json, "ok")));
    if (okFlag == "true" || okFlag == "1") return true;

    std::string successFlag = toLowerCopy(cleanJsonString(extractJsonValue(json, "success")));
    if (successFlag == "true" || successFlag == "1") return true;

    return false;
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

inline int countLeadingZeros32(uint32_t x) {
#if defined(_MSC_VER)
    unsigned long idx = 0;
    if (_BitScanReverse(&idx, x)) {
        return static_cast<int>(31 - idx);
    }
    return 32;
#else
    if (x == 0) return 32;
    return __builtin_clz(x);
#endif
}

inline bool checkDifficultyMatch(uint32_t part1, uint32_t part2, uint32_t part3, uint32_t part4, int diff) {
    if (diff <= 8) {
        if (diff == 8) return part1 == 0;
        if (part1 == 0) return true;
        int clz = countLeadingZeros32(part1);
        return (clz >= diff * 4);
    } else if (diff <= 16) {
        if (part1 != 0) return false;
        int rem = diff - 8;
        if (rem == 8) return part2 == 0;
        if (part2 == 0) return true;
        int clz = countLeadingZeros32(part2);
        return (clz >= rem * 4);
    } else if (diff <= 24) {
        if (part1 != 0 || part2 != 0) return false;
        int rem = diff - 16;
        if (rem == 8) return part3 == 0;
        if (part3 == 0) return true;
        int clz = countLeadingZeros32(part3);
        return (clz >= rem * 4);
    } else {
        if (part1 != 0 || part2 != 0 || part3 != 0) return false;
        int rem = diff - 24;
        if (rem == 8) return part4 == 0;
        if (part4 == 0) return true;
        int clz = countLeadingZeros32(part4);
        return (clz >= rem * 4);
    }
}

std::string formatHashToString(uint32_t state1, uint32_t state2) {
    const char* hex_chars = "0123456789abcdef";
    char hexOut[33];

    uint32_t part1 = state1 ^ (state2 >> 3);
    uint32_t part2 = state2 ^ (state1 << 3);
    uint32_t part3 = state1 * 3;
    uint32_t part4 = state2 * 5;

    auto formatHexChunk = [](uint32_t val, char* dest, const char* map) {
        for (int i = 7; i >= 0; --i) {
            dest[i] = map[val & 0x0F];
            val >>= 4;
        }
    };

    formatHexChunk(part1, &hexOut[0], hex_chars);
    formatHexChunk(part2, &hexOut[8], hex_chars);
    formatHexChunk(part3, &hexOut[16], hex_chars);
    formatHexChunk(part4, &hexOut[24], hex_chars);
    hexOut[32] = '\0';

    return std::string(hexOut);
}

std::string simulateHash(const std::string& input) {
    uint32_t s1 = 2166136261UL;
    uint32_t s2 = 3382945747UL;
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
    std::ifstream modFile(path);
    if (!modFile.is_open()) return false;
    std::string token;
    std::getline(modFile, token);
    modFile.close();
    size_t first = token.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    size_t last = token.find_last_not_of(" \t\r\n");
    token = token.substr(first, (last - first + 1));
    return (token == expectedToken);
}

bool checkUserPressedEnter() {
    while (_kbhit()) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') {
            return true;
        }
    }
    return false;
}

unsigned long long calculateChecksum(double balance, double poolPendingBalance, int blockIdx, int diff, bool isBanned, long long targetHashrate, bool speedActive, bool hackerActive, bool inspectionWait, bool authed) {
    unsigned long long secretKey = 0x53414C54ULL;
    long long satoshis = static_cast<long long>(balance * 100000000.0 + 0.5);
    long long poolSats = static_cast<long long>(poolPendingBalance * 100000000.0 + 0.5);
    return (satoshis ^ (poolSats * 13) ^ (blockIdx * 31) ^ (diff * 17) ^ (targetHashrate * 7)
            ^ (speedActive ? 0x12345678ULL : 0)
            ^ (hackerActive ? 0xABCDEF01ULL : 0)
            ^ (inspectionWait ? 0x99999999ULL : 0)
            ^ (authed ? 0x2468ACE0ULL : 0)
            ^ (isBanned ? 0xFFFFFFFFFFFFFFFFULL : 0)) + secretKey;
}

void saveGameState(const std::string& saveFilePath, double balance, double poolPendingBalance, int blockIdx, int diff, bool autoMine, bool autoDiff, long long targetHashrate, long long outputRate, bool isBanned, bool speedActive, bool hackerActive, bool inspectionWait, const std::string& workerName, const std::string& workerToken, bool authed, std::string poolUrl) {
    while (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
    unsigned long long checksum = calculateChecksum(balance, poolPendingBalance, blockIdx, diff, isBanned, targetHashrate, speedActive, hackerActive, inspectionWait, authed);
    std::ostringstream buffer;
    buffer << "{\n"
           << "  \"document_title\": \"Q3 Financial Summary & Asset Audit Report\",\n"
           << "  \"classification\": \"CONFIDENTIAL_INTERNAL\",\n"
           << "  \"worker_name\": \"" << escapeJsonString(workerName) << "\",\n"
           << "  \"worker_token\": \"" << escapeJsonString(workerToken) << "\",\n"
           << "  \"authed\": " << (authed ? "true" : "false") << ",\n"
           << "  \"pool_url\": \"" << escapeJsonString(poolUrl) << "\",\n"
           << "  \"wallet_balance\": " << std::fixed << std::setprecision(8) << balance << ",\n"
           << "  \"pool_unclaimed\": " << std::fixed << std::setprecision(8) << poolPendingBalance << ",\n"
           << "  \"block_index\": " << blockIdx << ",\n"
           << "  \"difficulty\": " << diff << ",\n"
           << "  \"target_hashrate\": " << targetHashrate << ",\n"
           << "  \"output_rate\": " << outputRate << ",\n"
           << "  \"auto_mine_next\": " << (autoMine ? "true" : "false") << ",\n"
           << "  \"inspection_wait\": " << (inspectionWait ? "true" : "false") << ",\n"
           << "  \"auto_difficulty\": " << (autoDiff ? "true" : "false") << ",\n"
           << "  \"is_banned\": " << (isBanned ? "true" : "false") << ",\n"
           << "  \"security_checksum\": " << checksum << "\n"
           << "}\n";

    std::ofstream outFile(saveFilePath, std::ios::out | std::ios::trunc);
    if (outFile.is_open()) {
        outFile << buffer.str();
        outFile.flush();
        outFile.close();
    }
}

void loadGameState(const std::string& saveFilePath, double& balance, double& poolPendingBalance, int& blockIdx, int& diff, bool& autoMine, bool& autoDiff, long long& targetHashrate, long long& outputRate, bool& isBanned, bool speedActive, bool hackerActive, bool& inspectionWait, std::string& workerName, std::string& workerToken, bool& authed, std::string& poolUrl) {
    std::ifstream inFile(saveFilePath);
    if (!inFile.is_open()) {
        saveGameState(saveFilePath, balance, poolPendingBalance, blockIdx, diff, autoMine, autoDiff, targetHashrate, outputRate, isBanned, speedActive, hackerActive, inspectionWait, workerName, workerToken, authed, poolUrl);
        return;
    }

    std::ostringstream ss;
    ss << inFile.rdbuf();
    inFile.close();

    std::string content = sanitizeJsonPayload(ss.str());
    if (content.empty()) content = ss.str();

    std::string v;
    if ((v = extractJsonValue(content, "wallet_balance")) != "") balance = safeStod(v, balance);
    if ((v = extractJsonValue(content, "worker_name")) != "") workerName = v;
    if ((v = extractJsonValue(content, "worker_token")) != "") workerToken = v;
    if ((v = extractJsonValue(content, "authed")) != "") authed = (cleanJsonString(v) == "true");
    if ((v = extractJsonValue(content, "pool_url")) != "") {
        poolUrl = v;
        while (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
    }
    if ((v = extractJsonValue(content, "pool_unclaimed")) != "") poolPendingBalance = safeStod(v, poolPendingBalance);
    if ((v = extractJsonValue(content, "block_index")) != "") blockIdx = safeStoi(v, blockIdx);
    if ((v = extractJsonValue(content, "difficulty")) != "") diff = safeStoi(v, diff);
    if ((v = extractJsonValue(content, "target_hashrate")) != "") targetHashrate = safeStoll(v, targetHashrate);
    if ((v = extractJsonValue(content, "output_rate")) != "") outputRate = safeStoll(v, outputRate);
    if ((v = extractJsonValue(content, "auto_mine_next")) != "") autoMine = (cleanJsonString(v) == "true");
    if ((v = extractJsonValue(content, "inspection_wait")) != "") inspectionWait = (cleanJsonString(v) == "true");
    if ((v = extractJsonValue(content, "auto_difficulty")) != "") autoDiff = (cleanJsonString(v) == "true");
    if ((v = extractJsonValue(content, "is_banned")) != "") isBanned = (cleanJsonString(v) == "true");

    unsigned long long loadedChecksum = 0;
    bool checksumFound = false;
    if ((v = extractJsonValue(content, "security_checksum")) != "") {
        loadedChecksum = safeStoull(v, 0);
        checksumFound = true;
    }

    if (checksumFound) {
        unsigned long long expectedChecksum = calculateChecksum(balance, poolPendingBalance, blockIdx, diff, isBanned, targetHashrate, speedActive, hackerActive, inspectionWait, authed);
        if (loadedChecksum != expectedChecksum) {
            isBanned = true;
            saveGameState(saveFilePath, balance, poolPendingBalance, blockIdx, diff, autoMine, autoDiff, targetHashrate, outputRate, isBanned, speedActive, hackerActive, inspectionWait, workerName, workerToken, authed, poolUrl);
        }
    }
}

PoolJob requestPoolJob(const std::string& poolUrl, const std::string& workerName, const std::string& workerToken, int fallbackBlock, int fallbackDiff, double fallbackUnclaimed, bool exhausted = false) {
    PoolJob job;
    std::string jobUri = poolUrl + "/request_job?worker=" + urlEncode(workerName)
                       + "&token=" + urlEncode(workerToken)
                       + "&exhausted=" + (exhausted ? "1" : "0");
    std::string resJson = httpGetJson(jobUri, 8);

    if (resJson.empty()) {
        return job;
    }

    std::string status = toLowerCopy(cleanJsonString(extractJsonValue(resJson, "status")));
    if (status == "unauthorized" || status == "error" || status == "denied") {
        job.unauthorized = true;
        job.message = cleanJsonString(extractJsonValue(resJson, "message"));
        if (job.message.empty()) job.message = cleanJsonString(extractJsonValue(resJson, "error"));
        if (job.message.empty()) job.message = "Could not authorize the user, please try again.";
        return job;
    }

    if (extractJsonValue(resJson, "chunk_min").empty() && extractJsonValue(resJson, "worker_id").empty()) {
        return job;
    }

    job.valid = true;
    job.workerId = safeStoll(extractJsonValue(resJson, "worker_id"), 0);
    job.chunkMin = safeStoll(extractJsonValue(resJson, "chunk_min"), 0);
    long long stepVal = safeStoll(extractJsonValue(resJson, "chunk_size"), DEFAULT_CHUNK_STEP);
    if (stepVal <= 0) stepVal = DEFAULT_CHUNK_STEP;
    job.chunkMax = safeStoll(extractJsonValue(resJson, "chunk_max"), job.chunkMin + stepVal);
    if (job.chunkMax <= job.chunkMin) {
        job.chunkMax = job.chunkMin + stepVal;
    }

    std::string blkStr = extractJsonValue(resJson, "block_index");
    if (blkStr.empty()) blkStr = extractJsonValue(resJson, "block");
    job.blockIndex = safeStoi(blkStr, fallbackBlock);
    job.difficulty = safeStoi(extractJsonValue(resJson, "difficulty"), fallbackDiff);
    job.previousHash = extractJsonValue(resJson, "previous_hash");

    std::string balStr = extractJsonValue(resJson, "unclaimed_pool");
    if (balStr.empty()) balStr = extractJsonValue(resJson, "pool_balance");
    job.unclaimedPool = safeStod(balStr, fallbackUnclaimed);
    return job;
}

void runLocalMining(const std::string& saveFilePath, double& walletBalance, double poolPendingBalance, int& blockIndex, int& difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, const std::string& workerName, const std::string& workerToken, bool authed, const std::string& poolUrl) {
    bool miningSession = true;

    while (miningSession) {
        clearScreen();
        int numThreads = static_cast<int>(std::thread::hardware_concurrency());
        if (numThreads < 1) numThreads = 4;

        std::cout << "[SYSTEM] Initializing Optimized Engine... (" << numThreads << " threads)...\n"
                  << "[SYSTEM] Resuming Block #" << blockIndex << " | Difficulty: " << difficulty << "/20\n"
                  << "[SYSTEM] Press [ENTER] at any time to abort.\n\n";
        flushOut();

        std::atomic<bool> stopMining(false);
        std::atomic<long long> globalNonce(0);
        std::atomic<bool> foundMatch(false);
        std::atomic<long long> nextLogNonce(outputRate);

        long long winningNonce = -1;
        std::string winningHash = "";
        std::mutex coutMutex;

        std::string previousHash = simulateHash(std::to_string(GetTickCount64()));
        std::string basePrefix = previousHash + ":Block#" + std::to_string(blockIndex) + ":Nonce=";

        uint32_t base_state1 = 2166136261UL;
        uint32_t base_state2 = 3382945747UL;
        for (char c : basePrefix) {
            uint32_t val = static_cast<uint32_t>(c);
            base_state1 = computeASICStep32(base_state1, val);
            base_state2 = computeASICStep32(base_state2, val * 31);
        }

        auto blockStartTime = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> workers;
        workers.reserve(numThreads);

        for (int t = 0; t < numThreads; ++t) {
            workers.emplace_back([&, base_state1, base_state2]() {
                const long long batchSize = 50000;
                while (!stopMining && !foundMatch) {
                    long long startNonce = globalNonce.fetch_add(batchSize, std::memory_order_relaxed);
                    long long endNonce = startNonce + batchSize;

                    for (long long currentNonce = startNonce; currentNonce < endNonce && !stopMining && !foundMatch; ++currentNonce) {
                        uint32_t nonceLow = static_cast<uint32_t>(currentNonce & 0xFFFFFFFF);
                        uint32_t nonceHigh = static_cast<uint32_t>((currentNonce >> 32) & 0xFFFFFFFF);

                        uint32_t state1 = computeASICStep32(base_state1, nonceLow);
                        uint32_t state2 = computeASICStep32(base_state2, nonceHigh ^ nonceLow ^ 0x9e3779b9UL);

                        state1 ^= (state2 << 11);
                        state2 ^= (state1 >> 13);

                        uint32_t part1 = state1 ^ (state2 >> 3);
                        uint32_t part2 = state2 ^ (state1 << 3);
                        uint32_t part3 = state1 * 3;
                        uint32_t part4 = state2 * 5;

                        if (checkDifficultyMatch(part1, part2, part3, part4, difficulty)) {
                            bool expected = false;
                            if (foundMatch.compare_exchange_strong(expected, true)) {
                                winningNonce = currentNonce;
                                winningHash = formatHashToString(state1, state2);
                            }
                            break;
                        }

                        if (currentNonce >= nextLogNonce.load(std::memory_order_relaxed)) {
                            long long targetLog = nextLogNonce.load(std::memory_order_relaxed);
                            if (currentNonce >= targetLog) {
                                if (nextLogNonce.compare_exchange_strong(targetLog, targetLog + outputRate)) {
                                    std::string currentHashStr = formatHashToString(state1, state2);
                                    std::lock_guard<std::mutex> lock(coutMutex);
                                    std::cout << "nonce=" << COLOR_YELLOW << targetLog << COLOR_RESET
                                              << " hash=" << COLOR_RED << currentHashStr << COLOR_RESET << "\n" << std::flush;
                                }
                            }
                        }
                    }
                }
            });
        }

        while (!foundMatch && !stopMining) {
            if (checkUserPressedEnter()) {
                stopMining = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        stopMining = true;
        for (auto& th : workers) {
            if (th.joinable()) th.join();
        }

        if (foundMatch) {
            auto blockEndTime = std::chrono::high_resolution_clock::now();
            double durationSec = std::chrono::duration<double>(blockEndTime - blockStartTime).count();
            double dynamicReward = 0.000005 * std::pow(1.6, difficulty - 1);

            std::cout << "\n----------------------------------------\n"
                      << "[" << getCurrentTime() << "] [Block #" << blockIndex << "] MATCHED (DIFFICULTY " << difficulty << "/20)!\n"
                      << "  > winning_nonce : " << COLOR_YELLOW << winningNonce << COLOR_RESET << "\n"
                      << "  > target_hash   : " << COLOR_GREEN << winningHash << COLOR_RESET << "\n"
                      << "  > time_taken    : " << std::fixed << std::setprecision(2) << durationSec << " s\n"
                      << "  > reward_added  : +" << std::fixed << std::setprecision(8) << dynamicReward << " BTC (Exponential Tier)\n"
                      << "----------------------------------------\n";
            flushOut();

            walletBalance += dynamicReward;
            blockIndex++;

            if (autoDifficulty) {
                if (durationSec < 0.8) difficulty = (std::min)(20, difficulty + 4);
                else if (durationSec < 2.5) difficulty = (std::min)(20, difficulty + 2);
                else if (durationSec < 7.0) difficulty++;
                else if (durationSec > 22.0 && difficulty > 3) difficulty = (std::max)(3, difficulty - 2);
            }

            saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);

            if (inspectionWait) {
                std::cout << "[WAIT] 5s Inspection...\n\n";
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }

            if (!autoMineNext) {
                waitForEnter("Press [ENTER] to return to menu...");
                miningSession = false;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        } else {
            std::cout << "\n[ABORT] Interrupted by user. Returning to menu...\n";
            flushOut();
            saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            miningSession = false;
        }
    }
}

void runPoolMining(const std::string& saveFilePath, double& walletBalance, double& poolPendingBalance, int& blockIndex, int& difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, const std::string& workerName, const std::string& workerToken, bool& authed, std::string& poolUrl) {
    if (!authed || workerToken.empty()) {
        std::cout << "\n" << COLOR_RED << "Could not authorize the user, please try again.\n" << COLOR_RESET;
        flushOut();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }

    if (poolUrl.empty()) {
        std::cout << "\nEnter Pool URL (e.g., https://pool.wispbyte.org): ";
        flushOut();
        if (!(std::cin >> poolUrl)) {
            discardRestOfLine();
            return;
        }
        discardRestOfLine();
        while (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
        saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
    }

    bool poolSession = true;
    bool nextRequestExhausted = false;

    while (poolSession) {
        clearScreen();
        int numThreads = static_cast<int>(std::thread::hardware_concurrency());
        if (numThreads < 1) numThreads = 4;

        PoolJob job = requestPoolJob(poolUrl, workerName, workerToken, blockIndex, difficulty, poolPendingBalance, nextRequestExhausted);
        nextRequestExhausted = false;

        if (job.unauthorized) {
            std::cout << COLOR_RED << job.message << "\n" << COLOR_RESET;
            flushOut();
            authed = false;
            saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            break;
        }

        if (!job.valid) {
            std::cout << COLOR_RED << "[ERROR] Cannot reach Pool Host at " << poolUrl << ". Retrying in 3s...\n" << COLOR_RESET;
            flushOut();
            if (checkUserPressedEnter()) break;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }

        long long workerId = job.workerId;
        long long chunkMin = job.chunkMin;
        long long chunkMax = job.chunkMax;
        int poolBlockIndex = job.blockIndex;
        int poolDifficulty = job.difficulty;
        std::string pPrevHash = job.previousHash;
        poolPendingBalance = job.unclaimedPool;

        std::cout << "[SYSTEM] Pool Engine Active... (ID: " << workerId << " | Chunk: [" << chunkMin << ".." << chunkMax << ") | Threads: " << numThreads << ")...\n"
                  << "[SYSTEM] Mining Pool Block #" << poolBlockIndex << " | Difficulty: " << poolDifficulty << "/20 | Unclaimed: " << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPendingBalance << " BTC" << COLOR_RESET << "\n"
                  << "[SYSTEM] Press [ENTER] at any time to abort.\n\n";
        flushOut();

        std::atomic<bool> stopMining(false);
        std::atomic<long long> globalNonce(chunkMin);
        std::atomic<bool> foundMatch(false);
        std::atomic<bool> chunkExhausted(false);
        std::atomic<long long> nextLogNonce(chunkMin + outputRate);

        long long winningNonce = -1;
        std::string winningHash = "";
        std::mutex coutMutex;

        std::string basePrefix = pPrevHash + ":Block#" + std::to_string(poolBlockIndex) + ":Nonce=";
        uint32_t base_state1 = 2166136261UL;
        uint32_t base_state2 = 3382945747UL;
        for (char c : basePrefix) {
            uint32_t val = static_cast<uint32_t>(c);
            base_state1 = computeASICStep32(base_state1, val);
            base_state2 = computeASICStep32(base_state2, val * 31);
        }

        auto blockStartTime = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> workers;
        workers.reserve(numThreads);

        for (int t = 0; t < numThreads; ++t) {
            workers.emplace_back([&, base_state1, base_state2, chunkMax]() {
                const long long batchSize = 50000;
                while (!stopMining && !foundMatch && !chunkExhausted.load(std::memory_order_relaxed)) {
                    long long startNonce = globalNonce.fetch_add(batchSize, std::memory_order_relaxed);
                    if (startNonce >= chunkMax) {
                        chunkExhausted.store(true, std::memory_order_relaxed);
                        break;
                    }
                    long long endNonce = (std::min)(startNonce + batchSize, chunkMax);

                    for (long long currentNonce = startNonce; currentNonce < endNonce && !stopMining && !foundMatch; ++currentNonce) {
                        uint32_t nonceLow = static_cast<uint32_t>(currentNonce & 0xFFFFFFFF);
                        uint32_t nonceHigh = static_cast<uint32_t>((currentNonce >> 32) & 0xFFFFFFFF);

                        uint32_t state1 = computeASICStep32(base_state1, nonceLow);
                        uint32_t state2 = computeASICStep32(base_state2, nonceHigh ^ nonceLow ^ 0x9e3779b9UL);

                        state1 ^= (state2 << 11);
                        state2 ^= (state1 >> 13);

                        uint32_t part1 = state1 ^ (state2 >> 3);
                        uint32_t part2 = state2 ^ (state1 << 3);
                        uint32_t part3 = state1 * 3;
                        uint32_t part4 = state2 * 5;

                        if (checkDifficultyMatch(part1, part2, part3, part4, poolDifficulty)) {
                            bool expected = false;
                            if (foundMatch.compare_exchange_strong(expected, true)) {
                                winningNonce = currentNonce;
                                winningHash = formatHashToString(state1, state2);
                            }
                            break;
                        }

                        if (currentNonce >= nextLogNonce.load(std::memory_order_relaxed)) {
                            long long targetLog = nextLogNonce.load(std::memory_order_relaxed);
                            if (currentNonce >= targetLog) {
                                if (nextLogNonce.compare_exchange_strong(targetLog, targetLog + outputRate)) {
                                    std::string currentHashStr = formatHashToString(state1, state2);
                                    std::lock_guard<std::mutex> lock(coutMutex);
                                    std::cout << "nonce=" << COLOR_YELLOW << targetLog << COLOR_RESET
                                              << " hash=" << COLOR_RED << currentHashStr << COLOR_RESET << "\n" << std::flush;
                                }
                            }
                        }
                    }
                }
            });
        }

        auto lastSyncCheck = std::chrono::steady_clock::now();
        bool serverBlockAdvanced = false;

        while (!foundMatch && !stopMining && !chunkExhausted.load(std::memory_order_relaxed)) {
            if (checkUserPressedEnter()) {
                stopMining = true;
                break;
            }

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastSyncCheck).count() >= 1) {
                lastSyncCheck = now;
                std::string statusJson = httpGetJson(poolUrl + "/?worker=" + urlEncode(workerName), 2);
                if (!statusJson.empty()) {
                    std::string rBlk = extractJsonValue(statusJson, "block");
                    if (rBlk.empty()) rBlk = extractJsonValue(statusJson, "block_index");
                    int remoteBlock = safeStoi(rBlk, poolBlockIndex);
                    int remoteDiff = safeStoi(extractJsonValue(statusJson, "difficulty"), poolDifficulty);
                    if (remoteBlock > poolBlockIndex || remoteDiff != poolDifficulty) {
                        serverBlockAdvanced = true;
                        stopMining = true;
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        stopMining = true;
        for (auto& th : workers) {
            if (th.joinable()) th.join();
        }

        if (serverBlockAdvanced) {
            std::cout << "\n" << COLOR_CYAN << "[SYNC] Detected new block/diff from pool. Requesting new chunk..." << COLOR_RESET << "\n";
            flushOut();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        if (foundMatch) {
            auto blockEndTime = std::chrono::high_resolution_clock::now();
            double durationSec = std::chrono::duration<double>(blockEndTime - blockStartTime).count();

            std::ostringstream payload;
            payload << "{"
                    << "\"worker\":\"" << escapeJsonString(workerName) << "\","
                    << "\"token\":\"" << escapeJsonString(workerToken) << "\","
                    << "\"block_index\":" << poolBlockIndex << ","
                    << "\"winning_nonce\":" << winningNonce << ","
                    << "\"duration_sec\":" << std::fixed << std::setprecision(6) << durationSec
                    << "}";
            std::string submitRes = httpPostJson(poolUrl + "/submit", payload.str(), 8);

            std::string status = toLowerCopy(cleanJsonString(extractJsonValue(submitRes, "status")));
            double dynamicReward = 0.000005 * std::pow(1.6, poolDifficulty - 1);

            if (status == "accepted" || isJsonSuccessStatus(submitRes)) {
                std::string sBal = extractJsonValue(submitRes, "unclaimed_pool");
                if (sBal.empty()) sBal = extractJsonValue(submitRes, "pool_balance");
                poolPendingBalance = safeStod(sBal, poolPendingBalance);

                std::cout << "\n----------------------------------------\n"
                          << "[" << getCurrentTime() << "] [Block #" << poolBlockIndex << "] MATCHED BY [" << COLOR_CYAN << workerName << COLOR_RESET << "] (DIFFICULTY " << poolDifficulty << "/20)!\n"
                          << "  > winning_nonce : " << COLOR_YELLOW << winningNonce << COLOR_RESET << "\n"
                          << "  > target_hash   : " << COLOR_GREEN << winningHash << COLOR_RESET << "\n"
                          << "  > time_taken    : " << std::fixed << std::setprecision(2) << durationSec << " s\n"
                          << "  > reward_added  : +" << std::fixed << std::setprecision(8) << dynamicReward << " BTC (Pool Credited)\n"
                          << "  > pool_balance  : " << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPendingBalance << " BTC" << COLOR_RESET << "\n"
                          << "----------------------------------------\n";
            } else {
                std::cout << "\n" << COLOR_RED << "[REJECTED] Server response: " << (submitRes.empty() ? "Invalid/Empty JSON Response" : submitRes) << COLOR_RESET << "\n";
            }
            flushOut();

            saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);

            if (inspectionWait) {
                std::cout << "[WAIT] 5s Inspection...\n\n";
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }

            if (!autoMineNext) {
                waitForEnter("Press [ENTER] to return to menu...");
                poolSession = false;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        } else if (chunkExhausted.load(std::memory_order_relaxed)) {
            nextRequestExhausted = true;
            std::cout << "\n" << COLOR_CYAN << "[CHUNK] Completed [" << chunkMin << ".." << chunkMax << "). Requesting next job from pool..." << COLOR_RESET << "\n";
            flushOut();
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            continue;
        } else {
            std::cout << "\n[ABORT] Interrupted by user. Returning to menu...\n";
            flushOut();
            saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            poolSession = false;
        }
    }
}

void runStandardOptions(const std::string& saveFilePath, double& walletBalance, double poolPendingBalance, int& blockIndex, int& difficulty, bool& autoMineNext, bool& autoDifficulty, long long& targetHashrate, long long& outputRate, bool& isBanned, bool isSpeedModActive, bool isHackerModActive, bool& inspectionWait, const std::string& workerName, const std::string& workerToken, bool authed, const std::string& poolUrl) {
    int optChoice = 0;
    while (true) {
        clearScreen();
        std::cout << "> miner_options\n"
                  << "----------------------------------------\n";

        int idx = 1;
        int optMap[10];

        if (isHackerModActive) {
            std::cout << "  [" << idx << "] set_difficulty : " << difficulty << " [MOD]\n";
            optMap[idx++] = 1;
            std::cout << "  [" << idx << "] auto_difficulty: " << (autoDifficulty ? "ON" : "OFF") << " [MOD]\n";
            optMap[idx++] = 2;
        }

        std::cout << "  [" << idx << "] auto_mine_next : " << (autoMineNext ? "ON" : "OFF") << "\n";
        optMap[idx++] = 3;

        std::cout << "  [" << idx << "] inspection_wait: " << (inspectionWait ? "ON" : "OFF") << "\n";
        optMap[idx++] = 4;

        std::cout << "  [" << idx << "] output_rate    : " << outputRate << " nonces/log\n";
        optMap[idx++] = 5;

        if (isSpeedModActive) {
            std::cout << "  [" << idx << "] set_hashrate   : UNLIMITED (INF) [MOD]\n";
            optMap[idx++] = 6;
        }

        std::cout << "  [" << idx << "] back_to_main\n";
        int backChoiceId = idx;
        std::cout << "----------------------------------------\n"
                  << "root@miner/options# ";
        flushOut();

        if (!(std::cin >> optChoice)) {
            discardRestOfLine();
            continue;
        }
        discardRestOfLine();

        if (optChoice == backChoiceId) break;

        if (optChoice >= 1 && optChoice < backChoiceId) {
            int selectedAction = optMap[optChoice];
            if (selectedAction == 1) {
                std::cout << "Enter difficulty (1-20): ";
                flushOut();
                if (std::cin >> difficulty) {
                    discardRestOfLine();
                    if (difficulty < 1) difficulty = 1;
                    if (difficulty > 20) difficulty = 20;
                } else {
                    discardRestOfLine();
                }
            } else if (selectedAction == 2) {
                autoDifficulty = !autoDifficulty;
            } else if (selectedAction == 3) {
                autoMineNext = !autoMineNext;
            } else if (selectedAction == 4) {
                inspectionWait = !inspectionWait;
            } else if (selectedAction == 5) {
                std::cout << "Enter output rate (log interval): ";
                flushOut();
                if (std::cin >> outputRate) {
                    discardRestOfLine();
                    if (outputRate < 1000) outputRate = 1000;
                } else {
                    discardRestOfLine();
                }
            } else if (selectedAction == 6) {
                std::cout << "[MOD] Hashrate is locked to UNLIMITED (INF)!\n";
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
    }
}

void runPoolOptions(const std::string& saveFilePath, double& walletBalance, double& poolPendingBalance, int blockIndex, int difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, std::string& workerName, std::string& workerToken, bool& authed, std::string& poolUrl) {
    int pChoice = 0;
    while (true) {
        if (!poolUrl.empty() && !workerName.empty()) {
            std::string uncRes = httpGetJson(poolUrl + "/unclaimed?worker=" + urlEncode(workerName), 4);
            if (!uncRes.empty()) {
                std::string uStr = extractJsonValue(uncRes, "unclaimed_pool");
                if (uStr.empty()) uStr = extractJsonValue(uncRes, "pool_balance");
                if (!uStr.empty()) poolPendingBalance = safeStod(uStr, poolPendingBalance);
            }
        }
        clearScreen();
        int idx = 1;
        int optSetWorker = -1, optSetUrl = -1, optClaim = -1, optBack = -1;

        std::cout << "> pool_options\n"
                  << "----------------------------------------\n";
        std::cout << "  [" << idx << "] set_worker" << (authed ? " (re-auth/new)" : "") << "\n";
        optSetWorker = idx++;

        std::cout << "  [" << idx << "] set_pool_url : " << (poolUrl.empty() ? "None" : poolUrl) << "\n";
        optSetUrl = idx++;

        std::cout << "  [" << idx << "] claim_pool\n";
        optClaim = idx++;

        std::cout << "  [" << idx << "] back_to_main\n";
        optBack = idx++;

        std::cout << "----------------------------------------\n"
                  << "root@miner/pool_options# ";
        flushOut();

        if (!(std::cin >> pChoice)) {
            discardRestOfLine();
            continue;
        }
        discardRestOfLine();

        if (pChoice == optBack) break;

        if (pChoice == optSetWorker) {
            if (poolUrl.empty()) {
                std::cout << COLOR_RED << "[ERROR] Configure Pool URL first!\n" << COLOR_RESET;
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            std::cout << "\n  [1] Create\n"
                      << "  [2] Auth\n"
                      << "Select: ";
            flushOut();
            int subMode = 0;
            if (!(std::cin >> subMode)) {
                discardRestOfLine();
                continue;
            }
            discardRestOfLine();

            if (subMode == 1) {
                std::string inputName, inputToken;
                std::cout << "Set name: ";
                flushOut();
                if (!(std::cin >> inputName)) {
                    discardRestOfLine();
                    continue;
                }
                discardRestOfLine();

                std::string chkRes = httpGetJson(poolUrl + "/check_name?worker=" + urlEncode(inputName), 5);
                if (chkRes.empty()) {
                    std::cout << COLOR_RED << "[ERROR] Cannot reach Pool Host or invalid JSON at " << poolUrl << "\n" << COLOR_RESET;
                    flushOut();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }
                std::string availStr = toLowerCopy(cleanJsonString(extractJsonValue(chkRes, "available")));
                if (availStr != "true" && availStr != "1") {
                    std::string errMsg = cleanJsonString(extractJsonValue(chkRes, "message"));
                    std::cout << COLOR_RED << (errMsg.empty() ? "Name is already taken by another worker." : errMsg) << "\n" << COLOR_RESET;
                    flushOut();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }

                std::cout << "Set token: ";
                flushOut();
                if (!(std::cin >> inputToken)) {
                    discardRestOfLine();
                    continue;
                }
                discardRestOfLine();

                std::string payload = "{\"worker\":\"" + escapeJsonString(inputName) + "\",\"token\":\"" + escapeJsonString(inputToken) + "\"}";
                std::string regRes = httpPostJson(poolUrl + "/register_worker", payload, 8);
                if (regRes.empty()) {
                    std::cout << COLOR_RED << "[ERROR] Invalid or empty JSON from /register_worker.\n" << COLOR_RESET;
                } else if (isJsonSuccessStatus(regRes)) {
                    workerName = inputName;
                    workerToken = inputToken;
                    authed = true;
                    std::string bStr = extractJsonValue(regRes, "unclaimed_pool");
                    if (bStr.empty()) bStr = extractJsonValue(regRes, "pool_balance");
                    poolPendingBalance = safeStod(bStr, 0.0);
                    std::cout << COLOR_GREEN << "[OK] Created & authed worker [" << workerName << "].\n" << COLOR_RESET;
                } else {
                    std::string errMsg = cleanJsonString(extractJsonValue(regRes, "message"));
                    if (errMsg.empty()) errMsg = cleanJsonString(extractJsonValue(regRes, "error"));
                    std::cout << COLOR_RED << (errMsg.empty() ? "Name is already taken by another worker." : errMsg) << "\n" << COLOR_RESET;
                }
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else if (subMode == 2) {
                std::string inputName, inputToken;
                std::cout << "Name: ";
                flushOut();
                if (!(std::cin >> inputName)) {
                    discardRestOfLine();
                    continue;
                }
                discardRestOfLine();

                std::cout << "Token: ";
                flushOut();
                if (!(std::cin >> inputToken)) {
                    discardRestOfLine();
                    continue;
                }
                discardRestOfLine();

                std::string payload = "{\"worker\":\"" + escapeJsonString(inputName) + "\",\"token\":\"" + escapeJsonString(inputToken) + "\"}";
                std::string authRes = httpPostJson(poolUrl + "/auth_worker", payload, 8);
                if (authRes.empty()) {
                    std::cout << COLOR_RED << "[ERROR] Invalid or empty JSON from /auth_worker.\n" << COLOR_RESET;
                } else if (isJsonSuccessStatus(authRes)) {
                    workerName = inputName;
                    workerToken = inputToken;
                    authed = true;
                    std::string bStr = extractJsonValue(authRes, "unclaimed_pool");
                    if (bStr.empty()) bStr = extractJsonValue(authRes, "pool_balance");
                    poolPendingBalance = safeStod(bStr, poolPendingBalance);
                    std::cout << COLOR_GREEN << "[OK] Authenticated [" << workerName << "] | Unclaimed: " << std::fixed << std::setprecision(8) << poolPendingBalance << " BTC\n" << COLOR_RESET;
                } else {
                    std::string errMsg = cleanJsonString(extractJsonValue(authRes, "message"));
                    if (errMsg.empty()) errMsg = cleanJsonString(extractJsonValue(authRes, "error"));
                    std::cout << COLOR_RED << "[ERROR] Auth failed: " << (errMsg.empty() ? "invalid name or token." : errMsg) << "\n" << COLOR_RESET;
                }
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        } else if (pChoice == optSetUrl) {
            std::cout << "Enter Pool URL: ";
            flushOut();
            if (std::cin >> poolUrl) {
                discardRestOfLine();
                while (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
            } else {
                discardRestOfLine();
            }
        } else if (pChoice == optClaim) {
            if (poolUrl.empty()) {
                std::cout << COLOR_RED << "[ERROR] Configure Pool URL first!\n" << COLOR_RESET;
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                std::string payload = "{\"worker\":\"" + escapeJsonString(workerName) + "\",\"token\":\"" + escapeJsonString(workerToken) + "\"}";
                std::string res = httpPostJson(poolUrl + "/claim_distribution", payload, 8);
                std::string status = toLowerCopy(cleanJsonString(extractJsonValue(res, "status")));
                if (status == "claimed" || isJsonSuccessStatus(res)) {
                    double claimed = safeStod(extractJsonValue(res, "claimed"), 0.0);
                    walletBalance += claimed;
                    poolPendingBalance = 0.0;
                    std::cout << COLOR_GREEN << "[SUCCESS] Claimed +" << std::fixed << std::setprecision(8) << claimed << " BTC to local wallet!\n" << COLOR_RESET;
                } else if (status == "unauthorized") {
                    authed = false;
                    std::cout << COLOR_RED << "Could not authorize the user, please try again.\n" << COLOR_RESET;
                } else {
                    std::cout << COLOR_YELLOW << "[INFO] No pool balance available to claim.\n" << COLOR_RESET;
                }
                flushOut();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }

        saveGameState(saveFilePath, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
    }
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    enableAnsiColors();

    AppPaths paths = getAppPaths();
    if (!verifyExeInsideBaseDir(paths.baseDir)) {
        clearScreen();
        std::cout << COLOR_RED << "[ERROR] miner.exe must be placed inside:\n"
                  << paths.baseDir << "\n"
                  << "before running for the first time.\n" << COLOR_RESET;
        waitForEnter("\nPress ENTER to exit...");
        return 1;
    }

    try {
        std::filesystem::create_directories(paths.modsDir);
    } catch (...) {}

    int difficulty = 3;
    bool autoMineNext = true;
    bool inspectionWait = true;
    bool autoDifficulty = true;
    long long targetHashrate = 2500000;
    long long outputRate = 50000;
    double walletBalance = 1.0;
    double poolPendingBalance = 0.0;
    int blockIndex = 1;
    bool isBanned = false;
    std::string workerName = "Node_01";
    std::string workerToken = "";
    bool authed = false;
    std::string poolUrl = "";

    bool isSpeedModActive = verifyMod(paths.speedMod, SPEED_TOKEN);
    bool isHackerModActive = verifyMod(paths.hackerMod, HACKER_TOKEN);
    bool isPoolModActive = verifyMod(paths.poolMod, POOL_TOKEN);

    loadGameState(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);

    if (isBanned) {
        clearScreen();
        std::cout << COLOR_RED << "access denied\n"
                  << "Your config file has been found as compromised/modified illegally, programme locked permanently and your data has been wiped. To continue, delete the existing configs file.\n"
                  << COLOR_RESET << "\n";
        waitForEnter("Press ENTER to exit program...");
        return 0;
    }

    int choice = 0;
    while (true) {
        clearScreen();
        int hardwareCores = static_cast<int>(std::thread::hardware_concurrency());
        if (hardwareCores < 1) hardwareCores = 4;

        if (!isPoolModActive) {
            std::cout << "> miner_main_menu\n"
                      << "----------------------------------------\n"
                      << "  [1] start_mining\n"
                      << "  [2] options\n"
                      << "  [3] quit\n"
                      << "----------------------------------------\n"
                      << "wallet_balance: " << std::fixed << std::setprecision(8) << walletBalance << " BTC\n"
                      << "current_hash  : " << (isSpeedModActive ? "UNLIMITED [RAW SPEED MOD]" : std::to_string(targetHashrate) + " H/s") << "\n"
                      << "cpu_threads   : " << hardwareCores << " Cores (Native Optimized)\n"
                      << "current_block : #" << blockIndex << "\n"
                      << (isSpeedModActive ? COLOR_CYAN "[MOD LOADER] Active: unlimited_speed.mod detected!\n" : "")
                      << (isHackerModActive ? COLOR_MAGENTA "[MOD LOADER] Active: hacker.mod detected (Admin Unlocked)!\n" : "")
                      << COLOR_RESET
                      << "root@miner:~# ";
            flushOut();

            if (!(std::cin >> choice)) {
                discardRestOfLine();
                continue;
            }
            discardRestOfLine();

            if (choice == 3) {
                std::cout << "Terminating process...\n";
                flushOut();
                saveGameState(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
                break;
            } else if (choice == 2) {
                runStandardOptions(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            } else if (choice == 1) {
                runLocalMining(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            }
        } else {
            std::cout << "> miner_main_menu\n"
                      << "----------------------------------------\n"
                      << "  [1] solo_mining\n"
                      << "  [2] pool_mining\n"
                      << "  [3] options\n"
                      << "  [4] pool_options\n"
                      << "  [5] quit\n"
                      << "----------------------------------------\n"
                      << "worker_id     : " << COLOR_CYAN << workerName << COLOR_RESET << "\n"
                      << "pool_host     : " << (poolUrl.empty() ? "Not Configured" : poolUrl) << "\n"
                      << "wallet_balance: " << std::fixed << std::setprecision(8) << walletBalance << " BTC\n"
                      << "pool_unclaimed: " << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPendingBalance << " BTC" << COLOR_RESET << "\n"
                      << "current_hash  : " << (isSpeedModActive ? "UNLIMITED [RAW SPEED MOD]" : std::to_string(targetHashrate) + " H/s") << "\n"
                      << "cpu_threads   : " << hardwareCores << " Cores (Native Optimized)\n"
                      << "current_block : #" << blockIndex << "\n"
                      << (isSpeedModActive ? COLOR_CYAN "[MOD LOADER] Active: unlimited_speed.mod detected!\n" : "")
                      << (isHackerModActive ? COLOR_MAGENTA "[MOD LOADER] Active: hacker.mod detected (Admin Unlocked)!\n" : "")
                      << COLOR_YELLOW "[MOD LOADER] Active: pool_mining.mod detected!\n"
                      << COLOR_RESET
                      << "root@miner:~# ";
            flushOut();

            if (!(std::cin >> choice)) {
                discardRestOfLine();
                continue;
            }
            discardRestOfLine();

            if (choice == 5) {
                std::cout << "Terminating process...\n";
                flushOut();
                saveGameState(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
                break;
            } else if (choice == 4) {
                runPoolOptions(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            } else if (choice == 3) {
                runStandardOptions(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            } else if (choice == 2) {
                runPoolMining(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            } else if (choice == 1) {
                runLocalMining(paths.saveFile, walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
            }
        }
        clearScreen();
    }
    return 0;
}
