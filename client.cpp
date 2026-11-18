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
#include <cctype>
#include <unistd.h>
#include <poll.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"

const std::string BASE_DIR        = "/storage/emulated/0/crypto_miner";
const std::string MODS_DIR        = BASE_DIR + "/mods";
const std::string SAVE_FILE_PATH  = BASE_DIR + "/financial_report.json";
const std::string SPEED_MOD_PATH  = MODS_DIR + "/unlimited_speed.mod";
const std::string HACKER_MOD_PATH = MODS_DIR + "/hacker.mod";
const std::string POOL_MOD_PATH   = MODS_DIR + "/pool_mining.mod";

const std::string SPEED_TOKEN  = "tk_756e6e696d697465647370656564";
const std::string HACKER_TOKEN = "KHAI_HACKER_ADMIN_2026";
const std::string POOL_TOKEN   = "tk_706f6f6c6d696e696e67";

struct WorkConfig {
    long long index = 1;
    int diff = 3;
    int epoch = 1;
    std::string prevHash = "";
};

void clearScreen() {
    system("clear");
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_c);
    std::ostringstream oss;
    oss << std::put_time(local_time, "%H:%M:%S");
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

std::string execCurl(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
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
    } else {
        size_t endPos = json.find_first_not_of("0123456789.-eE+", pos);
        if (endPos == std::string::npos) endPos = json.length();
        std::string val = json.substr(pos, endPos - pos);
        size_t first = val.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = val.find_last_not_of(" \t\r\n");
        return val.substr(first, (last - first + 1));
    }
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

