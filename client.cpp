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
#include <limits>
#include <poll.h>
#include <unistd.h>

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
#define COLOR_MAGENTA "\033[35m"

const std::string SAVE_FILE_PATH = "/storage/emulated/0/crypto_miner/financial_report.json";
const std::string SPEED_MOD_PATH = "/storage/emulated/0/crypto_miner/mods/unlimited_speed.mod";
const std::string HACKER_MOD_PATH = "/storage/emulated/0/crypto_miner/mods/hacker.mod";
const std::string POOL_MOD_PATH = "/storage/emulated/0/crypto_miner/mods/pool_mining.mod";

const std::string SPEED_TOKEN = "tk_756e6e696d697465647370656564";
const std::string HACKER_TOKEN = "KHAI_HACKER_ADMIN_2026";
const std::string POOL_TOKEN = "tk_706f6f6c6d696e696e67";

const long long DEFAULT_CHUNK_STEP = 10000000000LL;

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
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

inline void waitForEnter(const std::string& prompt = "Press [ENTER] to return to menu...") {
	std::cout << prompt;
	flushOut();
	std::cin.clear();
	std::string line;
	std::getline(std::cin, line);
}

void clearScreen() {
	flushOut();
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
	flushOut();
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
	try {
		return std::stoi(cleaned);
	}
	catch (...) {
		return defaultValue;
	}
}

inline long long safeStoll(const std::string& str, long long defaultValue = 0) {
	std::string cleaned = cleanJsonString(str);
	if (cleaned.empty()) return defaultValue;
	try {
		return std::stoll(cleaned);
	}
	catch (...) {
		return defaultValue;
	}
}

inline unsigned long long safeStoull(const std::string& str, unsigned long long defaultValue = 0) {
	std::string cleaned = cleanJsonString(str);
	if (cleaned.empty()) return defaultValue;
	try {
		return std::stoull(cleaned);
	}
	catch (...) {
		return defaultValue;
	}
}

inline double safeStod(const std::string& str, double defaultValue = 0.0) {
	std::string cleaned = cleanJsonString(str);
	if (cleaned.empty()) return defaultValue;
	try {
		return std::stod(cleaned);
	}
	catch (...) {
		return defaultValue;
	}
}

// Escape chuỗi an toàn khi tạo JSON body
std::string escapeJson(const std::string& s) {
	std::string out;
	out.reserve(s.size() + 8);
	for (char c : s) {
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

// Escape tham số đơn luồng cho shell single-quote '...'
std::string escapeShellSingleQuote(const std::string& s) {
	std::string out;
	out.reserve(s.size() + 8);
	for (char c : s) {
		if (c == '\'') out += "'\\''";
		else out += c;
	}
	return out;
}

// Encode URL query param cơ bản
std::string urlEncode(const std::string& value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex << std::uppercase;
	for (unsigned char c : value) {
		if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
		} else {
			escaped << '%' << std::setw(2) << static_cast<int>(c);
		}
	}
	return escaped.str();
}

// Tách đúng phần JSON object {...} hoặc array [...] hợp lệ từ raw output (loại bỏ HTTP headers, BOM, log rác)
std::string extractValidJsonPayload(const std::string& raw) {
	if (raw.empty()) return "";

	// Tìm vị trí bắt đầu JSON ({ hoặc [)
	size_t startObj = raw.find('{');
	size_t startArr = raw.find('[');
	size_t start = std::string::npos;
	char openChar = '{';
	char closeChar = '}';

	if (startObj != std::string::npos && (startArr == std::string::npos || startObj < startArr)) {
		start = startObj;
		openChar = '{';
		closeChar = '}';
	} else if (startArr != std::string::npos) {
		start = startArr;
		openChar = '[';
		closeChar = ']';
	} else {
		return "";
	}

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
		} else {
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
	}
	return "";
}

std::string execCurlRaw(const std::string& cmd, std::string* rawOut = nullptr) {
	std::string result;
	// Thêm 2>&1 để bắt cả lỗi curl (SSL, DNS, timeout) phục vụ debug nếu cần
	std::string fullCmd = cmd + " 2>&1";
	FILE* pipe = popen(fullCmd.c_str(), "r");
	if (!pipe) return "";
	char buffer[512];
	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		result += buffer;
	}
	pclose(pipe);
	if (rawOut) *rawOut = result;
	return extractValidJsonPayload(result);
}

