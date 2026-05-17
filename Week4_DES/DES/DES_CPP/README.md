# DES Rainbow Table Attack — C++ Version

> **Mục tiêu học thuật:** Minh họa tấn công *chosen-plaintext* dùng Rainbow Table lên thuật toán DES,
> giải thích vì sao DES bị khai tử năm 2001 và thay bằng AES.

---

## Cấu trúc thư mục

```
DES_CPP/
├── config.h            ← Cấu hình: KEY_BITS, CHAIN_LENGTH, NUM_CHAINS, PLAINTEXT
├── des.h               ← Khai báo: KeySchedule, indexToKey, desEncrypt, H()
├── des.cpp             ← Cài đặt DES (IP, S-box, key schedule, Feistel 16 round)
├── rainbow_table.h     ← Khai báo: R(), build/load table, crack()
├── rainbow_table.cpp   ← Xây bảng, lưu/load bảng, tra bảng tìm key
├── gpu_builder.h       ← Interface backend GPU
├── gpu_builder_stub.cpp← CPU fallback khi không build CUDA
├── cuda_rainbow.cu     ← CUDA backend sinh chain trên GPU
├── build_cpu.ps1       ← Build CPU trên Windows
├── build_cuda.ps1      ← Build CUDA trên Windows khi có nvcc
├── main.cpp            ← Menu, demo tự động, nhập key thủ công, benchmark
└── README.md           ← Tài liệu này
```

---

## Hướng dẫn cài đặt & biên dịch