inline bool checkDifficultyMatch(uint32_t part1, uint32_t part2, uint32_t part3, uint32_t part4, int diff) {
    if (diff <= 8) {
        if (diff == 8) return part1 == 0;
        if (part1 == 0) return true;
        int clz = __builtin_clz(part1);
        return (clz >= diff * 4);
    } else if (diff <= 16) {
        if (part1 != 0) return false;
        int rem = diff - 8;
        if (rem == 8) return part2 == 0;
        if (part2 == 0) return true;
        int clz = __builtin_clz(part2);
        return (clz >= rem * 4);
    } else if (diff <= 24) {
        if (part1 != 0 || part2 != 0) return false;
        int rem = diff - 16;
        if (rem == 8) return part3 == 0;
        if (part3 == 0) return true;
        int clz = __builtin_clz(part3);
        return (clz >= rem * 4);
    } else {
        if (part1 != 0 || part2 != 0 || part3 != 0) return false;
        int rem = diff - 24;
        if (rem == 8) return part4 == 0;
        if (part4 == 0) return true;
        int clz = __builtin_clz(part4);
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

std::string generateRandomHash() {
    return simulateHash(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
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
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    pfd.revents = 0;
    if (poll(&pfd, 1, 0) > 0) {
        char c;
        while (read(STDIN_FILENO, &c, 1) > 0 && c != '\n' && c != '\r');
        return true;
    }
    return false;
}

unsigned long long calculateChecksum(double balance, int blockIdx, int diff, bool isBanned, long long targetHashrate, bool speedActive, bool hackerActive, bool inspectionWait) {
    unsigned long long secretKey = 0x53414C54ULL;
    long long satoshis = static_cast<long long>(balance * 100000000.0 + 0.5);
    return (satoshis ^ (blockIdx * 31) ^ (diff * 17) ^ (targetHashrate * 7) ^ (speedActive ? 0x12345678ULL : 0) ^ (hackerActive ? 0xABCDEF01ULL : 0) ^ (inspectionWait ? 0x99999999ULL : 0) ^ (isBanned ? 0xFFFFFFFFFFFFFFFFULL : 0)) + secretKey;
}

void saveGameState(double balance, int blockIdx, int diff, bool autoMine, bool autoDiff, long long targetHashrate, long long outputRate, bool isBanned, bool speedActive, bool hackerActive, bool inspectionWait, const std::string& workerName, std::string poolUrl) {
    if (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
    unsigned long long checksum = calculateChecksum(balance, blockIdx, diff, isBanned, targetHashrate, speedActive, hackerActive, inspectionWait);
    std::ostringstream buffer;
    buffer << "{\n"
           << "  \"document_title\": \"Q3 Financial Summary & Asset Audit Report\",\n"
           << "  \"classification\": \"CONFIDENTIAL_INTERNAL\",\n"
           << "  \"worker_name\": \"" << workerName << "\",\n"
           << "  \"pool_url\": \"" << poolUrl << "\",\n"
           << "  \"wallet_balance\": " << std::fixed << std::setprecision(8) << balance << ",\n"
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

    std::ofstream outFile(SAVE_FILE_PATH, std::ios::out | std::ios::trunc);
    if (outFile.is_open()) {
        outFile << buffer.str();
        outFile.flush();
        outFile.close();
    }
}

void loadGameState(double& balance, int& blockIdx, int& diff, bool& autoMine, bool& autoDiff, long long& targetHashrate, long long& outputRate, bool& isBanned, bool speedActive, bool hackerActive, bool& inspectionWait, std::string& workerName, std::string& poolUrl) {
    std::ifstream inFile(SAVE_FILE_PATH);
    if (!inFile.is_open()) {
        saveGameState(balance, blockIdx, diff, autoMine, autoDiff, targetHashrate, outputRate, isBanned, speedActive, hackerActive, inspectionWait, workerName, poolUrl);
        return;
    }
    std::string line;
    unsigned long long loadedChecksum = 0;
    bool checksumFound = false;

    while (std::getline(inFile, line)) {
        try {
            auto parse = [&](const std::string& key) -> std::string {
                std::string exactKey = "\"" + key + "\"";
                size_t p = line.find(exactKey);
                if (p != std::string::npos) {
                    size_t c = line.find(':', p);
                    if (c != std::string::npos) {
                        std::string v = line.substr(c + 1);
                        size_t first = v.find_first_not_of(" \t\r\n\"");
                        if (first == std::string::npos) return "";
                        size_t last = v.find_last_not_of(" \t\r\n\",");
                        return v.substr(first, (last - first + 1));
                    }
                }
                return "";
            };

            std::string v;
            if ((v = parse("wallet_balance")) != "") balance = safeStod(v, balance);
            else if ((v = parse("worker_name")) != "") workerName = v;
            else if ((v = parse("pool_url")) != "") poolUrl = v;
            else if ((v = parse("block_index")) != "") blockIdx = safeStoi(v, blockIdx);
            else if ((v = parse("difficulty")) != "") diff = safeStoi(v, diff);
            else if ((v = parse("target_hashrate")) != "") targetHashrate = safeStoll(v, targetHashrate);
            else if ((v = parse("output_rate")) != "") outputRate = safeStoll(v, outputRate);
            else if ((v = parse("auto_mine_next")) != "") autoMine = (v == "true");
            else if ((v = parse("inspection_wait")) != "") inspectionWait = (v == "true");
            else if ((v = parse("auto_difficulty")) != "") autoDiff = (v == "true");
            else if ((v = parse("is_banned")) != "") isBanned = (v == "true");
            else if ((v = parse("security_checksum")) != "") {
                loadedChecksum = safeStoull(v, 0);
                checksumFound = true;
            }
        } catch (...) {}
    }
    inFile.close();

    if (checksumFound) {
        unsigned long long expectedChecksum = calculateChecksum(balance, blockIdx, diff, isBanned, targetHashrate, speedActive, hackerActive, inspectionWait);
        if (loadedChecksum != expectedChecksum) {
            isBanned = true;
            saveGameState(balance, blockIdx, diff, autoMine, autoDiff, targetHashrate, outputRate, isBanned, speedActive, hackerActive, inspectionWait, workerName, poolUrl);
        }
    }
}

void runLocalMining(double& walletBalance, int& blockIndex, int& difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, const std::string& workerName, const std::string& poolUrl) {
    bool miningSession = true;

    while (miningSession) {
        clearScreen();
        int numThreads = std::thread::hardware_concurrency();
        if (numThreads < 1) numThreads = 4;

        std::cout << "[SYSTEM] Initializing Optimized Engine... (" << numThreads << " threads)...\n"
                  << "[SYSTEM] Resuming Block #" << blockIndex << " | Difficulty: " << difficulty << "/20\n"
                  << "[SYSTEM] Press [ENTER] at any time to abort.\n\n";

        std::atomic<bool> stopMining(false);
        std::atomic<long long> globalNonce(0);
        std::atomic<bool> foundMatch(false);
        std::atomic<long long> nextLogNonce(outputRate);

        long long winningNonce = -1;
        std::string winningHash = "";
        std::mutex coutMutex;

        auto blockStartTime = std::chrono::high_resolution_clock::now();
        std::string previousHash = generateRandomHash();
        std::string basePrefix = previousHash + ":Block#" + std::to_string(blockIndex) + ":Nonce=";

        uint32_t base_state1 = 2166136261UL;
        uint32_t base_state2 = 3382945747UL;
        for (char c : basePrefix) {
            uint32_t val = static_cast<uint32_t>(c);
            base_state1 = computeASICStep32(base_state1, val);
            base_state2 = computeASICStep32(base_state2, val * 31);
        }

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
                                              << " hash=" << COLOR_RED << currentHashStr << COLOR_RESET << "\n";
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
                      << " > winning_nonce : " << COLOR_YELLOW << winningNonce << COLOR_RESET << "\n"
                      << " > target_hash   : " << COLOR_GREEN << winningHash << COLOR_RESET << "\n"
                      << " > time_taken    : " << std::fixed << std::setprecision(2) << durationSec << " s\n"
                      << " > reward_added  : +" << std::fixed << std::setprecision(8) << dynamicReward << " BTC\n"
                      << "----------------------------------------\n";

            walletBalance += dynamicReward;
            blockIndex++;

            if (autoDifficulty) {
                if (durationSec < 0.8) {
                    difficulty = std::min(20, difficulty + 4);
                } else if (durationSec < 2.5) {
                    difficulty = std::min(20, difficulty + 2);
                } else if (durationSec < 7.0) {
                    difficulty++;
                } else if (durationSec > 22.0 && difficulty > 3) {
                    difficulty -= 2;
                }
            }

            saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);

            if (inspectionWait) {
                std::cout << "[WAIT] 5s Inspection...\n\n";
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }

            if (!autoMineNext) {
                std::cout << "Press [ENTER] to return to menu...";
                std::cin.get();
                miningSession = false;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        } else {
            std::cout << "\n[ABORT] Interrupted by user. Returning to menu...\n";
            saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            miningSession = false;
        }
    }
}

void runPoolMining(double& walletBalance, double& poolPendingBalance, int& blockIndex, int& difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, const std::string& workerName, std::string& poolUrl) {
    if (poolUrl.empty()) {
        std::cout << "\nEnter Pool URL (e.g., http://vicky.hidencloud.com:24698): ";
        std::cin >> poolUrl;
        if (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
        saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
    }

    std::string joinUri = poolUrl + "/join?worker=" + workerName;
    std::string joinJson = execCurl("curl -s -L -A \"MinerClient\" -m 8 \"" + joinUri + "\"");
    
    if (joinJson.empty() || joinJson.find("worker_id") == std::string::npos) {
        std::cout << COLOR_RED << "[ERROR] Cannot reach Pool Host at " << poolUrl << ".\n" << COLOR_RESET;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }

    long long workerId = safeStoll(extractJsonValue(joinJson, "worker_id"), 0);
    long long baseStart = safeStoll(extractJsonValue(joinJson, "base_start"), 0);
    
    WorkConfig activeWork;
    activeWork.index = safeStoi(extractJsonValue(joinJson, "block_index"), 1);
    activeWork.diff = safeStoi(extractJsonValue(joinJson, "difficulty"), 3);
    activeWork.epoch = safeStoi(extractJsonValue(joinJson, "epoch"), 1);
    activeWork.prevHash = extractJsonValue(joinJson, "previous_hash");
    poolPendingBalance = safeStod(extractJsonValue(joinJson, "pool_balance"), 0.0);

    clearScreen();
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads < 1) numThreads = 4;

    std::cout << "[SYSTEM] Zero-Downtime Pool Engine Active... (ID: " << workerId << " | Threads: " << numThreads << ")...\n"
              << "[SYSTEM] Starting at Block #" << activeWork.index << " | Difficulty: " << activeWork.diff << "/20 | Epoch: #" << activeWork.epoch << "\n"
              << "[SYSTEM] Press [ENTER] at any time to abort.\n\n";

    std::atomic<bool> stopMining(false);
    std::atomic<long long> globalNonce(baseStart);
    std::atomic<bool> foundMatch(false);
    std::atomic<int> workVersion(0);
    std::atomic<long long> nextLogNonce(baseStart + outputRate);

    WorkConfig currentWork = activeWork;
    std::mutex workMutex;

    long long winningNonce = -1;
    std::string winningHash = "";
    std::mutex coutMutex;

    auto blockStartTime = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(numThreads);

    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&, t]() {
            const long long batchSize = 50000;
            int localVersion = -1;
            WorkConfig localWork;
            uint32_t base_state1 = 0, base_state2 = 0;

            while (!stopMining) {
                int v = workVersion.load(std::memory_order_acquire);
                if (v != localVersion) {
                    std::lock_guard<std::mutex> lock(workMutex);
                    localWork = currentWork;
                    localVersion = v;

                    std::string basePrefix = localWork.prevHash + ":Block#" + std::to_string(localWork.index) + ":Nonce=";
                    base_state1 = 2166136261UL;
                    base_state2 = 3382945747UL;
                    for (char c : basePrefix) {
                        uint32_t val = static_cast<uint32_t>(c);
                        base_state1 = computeASICStep32(base_state1, val);
                        base_state2 = computeASICStep32(base_state2, val * 31);
                    }
                    globalNonce.store(baseStart, std::memory_order_relaxed);
                    nextLogNonce.store(baseStart + outputRate, std::memory_order_relaxed);
                }

                if (foundMatch.load(std::memory_order_relaxed)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                long long startNonce = globalNonce.fetch_add(batchSize, std::memory_order_relaxed);
                long long endNonce = startNonce + batchSize;

                for (long long currentNonce = startNonce; currentNonce < endNonce && !stopMining && !foundMatch.load(std::memory_order_relaxed); ++currentNonce) {
                    if (workVersion.load(std::memory_order_relaxed) != localVersion) break;

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

                    if (checkDifficultyMatch(part1, part2, part3, part4, localWork.diff)) {
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
                                          << " hash=" << COLOR_RED << currentHashStr << COLOR_RESET << "\n";
                            }
                        }
                    }
                }
            }
        });
    }

    auto lastSyncCheck = std::chrono::steady_clock::now();
    bool sessionActive = true;

    while (sessionActive) {
        if (checkUserPressedEnter()) {
            stopMining = true;
            break;
        }

        if (foundMatch.load()) {
            auto blockEndTime = std::chrono::high_resolution_clock::now();
            double durationSec = std::chrono::duration<double>(blockEndTime - blockStartTime).count();

            long long sIndex;
            int sDiff;
            {
                std::lock_guard<std::mutex> lock(workMutex);
                sIndex = currentWork.index;
                sDiff = currentWork.diff;
            }

            std::string payload = "{\"worker\":\"" + workerName + "\",\"block_index\":" + std::to_string(sIndex) + ",\"winning_nonce\\":" + std::to_string(winningNonce) + ",\"duration_sec\":" + std::to_string(durationSec) + "}";
            // Trên Linux/Termux dùng nháy kép bình thường không cần escape phức tạp như Windows
            std::string curlCmd = "curl -s -L -A \"MinerClient\" -m 8 -X POST \"" + poolUrl + "/submit\" -H \"Content-Type: application/json\" -d '{\"worker\":\"" + workerName + "\",\"block_index\":" + std::to_string(sIndex) + ",\"winning_nonce\":" + std::to_string(winningNonce) + ",\"duration_sec\":" + std::to_string(durationSec) + "}'";
            std::string submitRes = execCurl(curlCmd);

            std::string status = extractJsonValue(submitRes, "status");
            double dynamicReward = 0.000005 * std::pow(1.6, sDiff - 1);

            if (status == "accepted") {
                poolPendingBalance = safeStod(extractJsonValue(submitRes, "pool_balance"), poolPendingBalance);

                std::string nextBlockIdxStr = extractJsonValue(submitRes, "block_index");
                std::string nextDiffStr = extractJsonValue(submitRes, "difficulty");
                std::string nextEpochStr = extractJsonValue(submitRes, "epoch");
                std::string nextPrevHash = extractJsonValue(submitRes, "previous_hash");

                {
                    std::lock_guard<std::mutex> lock(workMutex);
                    if (!nextBlockIdxStr.empty()) currentWork.index = safeStoi(nextBlockIdxStr, currentWork.index + 1);
                    if (!nextDiffStr.empty()) currentWork.diff = safeStoi(nextDiffStr, currentWork.diff);
                    if (!nextEpochStr.empty()) currentWork.epoch = safeStoi(nextEpochStr, currentWork.epoch + 1);
                    if (!nextPrevHash.empty()) currentWork.prevHash = nextPrevHash;
                    blockIndex = static_cast<int>(currentWork.index);
                    difficulty = currentWork.diff;
                }
                workVersion.fetch_add(1, std::memory_order_release);

                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "\n----------------------------------------\n"
                              << "[" << getCurrentTime() << "] [Block #" << sIndex << "] SOLVED BY [" << COLOR_CYAN << workerName << COLOR_RESET << "] (DIFF " << sDiff << "/20)!\n"
                              << " > winning_nonce : " << COLOR_YELLOW << winningNonce << COLOR_RESET << "\n"
                              << " > target_hash   : " << COLOR_GREEN << winningHash << COLOR_RESET << "\n"
                              << " > time_taken    : " << std::fixed << std::setprecision(2) << durationSec << " s\n"
                              << " > pool_balance  : " << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPendingBalance << " BTC" << COLOR_RESET << "\n"
                              << " > next_block    : #" << blockIndex << " (Diff: " << difficulty << ")\n"
                              << "----------------------------------------\n\n";
                }

                saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
                foundMatch.store(false);
                blockStartTime = std::chrono::high_resolution_clock::now();
            } else {
                std::string statusJson = execCurl("curl -s -L -A \"MinerClient\" -m 3 \"" + poolUrl + "/?worker=" + workerName + "\"");
                if (!statusJson.empty() && statusJson.find("block") != std::string::npos) {
                    std::lock_guard<std::mutex> lock(workMutex);
                    currentWork.index = safeStoi(extractJsonValue(statusJson, "block"), currentWork.index);
                    currentWork.diff = safeStoi(extractJsonValue(statusJson, "difficulty"), currentWork.diff);
                    currentWork.epoch = safeStoi(extractJsonValue(statusJson, "epoch"), currentWork.epoch);
                    std::string rPrev = extractJsonValue(statusJson, "previous_hash");
                    if (!rPrev.empty()) currentWork.prevHash = rPrev;
                    blockIndex = static_cast<int>(currentWork.index);
                    difficulty = currentWork.diff;
                }
                workVersion.fetch_add(1, std::memory_order_release);
                foundMatch.store(false);
                blockStartTime = std::chrono::high_resolution_clock::now();
            }
        }

        // Poll định kỳ mỗi 1.5s bắt tín hiệu Fake Sync Epoch từ Master Watchdog
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSyncCheck).count() >= 1500) {
            lastSyncCheck = now;
            std::string statusJson = execCurl("curl -s -L -A \"MinerClient\" -m 2 \"" + poolUrl + "/?worker=" + workerName + "\"");
            if (!statusJson.empty() && statusJson.find("block") != std::string::npos) {
                long long remoteBlock = safeStoi(extractJsonValue(statusJson, "block"), 0);
                int remoteEpoch = safeStoi(extractJsonValue(statusJson, "epoch"), 0);
                
                long long localBlockIdx;
                int localEpoch;
                {
                    std::lock_guard<std::mutex> lock(workMutex);
                    localBlockIdx = currentWork.index;
                    localEpoch = currentWork.epoch;
                }

                if (remoteBlock > localBlockIdx || remoteEpoch > localEpoch) {
                    std::string remotePrevHash = extractJsonValue(statusJson, "previous_hash");
                    int remoteDiff = safeStoi(extractJsonValue(statusJson, "difficulty"), 3);

                    {
                        std::lock_guard<std::mutex> lock(workMutex);
                        currentWork.index = remoteBlock;
                        currentWork.diff = remoteDiff;
                        currentWork.epoch = remoteEpoch;
                        if (!remotePrevHash.empty()) currentWork.prevHash = remotePrevHash;
                        blockIndex = static_cast<int>(currentWork.index);
                        difficulty = currentWork.diff;
                    }
                    workVersion.fetch_add(1, std::memory_order_release);
                    blockStartTime = std::chrono::high_resolution_clock::now();

                    std::lock_guard<std::mutex> lock(coutMutex);
                    if (remoteBlock > localBlockIdx) {
                        std::cout << COLOR_CYAN << "\n[" << getCurrentTime() << "] [SYNC] New block detected! Switched to Block #" << remoteBlock << " (Diff: " << remoteDiff << ") instantly!\n" << COLOR_RESET;
                    } else {
                        std::cout << COLOR_MAGENTA << "\n[" << getCurrentTime() << "] [SYNC] Master Watchdog triggered timeout! Difficulty dropped to " << remoteDiff << "/20. Restarting block from nonce 0...\n" << COLOR_RESET;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    stopMining = true;
    for (auto& th : workers) {
        if (th.joinable()) th.join();
    }
    saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
}

void runStandardOptions(double& walletBalance, int& blockIndex, int& difficulty, bool& autoMineNext, bool& autoDifficulty, long long& targetHashrate, long long& outputRate, bool& isBanned, bool isSpeedModActive, bool isHackerModActive, bool& inspectionWait, const std::string& workerName, const std::string& poolUrl) {
    int optChoice = 0;
    while (true) {
        clearScreen();
        std::cout << "> miner_options\n"
                  << "----------------------------------------\n";
        
        int idx = 1;
        int optMap[10];

        if (isHackerModActive) {
            std::cout << " [" << idx << "] set_difficulty   : " << difficulty << " [MOD]\n";
            optMap[idx++] = 1;
            std::cout << " [" << idx << "] auto_difficulty  : " << (autoDifficulty ? "ON" : "OFF") << " [MOD]\n";
            optMap[idx++] = 2;
        }

        std::cout << " [" << idx << "] auto_mine_next   : " << (autoMineNext ? "ON" : "OFF") << "\n";
        optMap[idx++] = 3;

        std::cout << " [" << idx << "] inspection_wait  : " << (inspectionWait ? "ON" : "OFF") << "\n";
        optMap[idx++] = 4;

        std::cout << " [" << idx << "] output_rate      : " << outputRate << " nonces/log\n";
        optMap[idx++] = 5;

        if (isSpeedModActive) {
            std::cout << " [" << idx << "] set_hashrate     : UNLIMITED (INF) [MOD]\n";
            optMap[idx++] = 6;
        }

        std::cout << " [" << idx << "] back_to_main\n";
        int backChoiceId = idx;
        std::cout << "----------------------------------------\n"
                  << "root@miner/options# ";

        if (!(std::cin >> optChoice)) {
            std::cin.clear(); std::cin.ignore(10000, '\n');
            continue;
        }

        if (optChoice == backChoiceId) break;

        if (optChoice >= 1 && optChoice < backChoiceId) {
            int selectedAction = optMap[optChoice];
            if (selectedAction == 1) {
                std::cout << "Enter difficulty (1-20): "; std::cin >> difficulty;
                if (difficulty < 1) difficulty = 1; if (difficulty > 20) difficulty = 20;
            } else if (selectedAction == 2) {
                autoDifficulty = !autoDifficulty;
            } else if (selectedAction == 3) {
                autoMineNext = !autoMineNext;
            } else if (selectedAction == 4) {
                inspectionWait = !inspectionWait;
            } else if (selectedAction == 5) {
                std::cout << "Enter output rate (log interval): "; std::cin >> outputRate;
                if (outputRate < 1000) outputRate = 1000;
            } else if (selectedAction == 6) {
                std::cout << "[MOD] Hashrate is locked to UNLIMITED (INF)!\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
    }
}

void runPoolOptions(double& walletBalance, double& poolPendingBalance, int blockIndex, int difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, std::string& workerName, std::string& poolUrl) {
    int pChoice = 0;
    while (true) {
        clearScreen();
        std::cout << "> pool_options\n"
                  << "----------------------------------------\n"
                  << " [1] set_worker_name : " << workerName << "\n"
                  << " [2] set_pool_url    : " << (poolUrl.empty() ? "None" : poolUrl) << "\n"
                  << " [3] transfer_money  : " << COLOR_YELLOW << "Withdraw/Send Pool Funds" << COLOR_RESET << "\n"
                  << " [4] back_to_main\n"
                  << "----------------------------------------\n"
                  << "root@miner/pool_options# ";

        if (!(std::cin >> pChoice)) {
            std::cin.clear(); std::cin.ignore(10000, '\n');
            continue;
        }

        if (pChoice == 4) break;

        if (pChoice == 1) {
            std::cout << "Enter Worker Name (No spaces): "; std::cin >> workerName;
        } else if (pChoice == 2) {
            std::cout << "Enter Pool URL: "; std::cin >> poolUrl;
            if (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
        } else if (pChoice == 3) {
            if (poolUrl.empty()) {
                std::cout << COLOR_RED << "[ERROR] Configure Pool URL first!\n" << COLOR_RESET;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                std::cout << "\n[1] Withdraw ALL to Local Wallet\n"
                          << "[2] Transfer ALL to another Worker on Pool\n"
                          << "Select mode: ";
                int tMode = 1;
                std::cin >> tMode;
                std::string toWorker = "";
                if (tMode == 2) {
                    std::cout << "Enter recipient Worker Name: ";
                    std::cin >> toWorker;
                }

                std::string curlCmd = "curl -s -L -A \"MinerClient\" -m 8 -X POST \"" + poolUrl + "/transfer\" -H \"Content-Type: application/json\" -d '{\"worker\":\"" + workerName + "\",\"option\":" + std::to_string(tMode) + ",\"recipient\":\"" + toWorker + "\"}'";
                std::string res = execCurl(curlCmd);
                
                std::string status = extractJsonValue(res, "status");
                std::string action = extractJsonValue(res, "action");
                
                std::string poolBalStr = extractJsonValue(res, "pool_balance");
                if (!poolBalStr.empty()) {
                    poolPendingBalance = safeStod(poolBalStr, poolPendingBalance);
                }

                if (status == "success") {
                    double transferred = safeStod(extractJsonValue(res, "transferred"), 0.0);
                    if (action == "withdrawn") {
                        walletBalance += transferred;
                        std::cout << COLOR_GREEN << "[SUCCESS] Withdrawn +" << std::fixed << std::setprecision(8) << transferred << " BTC to local wallet!\n" << COLOR_RESET;
                    } else if (action == "sent_to_peer") {
                        std::cout << COLOR_GREEN << "[SUCCESS] Sent " << std::fixed << std::setprecision(8) << transferred << " BTC to " << toWorker << "!\n" << COLOR_RESET;
                    }
                } else {
                    std::cout << COLOR_YELLOW << "[INFO] No available balance in the pool to transfer.\n" << COLOR_RESET;
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
        saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
    }
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    try {
        std::filesystem::create_directories(MODS_DIR);
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
    std::string workerName = "TVBox_Node";
    std::string poolUrl = "";

    bool isSpeedModActive = verifyMod(SPEED_MOD_PATH, SPEED_TOKEN);
    bool isHackerModActive = verifyMod(HACKER_MOD_PATH, HACKER_TOKEN);
    bool isPoolModActive = verifyMod(POOL_MOD_PATH, POOL_TOKEN);

    loadGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);

    if (!poolUrl.empty()) {
        std::string initSyncUri = poolUrl + "/join?worker=" + workerName;
        std::string initSyncJson = execCurl("curl -s -L -A \"MinerClient\" -m 3 \"" + initSyncUri + "\"");
        if (!initSyncJson.empty() && initSyncJson.find("pool_balance") != std::string::npos) {
            poolPendingBalance = safeStod(extractJsonValue(initSyncJson, "pool_balance"), 0.0);
        }
    }

    if (isBanned) {
        clearScreen();
        std::cout << COLOR_RED << "access denied\n"
                  << "Your config file has been found as compromised/modified illegally, programme locked permanently and your data has been wiped. To continue, delete the existing configs file.\n"
                  << COLOR_RESET;
        std::cout << "\nPress ENTER to exit program...";
        std::cin.get();
        return 0;
    }

    int choice = 0;
    while (true) {
        clearScreen();
        int hardwareCores = std::thread::hardware_concurrency();
        if (hardwareCores < 1) hardwareCores = 4;

        if (!isPoolModActive) {
            std::cout << "> miner_main_menu (Linux / Termux Edition)\n"
                      << "----------------------------------------\n"
                      << " [1] start_mining\n"
                      << " [2] options\n"
                      << " [3] quit\n"
                      << "----------------------------------------\n"
                      << "data_dir      : " << BASE_DIR << "\n"
                      << "wallet_balance: " << std::fixed << std::setprecision(8) << walletBalance << " BTC\n"
                      << "current_hash  : " << (isSpeedModActive ? "UNLIMITED [RAW SPEED MOD]" : std::to_string(targetHashrate) + " H/s") << "\n"
                      << "cpu_threads   : " << hardwareCores << " Cores (Native Optimized)\n"
                      << "current_block : #" << blockIndex << "\n"
                      << (isSpeedModActive ? COLOR_CYAN "[MOD LOADER] Active: unlimited_speed.mod detected!\n" : "")
                      << (isHackerModActive ? COLOR_MAGENTA "[MOD LOADER] Active: hacker.mod detected (Admin Unlocked)!\n" : "")
                      << COLOR_RESET
                      << "root@miner:~# ";
            
            if (!(std::cin >> choice)) {
                std::cin.clear(); std::cin.ignore(10000, '\n');
                continue;
            }

            if (choice == 3) {
                std::cout << "Terminating process...\n";
                saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
                break;
            } else if (choice == 2) {
                runStandardOptions(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
            } else if (choice == 1) {
                runLocalMining(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
            }
        } else {
            std::cout << "> miner_main_menu (Linux / Termux Edition)\n"
                      << "----------------------------------------\n"
                      << " [1] solo_mining\n"
                      << " [2] pool_mining\n"
                      << " [3] options\n"
                      << " [4] pool_options\n"
                      << " [5] quit\n"
                      << "----------------------------------------\n"
                      << "data_dir      : " << BASE_DIR << "\n"
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
            
            if (!(std::cin >> choice)) {
                std::cin.clear(); std::cin.ignore(10000, '\n');
                continue;
            }

            if (choice == 5) {
                std::cout << "Terminating process...\n";
                saveGameState(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
                break;
            } else if (choice == 4) {
                runPoolOptions(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
            } else if (choice == 3) {
                runStandardOptions(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
            } else if (choice == 2) {
                runPoolMining(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
            } else if (choice == 1) {
                runLocalMining(walletBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, poolUrl);
            }
        }
        clearScreen();
    } 
    return 0;
}