std::string httpGetJson(const std::string& url, int timeoutSec = 8, std::string* rawOut = nullptr) {
	// --post301 --post302 --post303 tránh đổi method sai khi server redirect HTTP -> HTTPS
	std::string cmd = "curl -sS -L --post301 --post302 --post303 "
	                  "-A \"MinerClient/1.0\" "
	                  "-H \"Accept: application/json\" "
	                  "-m " + std::to_string(timeoutSec) + " "
	                  "'" + escapeShellSingleQuote(url) + "'";
	return execCurlRaw(cmd, rawOut);
}

std::string httpPostJson(const std::string& url, const std::string& jsonPayload, int timeoutSec = 8, std::string* rawOut = nullptr) {
	std::string cmd = "curl -sS -L --post301 --post302 --post303 "
	                  "-A \"MinerClient/1.0\" "
	                  "-H \"Accept: application/json\" "
	                  "-H \"Content-Type: application/json\" "
	                  "-m " + std::to_string(timeoutSec) + " "
	                  "-X POST '" + escapeShellSingleQuote(url) + "' "
	                  "-d '" + escapeShellSingleQuote(jsonPayload) + "'";
	return execCurlRaw(cmd, rawOut);
}

// Parser lấy giá trị theo key trong chuỗi JSON hợp lệ (hỗ trợ khoảng trắng quanh dấu :, nested object, string, số, bool)
std::string extractJsonValue(const std::string& rawJson, const std::string& key) {
	std::string json = extractValidJsonPayload(rawJson);
	if (json.empty()) json = rawJson;

	std::string searchKey = "\"" + key + "\"";
	size_t pos = 0;

	while ((pos = json.find(searchKey, pos)) != std::string::npos) {
		size_t afterKey = pos + searchKey.length();
		while (afterKey < json.length() && std::isspace(static_cast<unsigned char>(json[afterKey]))) {
			afterKey++;
		}
		// Đảm bảo đây thực sự là key (theo sau bởi dấu ':') chứ không phải value trùng tên
		if (afterKey < json.length() && json[afterKey] == ':') {
			size_t valPos = afterKey + 1;
			while (valPos < json.length() && std::isspace(static_cast<unsigned char>(json[valPos]))) {
				valPos++;
			}
			if (valPos >= json.length()) return "";

			if (json[valPos] == '\"') {
				valPos++;
				std::string extracted;
				while (valPos < json.length()) {
					if (json[valPos] == '\\' && valPos + 1 < json.length()) {
						char nextChar = json[valPos + 1];
						if (nextChar == '\"' || nextChar == '\\' || nextChar == '/') extracted += nextChar;
						else if (nextChar == 'n') extracted += '\n';
						else if (nextChar == 'r') extracted += '\r';
						else if (nextChar == 't') extracted += '\t';
						else extracted += nextChar;
						valPos += 2;
					} else if (json[valPos] == '\"') {
						break;
					} else {
						extracted += json[valPos++];
					}
				}
				return extracted;
			} else if (json[valPos] == '{' || json[valPos] == '[') {
				return extractValidJsonPayload(json.substr(valPos));
			} else {
				size_t endPos = json.find_first_of(",}]\r\n", valPos);
				if (endPos == std::string::npos) endPos = json.length();
				std::string val = json.substr(valPos, endPos - valPos);
				size_t first = val.find_first_not_of(" \t\r\n");
				if (first == std::string::npos) return "";
				size_t last = val.find_last_not_of(" \t\r\n");
				return val.substr(first, (last - first + 1));
			}
		}
		pos = afterKey;
	}
	return "";
}