### Yêu cầu
- **Compiler:** GCC 9+ hoặc Clang 10+ (hỗ trợ C++17)
- **Windows:** Tải MinGW-w64 tại [winlibs.com](https://winlibs.com/) → giải nén → thêm `bin/` vào PATH
- **Linux/macOS:** `sudo apt install g++` hoặc `brew install gcc`
- **CUDA GPU backend:** NVIDIA driver + CUDA Toolkit có `nvcc`

### Biên dịch

```bash
# Windows (PowerShell hoặc CMD)
.\build_cpu.ps1

# Windows CUDA
.\build_cuda.ps1

# Linux CPU
g++ -O3 -std=c++17 -o rainbow_attack main.cpp des.cpp rainbow_table.cpp gpu_builder_stub.cpp -pthread

# Linux CUDA
nvcc -O3 -std=c++17 -DUSE_CUDA -o rainbow_attack_cuda main.cpp des.cpp rainbow_table.cpp cuda_rainbow.cu
```

> **Flag `-O3`** bật tối ưu hóa tối đa → nhanh gấp ~3–5x so với không có flag.

### Chạy

```bash
# Windows
.\rainbow_attack.exe
.\rainbow_attack_cuda.exe

# Linux / macOS
./rainbow_attack
./rainbow_attack_cuda
```

---

## Cài CUDA Toolkit / nvcc trên Ubuntu

Driver NVIDIA chỉ giúp máy nhận GPU. Để build file `.cu`, cần CUDA Toolkit có `nvcc`.

Kiểm tra:

```bash
nvidia-smi
nvcc --version
```

Nếu `nvidia-smi` nhận GPU nhưng chưa có `nvcc`, cài CUDA Toolkit từ repo NVIDIA:

```bash
sudo apt update
sudo apt install -y build-essential wget lsb-release
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu$(lsb_release -rs | tr -d .)/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt update
sudo apt install -y cuda-toolkit-12-6
```

Thêm PATH:

```bash
echo 'export PATH=/usr/local/cuda-12.6/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-12.6/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
nvcc --version
```

Build CUDA trên Ubuntu:

```bash
nvcc -O3 -std=c++17 -DUSE_CUDA -o rainbow_attack_cuda main.cpp des.cpp rainbow_table.cpp cuda_rainbow.cu
./rainbow_attack_cuda
```

Nếu máy cài CUDA version khác, đổi `cuda-12.6` thành đúng thư mục trong `/usr/local/`.

---

## Cách dùng

Chương trình có 4 chế độ:

```
Menu:
  1. Crack demo tu dong       (load bang co san neu co)
  2. Crack key thu cong       (0 -> KEY_SPACE-1)
  3. Precompute/load table    (giong buoc tao bang cua crack.sh)
  4. Xem thong tin thuc te ngoai doi
```

| Chế độ | Mô tả |
|--------|-------|
| **1** | Load bảng đã precompute nếu có, rồi crack key demo |
| **2** | Load bảng đã precompute nếu có, rồi crack key bạn nhập |
| **3** | Precompute bảng và lưu ra file `.bin`, lần sau load lại |
| **4** | Hiển thị thông tin thực tế về DES crack ngoài đời (crack.sh) |

> **Lưu ý:** Workflow giống crack.sh là chạy chế độ **3** trước để tạo bảng, sau đó các lần sau chọn **1** hoặc **2** để chỉ load bảng và tra.

---

## Điều chỉnh thông số

Mở `config.h` và chỉnh các giá trị:

```cpp
static constexpr int      KEY_BITS     = 28;    // Mini DES key space = 2^28
static constexpr int      CHAIN_LENGTH = 15000; // Dài chain càng lớn → coverage tốt hơn
static constexpr int      NUM_CHAINS   = 50000; // Nhiều chain → bảng lớn hơn
static constexpr uint64_t PLAINTEXT    = 0x1122334455667788ULL; // Chosen plaintext cố định
```

### Công thức coverage (lý thuyết):
```
Coverage ≈ 1 - (1 - CHAIN_LENGTH / KEY_SPACE)^NUM_CHAINS
```

Ví dụ với giá trị mặc định hiện tại:
```
Coverage ≈ 1 - (1 - 15000/268435456)^50000 ≈ 93.88%
```

Bảng được lưu theo tên chứa cấu hình, ví dụ:

```
rainbow_table_k28_m50000_t15000.bin
```

Nếu đổi `KEY_BITS`, `CHAIN_LENGTH`, `NUM_CHAINS` hoặc `PLAINTEXT`, chương trình sẽ tự tạo file bảng mới.

---

## Giải thích code

### `config.h` — Cấu hình trung tâm

File duy nhất cần sửa khi muốn thay đổi thông số. Mọi file khác đều `#include "config.h"`.

---

### `des.h` / `des.cpp` — Thuật toán DES

#### Cấu trúc `KeySchedule`
```cpp
struct KeySchedule { uint64_t K[16]; };
```
Lưu 16 subkey (mỗi key 48 bit) được sinh ra từ key gốc 56 bit.

#### `indexToKey(idx, out)` — Chuyển index → key 8 byte
```
index (56-bit mini/real space) → [B0][B1][B2][B3][B4][B5][B6][DD]
```
7 byte đầu lấy từ index, byte cuối cố định để tạo DES key 64-bit. Code dùng `uint64_t` để sau này có thể nâng dần tới key space DES 56-bit thật.

#### `buildKeySchedule(key)` — Sinh key schedule
Quy trình chuẩn DES:
1. **PC-1**: Loại bỏ parity bit, 64 bit → 56 bit, chia thành C (28 bit) và D (28 bit)
2. **16 vòng lặp**: Dịch vòng trái C và D theo `SHIFTS[]`, rồi áp dụng **PC-2** (56 → 48 bit) → subkey K[i]

#### `desEncrypt(plaintext, ks)` — Mã hóa DES một block
Quy trình Feistel 16 vòng:

```
plaintext (64-bit)
    │
    ▼ IP (Initial Permutation)
   [L0 | R0]  (32 bit mỗi nửa)
    │
    ▼ × 16 vòng:
   L(i+1) = R(i)
   R(i+1) = L(i) XOR f(R(i), K(i))
    │
    ▼ Swap L ↔ R
    ▼ IP⁻¹ (Final Permutation)
    │
ciphertext (64-bit)
```

**Hàm f(R, K):**
1. **E (Expansion):** R 32 bit → 48 bit (một số bit được lặp)
2. **XOR** với subkey K (48 bit)
3. **S-box:** 8 S-box, mỗi cái nhận 6 bit → cho ra 4 bit (tổng 48 → 32 bit)
4. **P (Permutation):** Hoán vị 32 bit kết quả

#### `H(keyIdx)` — Hash function của rainbow table
```cpp
uint64_t H(uint64_t keyIdx) {
    indexToKey(keyIdx, key);
    ks = buildKeySchedule(key);
    return desEncrypt(PLAINTEXT, ks);  // PLAINTEXT cố định!
}
```
Đây là **chosen-plaintext**: plaintext luôn cố định nên attacker có thể precompute.

---

### `rainbow_table.h` / `rainbow_table.cpp` — Rainbow Table

#### `R(cipher, position)` — Reduction function
```cpp
uint64_t R(uint64_t cipher, int position) {
    return (cipher XOR (position * MAGIC_CONSTANT)) % KEY_SPACE;
}
```
- Chuyển ciphertext 64-bit → key index trong `KEY_SPACE`
- **Mỗi `position` cho một hàm R khác nhau** → đây là tính chất "**Rainbow**" (khác với lookup table thường dùng một hàm R duy nhất)
- Tính chất này giúp tránh collision giữa các chain khác nhau

#### `generateChainEnd(start)` — Sinh một chain
```
k_start →[H]→ c₀ →[R₀]→ k₁ →[H]→ c₁ →[R₁]→ ... →[R_{t-1}]→ k_end
```
- Chỉ lưu `(start, end)` → tiết kiệm bộ nhớ rất nhiều so với lưu toàn bộ chain
- `CHAIN_LENGTH` bước tính toán cho mỗi chain

#### `buildTable()` — Xây bảng (Multi-threaded)

```
[Thread 1]  chain 0    → chain 1999
[Thread 2]  chain 2000 → chain 3999
[Thread 3]  chain 4000 → chain 5999
[Thread 4]  chain 6000 → chain 7999
    ↓ (song song)
HashMap: { end_idx:uint64 → start_idx:uint64 }
```

1. Sinh `NUM_CHAINS` điểm bắt đầu ngẫu nhiên
2. Chia đều cho `N` thread (tự detect số CPU core)
3. Mỗi thread chạy `generateChainEnd()` độc lập
4. Gộp kết quả vào `unordered_map<uint64_t, uint64_t>`

Nếu build bằng CUDA, phần sinh chain chạy trong `cuda_rainbow.cu`; nếu không có CUDA, chương trình tự dùng CPU fallback.

#### `crack(targetCipher, table, steps)` — Tra bảng tìm key

Thuật toán từ cuối chain về đầu:

```
Với p = (t-1) xuống 0:
    cur = R_p(targetCipher)          // giả sử cipher xuất hiện ở bước p
    cur = R_{p+1}(H(cur))
    ...
    cur = R_{t-1}(H(cur))            // đến cuối chain
    
    Nếu cur có trong bảng:
        Tái tạo chain từ start để xác nhận và tìm key chính xác
```

**Tại sao tìm từ cuối về đầu?** Vì attacker không biết targetCipher nằm ở bước nào trong chain, phải thử tất cả vị trí.

---

### `main.cpp` — Giao diện người dùng

| Hàm | Vai trò |
|-----|---------|
| `benchmarkSpeed()` | Đo tốc độ DES: chạy 50,000 encrypt, tính ops/giây |
| `doAttack(idx, table)` | Thực hiện và in kết quả tấn công một key cụ thể |
| `demoAuto(table)` | Vòng lặp crack key ngẫu nhiên |
| `demoManual(table)` | Cho nhập key, hỗ trợ hex (0xABCD) và decimal |
| `showRealWorld()` | In thông tin thực tế về DES crack ngoài đời |

---

## So sánh backend

| Backend | Build command | Vai trò |
|---------|---------------|--------|
| CPU fallback | `g++ ... gpu_builder_stub.cpp -pthread` | Dễ build, dùng `std::thread`, phù hợp test logic |
| CUDA GPU | `nvcc ... cuda_rainbow.cu` | Sinh chain trên NVIDIA GPU, giống hướng precompute thực tế hơn |

Tốc độ phụ thuộc CPU/GPU, driver, CUDA Toolkit và `KEY_BITS/CHAIN_LENGTH/NUM_CHAINS`. Với workflow đúng, bảng chỉ cần build một lần rồi lưu lại; những lần sau chương trình load file `.bin` để crack.

---

## Thực tế ngoài đời

| Thông số | Giá trị |
|----------|---------|
| Plaintext cố định | `0x1122334455667788` (dùng trong Net-NTLMv1) |
| Key space thật | 2⁵⁶ ≈ 72 nghìn tỷ key |
| Công cụ thật | [crack.sh](https://crack.sh) (miễn phí), ophcrack |
| Phần cứng | GPU cluster / FPGA |
| Thời gian xây bảng | Nhiều tuần |
| Thời gian crack | **~25 giây** (chỉ tra bảng!) |
| Kích thước bảng | ~7 GB (đã nén) |

> **Kết luận:** DES bị khai tử năm 2001 chính vì chosen-plaintext rainbow table attack.
> Plaintext cố định trong Net-NTLMv1 biến DES từ 2⁵⁶ brute-force → có thể precompute hoàn toàn.
> **AES** (Advanced Encryption Standard) được dùng thay thế từ đó đến nay.

---

## Lý thuyết Rainbow Table

### Tại sao không dùng lookup table thông thường?

| Phương pháp | Không gian lưu trữ | Thời gian tìm |
|-------------|-------------------|---------------|
| Lookup table đầy đủ | O(N) — 4 tỷ entry → hàng GB | O(1) |
| Brute force | O(1) | O(N) |
| **Rainbow Table** | **O(N/t)** | **O(t²)** |

Rainbow table là điểm cân bằng tối ưu giữa bộ nhớ và thời gian tính toán (*time-memory trade-off*).

### Tại sao dùng nhiều hàm R khác nhau (R₀, R₁, ..., R_{t-1})?

Nếu chỉ dùng một hàm R:
- Hai chain cùng đi qua một điểm → **merge** → lãng phí bộ nhớ và giảm coverage

Dùng nhiều hàm R khác nhau theo vị trí:
- Chain merge ở vị trí `i` vẫn **phân kỳ** ở vị trí `i+1` → coverage tốt hơn nhiều
- Đây là đóng góp chính của Philippe Oechslin (2003)

---

*Được tạo cho mục đích học thuật — MMUD Week 4: DES Security Analysis*
