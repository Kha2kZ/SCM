#CLI-BASED POW CRYPTO MINER

A high-performance, multi-threaded C++ Proof-of-Work (PoW) mining system featuring native ASIC-style hashing, solo and HTTP REST pool mining pipelines, and an external file-based Mod Loader.

> **Language:** [English](README.md) | [Tiếng Việt](README_VI.md)

---

## 1. TECHNICAL SPECIFICATIONS

| Component | Architecture / Implementation |
| :--- | :--- |
| **Core Language** | C++17 (Native Execution, Zero External Runtime Dependencies) |
| **Concurrency Model** | Persistent Thread Pool synchronized via `std::atomic` & `std::mutex` (Zero-Downtime Pipeline) |
| **Target Compilers** | Windows (`x86_64-w64-mingw32-g++ -O3 -static`) / Linux ARM64 (`aarch64-linux-gnu-g++ -O3 -static`) |
| **Networking Stack** | Stateless HTTP/1.1 REST client via system `curl` subprocesses (Pool Handshake, Submit, Epoch Sync) |
| **State & Security** | Local JSON (`financial_report.json`) protected by a salted bitwise Checksum anti-tamper lock |

---

## 2. MINING PIPELINE & HASHING ENGINE (`simulateHashFast`)

The core hashing loop is engineered for minimal memory allocation and maximum CPU register utilization, bypassing standard string manipulation inside hot loops.

### Block Construction & Nonce Iteration
*   **Base Prefix Initialization:** Before spawning worker threads, the engine constructs the static block header string:
    `[previous_hash]:Block#[block_index]:Nonce=`
*   **Pre-Computed State:** The engine hashes the static prefix once into two 32-bit base states (`base_state1`, `base_state2`). Worker threads clone these base states locally and iterate over contiguous batches (`50,000` nonces per atomic fetch) to eliminate lock contention.
*   **64-Bit Nonce Splitting:** Each 64-bit `nonce` is split into two 32-bit chunks (`nonceLow` and `nonceHigh`) to feed directly into 32-bit bitwise operations without heap overhead.

### `computeASICStep32` (Bitwise Diffusion)
*   **Dual-State Architecture:** Runs two parallel FNV-1a-derived state registers initialized with prime seeds (`2166136261UL` and `3382945747UL`).
*   **Entropy Mixing:** Each step applies continuous bitwise XOR (`^`), cyclic bit-rotations (`<<`, `>>`), and prime magic constants (`0x5bd1e995UL`, `0x9e3779b9UL`) to achieve rapid avalanche diffusion:
    ```cpp
    state ^= nonceChunk;
    state = (state << 13) | (state >> 19);
    state ^= 0x5bd1e995UL;
    state += (state << 5);
    state ^= (state >> 15);
    state += (state << 3);
    state ^= (state >> 7);
    ```
*   **Final Fold:** `state1` and `state2` are cross-XORed (`state1 ^ (state2 >> 3)`, `state2 ^ (state1 << 3)`) and multiplied into four 32-bit output words (`part1` through `part4`), forming a 128-bit (32-character hex) digest.

### Difficulty Matching (`checkDifficultyMatch`)
*   Evaluates leading zero bits across `part1` to `part4` using hardware-accelerated Count Leading Zeros (`__builtin_clz`).
*   Supports dynamic difficulty scaling from **Tier 1 to Tier 20** (requiring `diff * 4` leading zero bits).

---

## 3. VIRTUAL ASSETS & REWARD ECONOMICS

| Parameter | Rule / Formula |
| :--- | :--- |
| **Asset Nature** | **100% Virtual.** Balances displayed in `BTC` are strictly for entertainment, benchmarking, and distributed systems experimentation. They hold **zero** real-world monetary value and cannot be withdrawn to real crypto networks. |
| **Reward Formula** | $\text{Reward} = 0.000005 \times 1.6^{(\text{difficulty} - 1)} \text{ BTC}$ |
| **Auto-Difficulty** | Dynamically adjusts block difficulty (`+1` to `+4` or `-2`) based on solve duration thresholds (`< 0.8s`, `< 2.5s`, `< 7.0s`, `> 22.0s`) in Solo mode. |

---

## 4. MOD LOADER & TOKEN SECURITY POLICY

### External Mod Architecture
On startup, the client scans the `crypto_miner/mods/` directory for specific `.mod` activation files:
*   `unlimited_speed.mod` — Removes artificial hashrate caps for raw hardware benchmarking.
*   `hacker.mod` — Unlocks administrative overrides (manual difficulty injection, auto-difficulty toggles).
*   `pool_mining.mod` — Enables HTTP REST multi-worker pool mining and peer-to-peer balance transfers.

### Token Distribution & Security Policy
*   **Zero Hardcoded Tokens:** Authentication tokens are strictly excluded from public source files, binaries, and configuration templates.
*   **Anti-Tamper Checksum:** Modifying `financial_report.json` or spoofing mod states without a valid runtime token invalidates the internal salted checksum, permanently locking the local save profile until wiped.
*   **How to Obtain Tokens:** Valid `.mod` authentication tokens are issued exclusively through the **Official Project Discord Server**. Do not trust third-party token generators.