// Kiểm tra response JSON có báo thành công không (linh hoạt theo nhiều chuẩn API)
bool isJsonSuccess(const std::string& json) {
	if (json.empty()) return false;
	std::string st = cleanJsonString(extractJsonValue(json, "status"));
	std::transform(st.begin(), st.end(), st.begin(), ::tolower);
	if (st == "ok" || st == "success" || st == "created" || st == "authenticated" || st == "true") {
		return true;
	}
	std::string okFlag = cleanJsonString(extractJsonValue(json, "ok"));
	std::string successFlag = cleanJsonString(extractJsonValue(json, "success"));
	std::transform(okFlag.begin(), okFlag.end(), okFlag.begin(), ::tolower);
	std::transform(successFlag.begin(), successFlag.end(), successFlag.begin(), ::tolower);
	return (okFlag == "true" || okFlag == "1" || successFlag == "true" || successFlag == "1");
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
	if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
		char c;
		bool gotAny = false;
		while (read(STDIN_FILENO, &c, 1) > 0) {
			gotAny = true;
			if (c == '\n' || c == '\r') break;
		}
		std::cin.clear();
		return gotAny;
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

void saveGameState(double balance, double poolPendingBalance, int blockIdx, int diff, bool autoMine, bool autoDiff, long long targetHashrate, long long outputRate, bool isBanned, bool speedActive, bool hackerActive, bool inspectionWait, const std::string& workerName, const std::string& workerToken, bool authed, std::string poolUrl) {
	while (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
	unsigned long long checksum = calculateChecksum(balance, poolPendingBalance, blockIdx, diff, isBanned, targetHashrate, speedActive, hackerActive, inspectionWait, authed);
	std::ostringstream buffer;
	buffer << "{\n"
	       << "  \"document_title\": \"Q3 Financial Summary & Asset Audit Report\",\n"
	       << "  \"classification\": \"CONFIDENTIAL_INTERNAL\",\n"
	       << "  \"worker_name\": \"" << escapeJson(workerName) << "\",\n"
	       << "  \"worker_token\": \"" << escapeJson(workerToken) << "\",\n"
	       << "  \"authed\": " << (authed ? "true" : "false") << ",\n"
	       << "  \"pool_url\": \"" << escapeJson(poolUrl) << "\",\n"
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

	std::ofstream outFile(SAVE_FILE_PATH, std::ios::out | std::ios::trunc);
	if (outFile.is_open()) {
		outFile << buffer.str();
		outFile.flush();
		outFile.close();
	}
}

void loadGameState(double& balance, double& poolPendingBalance, int& blockIdx, int& diff, bool& autoMine, bool& autoDiff, long long& targetHashrate, long long& outputRate, bool& isBanned, bool speedActive, bool hackerActive, bool& inspectionWait, std::string& workerName, std::string& workerToken, bool& authed, std::string& poolUrl) {
	std::ifstream inFile(SAVE_FILE_PATH);
	if (!inFile.is_open()) {
		saveGameState(balance, poolPendingBalance, blockIdx, diff, autoMine, autoDiff, targetHashrate, outputRate, isBanned, speedActive, hackerActive, inspectionWait, workerName, workerToken, authed, poolUrl);
		return;
	}
	std::stringstream ss;
	ss << inFile.rdbuf();
	inFile.close();

	std::string content = extractValidJsonPayload(ss.str());
	if (content.empty()) return;

	std::string v;
	if (!(v = extractJsonValue(content, "wallet_balance")).empty()) balance = safeStod(v, balance);
	if (!(v = extractJsonValue(content, "worker_name")).empty()) workerName = v;
	if (!(v = extractJsonValue(content, "worker_token")).empty()) workerToken = v;
	if (!(v = extractJsonValue(content, "authed")).empty()) authed = (cleanJsonString(v) == "true");
	if (!(v = extractJsonValue(content, "pool_url")).empty()) poolUrl = v;
	if (!(v = extractJsonValue(content, "pool_unclaimed")).empty()) poolPendingBalance = safeStod(v, poolPendingBalance);
	if (!(v = extractJsonValue(content, "block_index")).empty()) blockIdx = safeStoi(v, blockIdx);
	if (!(v = extractJsonValue(content, "difficulty")).empty()) diff = safeStoi(v, diff);
	if (!(v = extractJsonValue(content, "target_hashrate")).empty()) targetHashrate = safeStoll(v, targetHashrate);
	if (!(v = extractJsonValue(content, "output_rate")).empty()) outputRate = safeStoll(v, outputRate);
	if (!(v = extractJsonValue(content, "auto_mine_next")).empty()) autoMine = (cleanJsonString(v) == "true");
	if (!(v = extractJsonValue(content, "inspection_wait")).empty()) inspectionWait = (cleanJsonString(v) == "true");
	if (!(v = extractJsonValue(content, "auto_difficulty")).empty()) autoDiff = (cleanJsonString(v) == "true");
	if (!(v = extractJsonValue(content, "is_banned")).empty()) isBanned = (cleanJsonString(v) == "true");

	std::string chkStr = extractJsonValue(content, "security_checksum");
	if (!chkStr.empty()) {
		unsigned long long loadedChecksum = safeStoull(chkStr, 0);
		unsigned long long expectedChecksum = calculateChecksum(balance, poolPendingBalance, blockIdx, diff, isBanned, targetHashrate, speedActive, hackerActive, inspectionWait, authed);
		if (loadedChecksum != expectedChecksum) {
			isBanned = true;
			saveGameState(balance, poolPendingBalance, blockIdx, diff, autoMine, autoDiff, targetHashrate, outputRate, isBanned, speedActive, hackerActive, inspectionWait, workerName, workerToken, authed, poolUrl);
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

	std::string status = cleanJsonString(extractJsonValue(resJson, "status"));
	std::transform(status.begin(), status.end(), status.begin(), ::tolower);
	if (status == "unauthorized") {
		job.unauthorized = true;
		job.message = extractJsonValue(resJson, "message");
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

	job.blockIndex = safeStoi(extractJsonValue(resJson, "block_index"), fallbackBlock);
	job.difficulty = safeStoi(extractJsonValue(resJson, "difficulty"), fallbackDiff);
	job.previousHash = extractJsonValue(resJson, "previous_hash");

	std::string balStr = extractJsonValue(resJson, "unclaimed_pool");
	if (balStr.empty()) balStr = extractJsonValue(resJson, "pool_balance");
	job.unclaimedPool = safeStod(balStr, fallbackUnclaimed);
	return job;
}

void runLocalMining(double& walletBalance, double poolPendingBalance, int& blockIndex, int& difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, const std::string& workerName, const std::string& workerToken, bool authed, const std::string& poolUrl) {
	bool miningSession = true;

	while (miningSession) {
		clearScreen();
		int numThreads = std::thread::hardware_concurrency();
		if (numThreads < 1) numThreads = 4;

		std::cout << "[SYSTEM] Initializing... (" << numThreads << " threads)...\n"
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

		std::string previousHash = simulateHash(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
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
				if (durationSec < 0.8) difficulty = std::min(20, difficulty + 4);
				else if (durationSec < 2.5) difficulty = std::min(20, difficulty + 2);
				else if (durationSec < 7.0) difficulty++;
				else if (durationSec > 22.0 && difficulty > 3) difficulty = std::max(3, difficulty - 2);
			}

			saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);

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
			saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			miningSession = false;
		}
	}
}

void runPoolMining(double& walletBalance, double& poolPendingBalance, int& blockIndex, int& difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, const std::string& workerName, const std::string& workerToken, bool& authed, std::string& poolUrl) {
	if (!authed || workerToken.empty()) {
		std::cout << "\n" << COLOR_RED << "Could not authorize the user, please try again.\n" << COLOR_RESET;
		flushOut();
		std::this_thread::sleep_for(std::chrono::seconds(2));
		return;
	}

	if (poolUrl.empty()) {
		std::cout << "\nEnter Pool URL (e.g., https://pool.wispbyte.com): ";
		flushOut();
		if (!(std::cin >> poolUrl)) {
			discardRestOfLine();
			return;
		}
		discardRestOfLine();
		while (!poolUrl.empty() && poolUrl.back() == '/') poolUrl.pop_back();
		saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
	}

	bool poolSession = true;
	bool nextRequestExhausted = false;

	while (poolSession) {
		clearScreen();
		int numThreads = std::thread::hardware_concurrency();
		if (numThreads < 1) numThreads = 4;

		PoolJob job = requestPoolJob(poolUrl, workerName, workerToken, blockIndex, difficulty, poolPendingBalance, nextRequestExhausted);
		nextRequestExhausted = false;

		if (job.unauthorized) {
			std::cout << COLOR_RED << job.message << "\n" << COLOR_RESET;
			flushOut();
			authed = false;
			saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
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

		std::cout << "[SYSTEM] Initializing... (ID: " << workerId << " | Chunk: [" << chunkMin << ".." << chunkMax << ") | Threads: " << numThreads << ")...\n"
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
					long long endNonce = std::min(startNonce + batchSize, chunkMax);

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

			std::string payload = "{\"worker\":\"" + escapeJson(workerName) + "\",\"token\":\"" + escapeJson(workerToken) + "\",\"block_index\":" + std::to_string(poolBlockIndex) + ",\"winning_nonce\":" + std::to_string(winningNonce) + ",\"duration_sec\":" + std::to_string(durationSec) + "}";
			std::string submitRaw;
			std::string submitRes = httpPostJson(poolUrl + "/submit", payload, 8, &submitRaw);

			std::string status = cleanJsonString(extractJsonValue(submitRes, "status"));
			std::transform(status.begin(), status.end(), status.begin(), ::tolower);
			double dynamicReward = 0.000005 * std::pow(1.6, poolDifficulty - 1);

			if (status == "accepted" || isJsonSuccess(submitRes)) {
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
				std::string errMsg = extractJsonValue(submitRes, "message");
				if (errMsg.empty()) errMsg = !submitRes.empty() ? submitRes : (!submitRaw.empty() ? submitRaw : "Connection Timeout");
				std::cout << "\n" << COLOR_RED << "[REJECTED] Server response: " << errMsg << COLOR_RESET << "\n";
			}
			flushOut();

			saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);

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
			saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			poolSession = false;
		}
	}
}

void runStandardOptions(double& walletBalance, double poolPendingBalance, int& blockIndex, int& difficulty, bool& autoMineNext, bool& autoDifficulty, long long& targetHashrate, long long& outputRate, bool& isBanned, bool isSpeedModActive, bool isHackerModActive, bool& inspectionWait, const std::string& workerName, const std::string& workerToken, bool authed, const std::string& poolUrl) {
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
		saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
	}
}

void runPoolOptions(double& walletBalance, double& poolPendingBalance, int blockIndex, int difficulty, bool autoMineNext, bool autoDifficulty, long long targetHashrate, long long outputRate, bool isBanned, bool isSpeedModActive, bool isHackerModActive, bool inspectionWait, std::string& workerName, std::string& workerToken, bool& authed, std::string& poolUrl) {
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

				std::string chkRaw;
				std::string chkRes = httpGetJson(poolUrl + "/check_name?worker=" + urlEncode(inputName), 5, &chkRaw);
				if (chkRes.empty()) {
					std::cout << COLOR_RED << "[ERROR] Cannot reach Pool Host or invalid JSON at " << poolUrl << "\n"
					          << "Raw response: " << (chkRaw.empty() ? "(empty)" : chkRaw) << "\n" << COLOR_RESET;
					flushOut();
					std::this_thread::sleep_for(std::chrono::seconds(3));
					continue;
				}

				std::string avail = cleanJsonString(extractJsonValue(chkRes, "available"));
				std::transform(avail.begin(), avail.end(), avail.begin(), ::tolower);
				if (!avail.empty() && avail != "true" && avail != "1") {
					std::string msg = extractJsonValue(chkRes, "message");
					std::cout << COLOR_RED << (msg.empty() ? "Name is already taken by another worker." : msg) << "\n" << COLOR_RESET;
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

				std::string payload = "{\"worker\":\"" + escapeJson(inputName) + "\",\"token\":\"" + escapeJson(inputToken) + "\"}";
				std::string regRaw;
				std::string regRes = httpPostJson(poolUrl + "/register_worker", payload, 8, &regRaw);

				if (isJsonSuccess(regRes)) {
					workerName = inputName;
					workerToken = inputToken;
					authed = true;
					std::string bStr = extractJsonValue(regRes, "unclaimed_pool");
					if (bStr.empty()) bStr = extractJsonValue(regRes, "pool_balance");
					poolPendingBalance = safeStod(bStr, 0.0);
					std::cout << COLOR_GREEN << "[OK] Created & authed worker [" << workerName << "].\n" << COLOR_RESET;
				} else {
					std::string errMsg = extractJsonValue(regRes, "message");
					if (errMsg.empty()) errMsg = extractJsonValue(regRes, "error");
					std::cout << COLOR_RED << "[ERROR] Register failed: "
					          << (!errMsg.empty() ? errMsg : (!regRes.empty() ? regRes : (!regRaw.empty() ? regRaw : "Empty response")))
					          << "\n" << COLOR_RESET;
				}
				flushOut();
				std::this_thread::sleep_for(std::chrono::seconds(2));
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

				std::string payload = "{\"worker\":\"" + escapeJson(inputName) + "\",\"token\":\"" + escapeJson(inputToken) + "\"}";
				std::string authRaw;
				std::string authRes = httpPostJson(poolUrl + "/auth_worker", payload, 8, &authRaw);

				if (isJsonSuccess(authRes)) {
					workerName = inputName;
					workerToken = inputToken;
					authed = true;
					std::string bStr = extractJsonValue(authRes, "unclaimed_pool");
					if (bStr.empty()) bStr = extractJsonValue(authRes, "pool_balance");
					poolPendingBalance = safeStod(bStr, poolPendingBalance);
					std::cout << COLOR_GREEN << "[OK] Authenticated [" << workerName << "] | Unclaimed: " << std::fixed << std::setprecision(8) << poolPendingBalance << " BTC\n" << COLOR_RESET;
				} else {
					std::string errMsg = extractJsonValue(authRes, "message");
					if (errMsg.empty()) errMsg = extractJsonValue(authRes, "error");
					std::cout << COLOR_RED << "[ERROR] Auth failed: "
					          << (!errMsg.empty() ? errMsg : (!authRes.empty() ? authRes : (!authRaw.empty() ? authRaw : "Empty response")))
					          << "\n" << COLOR_RESET;
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
				std::string payload = "{\"worker\":\"" + escapeJson(workerName) + "\",\"token\":\"" + escapeJson(workerToken) + "\"}";
				std::string res = httpPostJson(poolUrl + "/claim_distribution", payload, 8);
				std::string status = cleanJsonString(extractJsonValue(res, "status"));
				std::transform(status.begin(), status.end(), status.begin(), ::tolower);
				if (status == "claimed" || isJsonSuccess(res)) {
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

		saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
	}
}

int main() {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(NULL);

	try {
		std::filesystem::create_directories("/storage/emulated/0/crypto_miner/mods");
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

	bool isSpeedModActive = verifyMod(SPEED_MOD_PATH, SPEED_TOKEN);
	bool isHackerModActive = verifyMod(HACKER_MOD_PATH, HACKER_TOKEN);
	bool isPoolModActive = verifyMod(POOL_MOD_PATH, POOL_TOKEN);

	loadGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);

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
		int hardwareCores = std::thread::hardware_concurrency();
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
			          << "cpu_threads   : " << hardwareCores << " cores\n"
			          << "current_block : #" << blockIndex << "\n"
			          << (isSpeedModActive ? COLOR_CYAN "[MOD LOADER] Active: unlimited_speed.mod detected!\n" : "")
			          << (isHackerModActive ? COLOR_CYAN "[MOD LOADER] Active: hacker.mod detected (Admin Unlocked)!\n" : "")
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
				saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
				break;
			} else if (choice == 2) {
				runStandardOptions(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
			} else if (choice == 1) {
				runLocalMining(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
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
			          << "wallet_balance: " << std::fixed << std::setprecision(8) << walletBalance << " BTC\n"
			          << "pool_unclaimed: " << COLOR_YELLOW << std::fixed << std::setprecision(8) << poolPendingBalance << " BTC" << COLOR_RESET << "\n"
			          << "current_hash  : " << (isSpeedModActive ? "UNLIMITED [RAW SPEED MOD]" : std::to_string(targetHashrate) + " H/s") << "\n"
			          << "cpu_threads   : " << hardwareCores << " cores\n"
			          << "current_block : #" << blockIndex << "\n"
			          << (isSpeedModActive ? COLOR_CYAN "[MOD LOADER] Active: unlimited_speed.mod detected!\n" : "")
			          << (isHackerModActive ? COLOR_CYAN "[MOD LOADER] Active: hacker.mod detected (Admin Unlocked)!\n" : "")
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
				saveGameState(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
				break;
			} else if (choice == 4) {
				runPoolOptions(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
			} else if (choice == 3) {
				runStandardOptions(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
			} else if (choice == 2) {
				runPoolMining(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
			} else if (choice == 1) {
				runLocalMining(walletBalance, poolPendingBalance, blockIndex, difficulty, autoMineNext, autoDifficulty, targetHashrate, outputRate, isBanned, isSpeedModActive, isHackerModActive, inspectionWait, workerName, workerToken, authed, poolUrl);
			}
		}
		clearScreen();
	}
	return 0;
}
