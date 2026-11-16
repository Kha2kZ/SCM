# CRYPTO_SIM_ENGINE // TRÌNH MÔ PHỎNG ĐÀO PROOF-OF-WORK (CLI)

Công cụ mô phỏng thuật toán đào Proof-of-Work (PoW) viết bằng C++ đa luồng hiệu năng cao, tích hợp nhân băm chuẩn ASIC tùy biến, hỗ trợ đào Solo, đào Pool qua HTTP REST và hệ thống Mod Loader ngoại vi.

> **Ngôn ngữ:** [English](README.md) | [Tiếng Việt](README_VI.md)

---

## 1. THÔNG SỐ KỸ THUẬT HỆ THỐNG

| Thành phần | Kiến trúc / Triển khai |
| :--- | :--- |
| **Ngôn ngữ lõi** | C++17 (Hiệu năng gốc Native, không phụ thuộc thư viện runtime ngoài) |
| **Xử lý đa luồng** | Thread Pool duy trì liên tục qua `std::atomic` & `std::mutex` (Zero-Downtime Pipeline) |
| **Mục tiêu biên dịch** | Windows (`x86_64-w64-mingw32-g++ -O3 -static`) / Linux ARM64 (`aarch64-linux-gnu-g++ -O3 -static`) |
| **Giao thức mạng** | HTTP/1.1 REST phi trạng thái qua tiến trình `curl` (Handshake, Submit, Đồng bộ Epoch) |
| **Lưu trữ & Bảo mật** | JSON cục bộ (`financial_report.json`) khóa bằng mã băm Checksum chống chỉnh sửa trái phép |

---

## 2. CƠ CHẾ ĐÀO & THUẬT TOÁN BĂM (`simulateHashFast`)

Vòng lặp băm cốt lõi được tối ưu hóa để triệt tiêu cấp phát bộ nhớ động và tận dụng tối đa thanh ghi CPU, loại bỏ hoàn toàn các thao tác xử lý chuỗi nặng nề bên trong luồng tính toán.

### Khởi tạo Block & Vòng lặp Nonce
*   **Cấu trúc tiền tố (Base Prefix):** Trước khi kích hoạt các luồng đào, hệ thống khởi tạo chuỗi định danh khối tĩnh:
    `[previous_hash]:Block#[block_index]:Nonce=`
*   **Tính toán trước trạng thái (Pre-Computed State):** Hệ thống băm chuỗi tiền tố tĩnh một lần duy nhất thành hai biến trạng thái 32-bit gốc (`base_state1`, `base_state2`). Các luồng worker nhân bản trạng thái này về bộ nhớ cục bộ và nhận từng lô (`50,000` nonce mỗi lần cấp phát atomic) để loại bỏ nghẽn cổ chai khóa luồng.
*   **Tách Nonce 64-bit:** Mỗi giá trị `nonce` 64-bit được tách thành hai phần 32-bit (`nonceLow` và `nonceHigh`) để đưa thẳng vào các phép toán bit 32-bit trên thanh ghi.

### Hàm băm `computeASICStep32` (Khuếch tán Bitwise)
*   **Kiến trúc Dual-State:** Vận hành song song hai thanh ghi trạng thái phát triển từ thuật toán FNV-1a với hạt giống số nguyên tố (`2166136261UL` và `3382945747UL`).
*   **Trộn Entropy:** Mỗi bước thực hiện liên tiếp các phép toán XOR (`^`), xoay bit vòng (`<<`, `>>`), và cộng dồn hằng số ma thuật (`0x5bd1e995UL`, `0x9e3779b9UL`) để tạo hiệu ứng thác đổ (avalanche effect):
    ```cpp
    state ^= nonceChunk;
    state = (state << 13) | (state >> 19);
    state ^= 0x5bd1e995UL;
    state += (state << 5);
    state ^= (state >> 15);
    state += (state << 3);
    state ^= (state >> 7);
    ```
*   **Kết xuất mã băm:** `state1` và `state2` được trộn chéo XOR (`state1 ^ (state2 >> 3)`, `state2 ^ (state1 << 3)`) và nhân tổ hợp thành bốn từ 32-bit (`part1` đến `part4`), tạo ra chuỗi băm 128-bit (32 ký tự hex).

### Kiểm tra độ khó (`checkDifficultyMatch`)
*   Đếm số lượng bit `0` dẫn đầu trên các phân đoạn từ `part1` đến `part4` bằng chỉ thị phần cứng Count Leading Zeros (`__builtin_clz`).
*   Hỗ trợ thang độ khó từ **Cấp 1 đến Cấp 20** (yêu cầu `diff * 4` bit `0` đứng đầu).

---

## 3. TÀI SẢN ẢO & CƠ CHẾ PHẦN THƯỞNG

| Tham số | Quy định / Công thức |
| :--- | :--- |
| **Bản chất tài sản** | **100% Giả lập (Virtual Asset).** Số dư hiển thị dưới đơn vị `BTC` chỉ phục vụ mục đích giải trí, đo hiệu năng phần cứng và nghiên cứu hệ thống phân tán. Hoàn toàn **không** có giá trị tiền tệ thực tế và không thể giao dịch trên mạng lưới blockchain thật. |
| **Công thức thưởng** | $\text{Reward} = 0.000005 \times 1.6^{(\text{difficulty} - 1)} \text{ BTC}$ |
| **Tự động chỉnh độ khó** | Tự động tăng/giảm độ khó (`+1` đến `+4` hoặc `-2`) dựa trên thời gian giải block (`< 0.8s`, `< 2.5s`, `< 7.0s`, `> 22.0s`) ở chế độ Solo. |

---

## 4. HỆ THỐNG MODS & CHÍNH SÁCH BẢO MẬT TOKEN

### Kiến trúc Mod Loader ngoại vi
Khi khởi chạy, chương trình tự động quét thư mục `crypto_miner/mods/` để tìm các tệp kích hoạt `.mod`:
*   `unlimited_speed.mod` — Gỡ bỏ giới hạn hashrate mặc định để bung tối đa sức mạnh CPU.
*   `hacker.mod` — Mở khóa quyền quản trị (chỉnh tay độ khó, bật/tắt tự động điều chỉnh độ khó).
*   `pool_mining.mod` — Kích hoạt chế độ đào nhóm (Pool Mining) qua HTTP REST và chuyển tiền nội bộ giữa các worker.

### Chính sách bảo mật & Cấp phát Token
*   **Không lưu cứng Token:** Tuyệt đối không nhúng (hardcode) token kích hoạt vào mã nguồn công khai, file thực thi hay tệp cấu hình mẫu.
*   **Khóa chống can thiệp (Anti-Tamper Checksum):** Mọi hành vi chỉnh sửa trực tiếp file `financial_report.json` hoặc giả mạo trạng thái mod mà không có token hợp lệ sẽ làm sai lệch mã kiểm tra Checksum, dẫn đến khóa vĩnh viễn dữ liệu lưu trữ hiện tại (yêu cầu xóa file config để làm lại từ đầu).
*   **Kênh nhận Token:** Các token `.mod` hợp lệ chỉ được cấp phát độc quyền thông qua **Server Discord chính thức của dự án**.
