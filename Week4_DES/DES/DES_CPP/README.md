# DES Rainbow Table Attack - C++ / CUDA

Dự án này minh họa tấn công DES chosen-plaintext bằng rainbow table. Bản hiện tại tối ưu cho bảng lớn gần 20 GiB, có thể build bằng CUDA nếu máy có GPU NVIDIA, và tra cứu bằng `vector<pair<endpoint,start>>` đã sort để giảm RAM so với `unordered_map`.

Đây là demo học thuật, không phải công cụ crack production.

## Dự Án Này Làm Gì?

Chương trình mô phỏng tình huống attacker biết trước một plaintext cố định và ciphertext tương ứng. Trong code, plaintext cố định là:

```text
0x1122334455667788
```

Với mỗi `key index`, chương trình tạo DES key từ index đó, mã hóa plaintext cố định, rồi thử tìm lại key từ ciphertext.

Dự án có 2 giai đoạn:

| Giai đoạn | Mục đích | Chạy khi nào |
|---|---|---|
| Precompute | Tạo rainbow table và lưu ra file `.bin` | Chọn menu `3` lần đầu |
| Crack/lookup | Load table có sẵn và tìm key | Chọn menu `1` hoặc `2` |

Ý tưởng là đổi thời gian build bảng trước để lấy tốc độ tra bảng nhanh về sau. Lần đầu tạo bảng sẽ lâu và tốn disk/RAM. Các lần sau chỉ load file `.bin`.

## Nguyên Lý Rainbow Table

Nếu brute force thuần túy, để crack một ciphertext cần thử từng key:

```text
key -> DES(plaintext, key) -> ciphertext
```

Với key space lớn, việc thử từng key mỗi lần rất tốn thời gian. Rainbow table dùng ý tưởng lưu trữ rút gọn theo chain:

```text
start_key -> H -> R0 -> H -> R1 -> ... -> end_key
```

Trong đó:

| Ký hiệu | Ý nghĩa trong code |
|---|---|
| `H(key)` | DES encrypt plaintext cố định bằng key đó |
| `R(cipher, position)` | Reduction function, biến ciphertext về một key index mới |
| `start_key` | Điểm đầu của chain |
| `end_key` | Điểm cuối của chain |

Thay vì lưu mọi key trong chain, chương trình chỉ lưu:

```text
endpoint -> start
```

Mỗi entry là 16 byte:

```text
uint64_t endpoint
uint64_t start
```

Vì vậy kích thước file xấp xỉ:

```text
NUM_CHAINS * 16 bytes
```

Với config full:

```text
1,342,177,280 * 16 ~= 20 GiB
```

## Vì Sao Crack Được?

Khi có ciphertext mục tiêu, chương trình không biết ciphertext đó nằm ở vị trí nào trong chain. Vì vậy nó thử giả lập từng vị trí:

```text
ciphertext -> R_p -> H -> R_{p+1} -> ... -> endpoint giả định
```

Nếu endpoint giả định có trong table, chương trình lấy `start_key`, chạy lại chain từ đầu và kiểm tra từng key:

```text
start_key -> H -> R0 -> H -> R1 -> ...
```

Khi `H(key)` bằng ciphertext mục tiêu, key đó là kết quả.

Vì table không phủ 100% key space và có collision, có thể có trường hợp key hợp lệ nhưng vẫn `Không tìm thấy`. Đây là hạn chế bình thường của rainbow table.

## Key Index Và DES Key

Menu nhập key làm việc với `key index`, không phải password thật. Code map `key index` thành DES key 8 byte.

DES key có 64 bit trên biểu diễn, nhưng chỉ 56 bit là key thật; mỗi byte có 7 bit key và 1 parity bit. Code hiện tại map index vào 7 bit key thật của từng byte và cố định parity bit:

```text
key index -> 8 byte DES key -> DES key schedule
```

Vì sao phải làm vậy: nếu nhầm cả parity bit vào key index, nhiều index khác nhau có thể tạo ra cùng DES key hiệu dụng. Khi đó chương trình tìm được key khác số với key thật nhưng vẫn giải đúng ciphertext.

Bản mới dùng format table `RBT4`. Format này vừa sửa mapping DES key, vừa giữ lại các endpoint trùng thay vì xóa đi. Lý do: nhiều chain khác nhau có thể kết thúc ở cùng endpoint; nếu chỉ giữ một chain thì file nhỏ hơn nhưng mất coverage.

Nếu đã tạo bảng bằng bản code cũ, cần build lại bảng mới. Bảng cũ không tương thích với format `RBT4`.

## Lệnh Chính Để Build Bảng 20 GiB

Trên Ubuntu, nếu đã có NVIDIA driver và CUDA Toolkit, chạy đúng lệnh này:

```bash
chmod +x run_20gb_cuda.sh
./run_20gb_cuda.sh
```

Script này sẽ:

```text
1. Biên dịch bản CUDA full
2. Tạo file chạy ./rainbow_attack_cuda
3. Mở chương trình
```

Khi menu hiện ra, chọn:

```text
3
```

Chương trình sẽ tạo/load bảng full:

```text
rainbow_table_k36_m1342177280_t256.bin
```

Dung lượng raw ước tính:

```text
~20 GiB
```

Nếu bảng đã có sẵn và config khớp, chương trình sẽ load lại bảng thay vì build lại.

Khi GPU chạy đúng, output trong phase precompute phải có:

```text
Backend           : CUDA GPU
```

Có thể mở terminal khác để xem GPU:

```bash
watch -n 1 nvidia-smi
```

## Compile Thủ Công

### File cần biên dịch

Bản CPU cần biên dịch các file:

```text
main.cpp
des.cpp
rainbow_table.cpp
gpu_builder_stub.cpp
```

Lệnh compile thủ công trên Ubuntu:

```bash
g++ -O3 -std=c++17 -pthread -o rainbow_attack main.cpp des.cpp rainbow_table.cpp gpu_builder_stub.cpp
./rainbow_attack
```

Bản CUDA cần biên dịch các file:

```text
main.cpp
des.cpp
rainbow_table.cpp
cuda_rainbow.cu
```

Lệnh compile thủ công trên Ubuntu:

```bash
nvcc -O3 -std=c++17 -DUSE_CUDA -o rainbow_attack_cuda main.cpp des.cpp rainbow_table.cpp cuda_rainbow.cu
./rainbow_attack_cuda
```

File chạy sau khi biên dịch:

| Bản build | File chạy |
|---|---|
| CPU | `./rainbow_attack` |
| CUDA GPU | `./rainbow_attack_cuda` |

## Đã Có Bảng Rồi Thì Chạy Như Nào?

Nếu đã build xong file table `.bin`, ví dụ:

```text
rainbow_table_k36_m1342177280_t256.bin
```

thì không cần precompute lại. Chỉ cần build đúng binary với cùng config, rồi chạy chương trình.

Nếu bảng được tạo bằng config mặc định trong `config.h`:

```text
KEY_BITS     = 36
CHAIN_LENGTH = 256
NUM_CHAINS   = 1342177280
PLAINTEXT    = 0x1122334455667788
```

thì build bản CUDA đầy đủ:

```bash
make cuda
./rainbow_attack_cuda
```

Hoặc chạy script một phát:

```bash
./run_20gb_cuda.sh
```

Khi menu hiện ra:

```text
1. Crack demo tự động
2. Crack key thủ công
3. Precompute/load table
4. Xem thông tin thực tế ngoài đời
```

chọn `1` hoặc `2`. Chương trình sẽ tự load file `.bin` có sẵn:

```text
Rainbow table file: rainbow_table_k36_m1342177280_t256.bin
[OK] Đã load bảng có sẵn: ... endpoints
```

Chỉ chọn `3` khi muốn tạo table mới hoặc kiểm tra load table. Nếu file `.bin` đã tồn tại và config khớp, chọn `3` cũng chỉ load lại, không build lại.

Lưu ý: tên file table phụ thuộc vào config lúc build binary. Nếu build bằng `make demo` hoặc `make demo-cuda`, binary sẽ tìm file:

```text
rainbow_table_k20_m65536_t64.bin
```

Còn nếu build bằng `make cuda`, binary sẽ tìm file:

```text
rainbow_table_k36_m1342177280_t256.bin
```

Vì vậy, nếu đã có bảng 36-bit rồi thì dùng:

```bash
make cuda
./rainbow_attack_cuda
```

không dùng `make demo-cuda`, vì nó sẽ tìm bảng demo 20-bit.

## Key Nhập Vào Là Gì?

Menu `Crack key thủ công` nhận 3 dạng input:

```text
12345
0xABCD
hello123
```

Với số decimal/hex, chương trình xem đó là `key index` trong vùng `0 -> KEY_SPACE-1`.

Với chuỗi text, chương trình hash chuỗi đó về một `key index` hợp lệ để demo.

DES key thật sự là 8 byte, nên output sẽ hiện thêm dòng:

```text
DES key    : 0x010101010549D107
```

Mỗi byte DES có 7 bit key thật và 1 parity bit. Code map `key index` vào 7 bit thật của từng byte và cố định parity bit, để tránh trường hợp nhiều index khác nhau nhưng DES xem như cùng một key.

Nếu từng build table bằng bản code cũ, hãy build lại table. Bản mới dùng magic `RBT4`, nên bảng cũ sẽ không được load nhầm sau khi đổi mapping key và đổi cách lưu endpoint.

Trong trường hợp rất hiếm có key khác cho cùng ciphertext, chương trình sẽ in:

```text
DES Equivalent Key!
```

## Lệnh Test Nhanh

Dùng để test nhanh, không cần GPU:

```bash
sudo apt update
sudo apt install build-essential
make demo
./rainbow_attack
```

Bản demo dùng:

```text
KEY_BITS     = 20
CHAIN_LENGTH = 64
NUM_CHAINS   = 65536
```

Vì vậy menu sẽ hiện key range `0 -> 1048575` và build table rất nhanh. Đây không phải cấu hình tấn công lớn, chỉ để test logic.

### Ubuntu CUDA demo

Dùng để test GPU với cấu hình nhỏ:

```bash
nvidia-smi
nvcc --version
make demo-cuda
./rainbow_attack_cuda
```

Sau khi chương trình mở menu, chọn:

```text
3
```

Nếu GPU chạy đúng, output sẽ có:

```text
Backend           : CUDA GPU
```

### Ubuntu CUDA bản lớn

Dùng cấu hình mặc định trong `config.h`, tạo table gần 20 GiB:

```bash
nvidia-smi
nvcc --version
make cuda
./rainbow_attack_cuda
```

Nếu muốn build bảng lớn hơn demo nhưng chưa muốn nhảy thẳng lên 20 GiB, dùng preset:

| Mục tiêu | Lệnh build | File table ước tính |
|---|---|---:|
| CUDA demo | `make demo-cuda` | ~1 MiB |
| CUDA 1 GiB | `make cuda-1g` | ~1 GiB |
| CUDA 2 GiB | `make cuda-2g` | ~2 GiB |
| CUDA 5 GiB | `make cuda-5g` | ~5 GiB |
| CUDA 10 GiB | `make cuda-10g` | ~10 GiB |
| CUDA full | `make cuda` | ~20 GiB |

Ví dụ build bảng 5 GiB:

```bash
make cuda-5g
./rainbow_attack_cuda
```

Hoặc dùng script shell:

```bash
./build_cuda.sh 5g
./rainbow_attack_cuda
```

Sau đó chọn `3` để tạo/load table. Tên file sẽ tự đổi theo `NUM_CHAINS`, ví dụ:

```text
rainbow_table_k36_m335544320_t256.bin
```

## Windows

CPU:

```powershell
.\build_cpu.ps1
.\rainbow_attack.exe
```

CUDA:

```powershell
nvcc --version
.\build_cuda.ps1
.\rainbow_attack_cuda.exe
```

Build CUDA theo preset trên Windows:

```powershell
.\build_cuda.ps1 demo
.\build_cuda.ps1 1g
.\build_cuda.ps1 2g
.\build_cuda.ps1 5g
.\build_cuda.ps1 10g
.\build_cuda.ps1 full
```

## Trạng Thái Hiện Tại

Config mặc định trong `config.h`:

```cpp
static constexpr int      KEY_BITS     = 36;
static constexpr uint64_t KEY_SPACE    = 1ULL << KEY_BITS;
static constexpr int      CHAIN_LENGTH = 256;
static constexpr int      NUM_CHAINS   = 1342177280;
static constexpr uint64_t PLAINTEXT    = 0x1122334455667788ULL;
```

Ý nghĩa:

| Thông số | Giá trị |
|---|---:|
| Key space | 2^36 keys |
| Chain length | 256 |
| Số chain | 1,342,177,280 |
| File table raw | ~20.00 GiB |
| Kiểu table trong RAM | `vector<pair<endpoint,start>>` đã sort |
| Lookup endpoint | binary search bằng `std::lower_bound` |

Lưu ý RAM: file table ~20 GiB nhưng khi build/sort/load vẫn cần RAM lớn. Nên dùng máy có tối thiểu 32 GiB RAM, tốt hơn là 64 GiB+.

## Cấu Trúc File

```text
DES_CPP/
|-- config.h             # Cấu hình KEY_BITS, CHAIN_LENGTH, NUM_CHAINS
|-- des.h / des.cpp      # DES thuần C++
|-- rainbow_table.h      # API rainbow table
|-- rainbow_table.cpp    # Build, save/load, sort, crack
|-- gpu_builder.h        # Interface GPU backend
|-- gpu_builder_stub.cpp # CPU fallback khi build không CUDA
|-- cuda_rainbow.cu      # CUDA kernel sinh chain
|-- main.cpp             # Menu và demo
|-- build_cpu.ps1        # Build CPU trên Windows
|-- build_cuda.ps1       # Build CUDA trên Windows
|-- Makefile             # Build trên Linux/Ubuntu
|-- build_cpu.sh         # Build CPU trên Linux/Ubuntu
|-- build_cuda.sh        # Build CUDA trên Linux/Ubuntu
|-- run_20gb_cuda.sh     # Build CUDA full và chạy chương trình
|-- README.md
```

## Cách Chạy

Menu:

```text
1. Crack demo tự động       (load bảng có sẵn nếu có)
2. Crack key thủ công       (0 -> KEY_SPACE-1)
3. Precompute/load table    (giống bước tạo bảng của crack.sh)
4. Xem thông tin thực tế ngoài đời
```

Workflow nên dùng:

1. Build một bản phù hợp:
   - Test nhanh CPU: `make demo`
   - Test nhanh CUDA: `make demo-cuda`
   - Bảng lớn CPU: `make cpu`
   - Bảng lớn CUDA: `make cuda`
2. Chạy binary tương ứng:
   - CPU: `./rainbow_attack`
   - CUDA: `./rainbow_attack_cuda`
3. Chọn `3` để precompute/load table.
4. Các lần sau chọn `1` hoặc `2`, chương trình sẽ load file `.bin` đã tạo.

Tên file table tự động dựa trên config:

```text
rainbow_table_k36_m1342177280_t256.bin
```

Nếu đổi `KEY_BITS`, `NUM_CHAINS`, `CHAIN_LENGTH` hoặc `PLAINTEXT`, chương trình sẽ tạo/load một file table khác.

## Tối Ưu Đã Áp Dụng

### 1. Không dùng `unordered_map` cho bảng lớn

Bản cũ lưu table bằng:

```cpp
unordered_map<endpoint, start>
```

Với hàng tỷ endpoint, `unordered_map` tốn RAM rất lớn do bucket/node overhead. Bản mới lưu:

```cpp
using RainbowTable = std::vector<std::pair<uint64_t, uint64_t>>;
```

Mỗi entry vẫn là `(endpoint, start)` 16 bytes. Sau khi build xong, table được sort theo endpoint nhưng vẫn giữ endpoint trùng. Điều này làm file đúng gần với kích thước raw `NUM_CHAINS * 16`, và tránh mất coverage do xóa nhầm các chain có cùng endpoint.

### 2. Lookup bằng binary search

Khi crack, code tính endpoint giả định rồi tìm range bằng:

```cpp
std::lower_bound(...)
std::upper_bound(...)
```

Sau đó chương trình thử tất cả `start` có cùng endpoint. Thời gian tìm range là `O(log n)`, cộng thêm số chain bị trùng endpoint cần thử.

### 3. Build theo batch

Code không cấp phát `starts` và `results` cho toàn bộ `NUM_CHAINS` cùng lúc. Mỗi batch hiện tại:

```cpp
BUILD_BATCH_CHAINS = 1 << 20;
```

Tức là mỗi lần sinh 1,048,576 chain, append vào table, rồi tiếp tục batch sau.

### 4. CUDA kernel chia chunk

CUDA chain được chia thành các chunk nhỏ:

```cpp
CUDA_CHAIN_STEP_CHUNK = 256;
```

Với `CHAIN_LENGTH=256`, mỗi chain hiện tại chạy đúng 1 kernel chunk. Nếu sau này tăng chain length, kernel sẽ chia nhỏ để giảm rủi ro Windows TDR timeout.

## Vì Sao CPU Vẫn Chạy Khi Build CUDA?

CUDA chỉ tăng tốc phần sinh chain trong phase precompute. Các phần sau vẫn chạy trên CPU:

| Phần việc | CPU/GPU |
|---|---|
| Benchmark DES đầu chương trình | CPU |
| Sinh chain khi có CUDA | GPU |
| Append vector, sort endpoint | CPU |
| Save/load file table | CPU / disk I/O |
| Crack/lookup/verify chain | CPU |

Việc CPU vẫn có load là bình thường. Điều cần xem là dòng `Backend` và `nvidia-smi`.

Nếu output có:

```text
Backend           : CUDA GPU
```

và `nvidia-smi` hiện process `./rainbow_attack_cuda` với `GPU-Util` cao, thì chương trình đang dùng GPU. CPU vẫn có thể tăng do phải tạo batch, copy dữ liệu, append vector, sort endpoint và ghi file.

## Vì Sao Nhập Key Trong Vùng Vẫn Có Thể Không Thấy?

Rainbow table không đảm bảo phủ 100% key space. Nó là time-memory trade-off.

Lý do miss:

- Key đó không nằm trong bất kỳ chain nào của table.
- Chain collision/merge làm coverage thực tế thấp hơn công thức lý thuyết.
- Reduction function tạo endpoint trùng.
- Table được build với config/PLAINTEXT khác binary đang chạy.

Code đã chặn trường hợp nhập key vượt vùng:

```text
0 -> KEY_SPACE-1
```

Nhưng nếu key hợp lệ mà không thuộc coverage của table thì vẫn sẽ báo `Không tìm thấy`.

## Công Thức Coverage

Ước tính lý thuyết:

```text
coverage ~= 1 - (1 - CHAIN_LENGTH / KEY_SPACE) ^ NUM_CHAINS
```

Công thức này chỉ là xấp xỉ lý tưởng. Coverage thực tế thấp hơn do collision và merge.

Với config hiện tại:

```text
KEY_SPACE    = 2^36
CHAIN_LENGTH = 256
NUM_CHAINS   = 1342177280
```

Tổng số bước chain lý thuyết rất lớn, nhưng không đồng nghĩa 100% vì có trùng lặp.

## Tăng/Giảm Cấu Hình

Muốn file table lớn hơn/nhỏ hơn, đổi `NUM_CHAINS`.

Công thức dùng để ước tính file:

```text
file_size_bytes ~= NUM_CHAINS * 16
```

Ví dụ:

| File mong muốn | NUM_CHAINS gần đúng |
|---|---:|
| 1 GiB | 67,108,864 |
| 2 GiB | 134,217,728 |
| 5 GiB | 335,544,320 |
| 10 GiB | 671,088,640 |
| 20 GiB | 1,342,177,280 |

Trong Makefile đã có sẵn preset `cuda-1g`, `cuda-2g`, `cuda-5g`, `cuda-10g`, và `cuda`.

Script cũng đã cập nhật preset:

```bash
./build_cuda.sh demo
./build_cuda.sh 1g
./build_cuda.sh 2g
./build_cuda.sh 5g
./build_cuda.sh 10g
./build_cuda.sh full
```

Trên Windows:

```powershell
.\build_cuda.ps1 demo
.\build_cuda.ps1 1g
.\build_cuda.ps1 2g
.\build_cuda.ps1 5g
.\build_cuda.ps1 10g
.\build_cuda.ps1 full
```

Muốn search nhanh hơn, giảm `CHAIN_LENGTH`. Đổi lại phải tăng `NUM_CHAINS` nếu muốn giữ coverage.

Muốn coverage cao hơn:

- Tăng `NUM_CHAINS` là cách trực tiếp nhất.
- Tăng `CHAIN_LENGTH` giúp coverage nhưng làm crack chậm hơn, vì crack phải thử nhiều vị trí chain.
- Dùng nhiều table với seed/reduction khác sẽ tốt hơn một table quá dài, nhưng code hiện tại chưa implement multi-table.

## Giới Hạn Thực Tế

Bảng gần 20 GiB sẽ tốn:

- Nhiều giờ hoặc lâu hơn tùy GPU.
- Khoảng 20 GiB disk cho file `.bin`.
- Nhiều RAM khi sort/load.
- I/O disk đáng kể khi save/load.

Nếu máy chỉ có 16 GiB RAM, nên giảm `NUM_CHAINS` xuống 5-10 GiB trước.

Với GPU laptop 4 GiB VRAM, không thể đưa toàn bộ table 20 GiB lên GPU để sort một lần. GPU trong code hiện tại được dùng cho phần nặng nhất là sinh chain DES. Các bước sort/save vẫn nằm trên CPU/RAM/disk.

## Ghi Chú Về DES

`H(keyIdx)` trong project:

```cpp
uint64_t H(uint64_t keyIdx) {
    indexToKey(keyIdx, key);
    KeySchedule ks = buildKeySchedule(key);
    return desEncrypt(PLAINTEXT, ks);
}
```

Plaintext cố định nên attacker có thể precompute table trước. Đây là tình huống chosen-plaintext/minh họa tương tự lý do DES không còn an toàn cho hệ thống hiện đại.

## Troubleshooting

### `nvcc` không nhận

CUDA Toolkit chưa cài hoặc `nvcc` chưa nằm trong PATH.

Kiểm tra:

```bash
nvcc --version
nvidia-smi
```

### Build CUDA nhưng vẫn CPU fallback

Kiểm tra:

- Có NVIDIA GPU không.
- Driver có chạy không.
- Binary đang chạy có phải `./rainbow_attack_cuda` trên Linux hoặc `rainbow_attack_cuda.exe` trên Windows không.
- Output đầu chương trình có hiện `CUDA build` không.
- Khi chọn `3`, output có hiện `Backend : CUDA GPU` không.

Trên Ubuntu, nếu `nvidia-smi` báo không giao tiếp được với driver, cài/sửa driver:

```bash
sudo ubuntu-drivers devices
sudo ubuntu-drivers autoinstall
sudo reboot
```

### Load bảng cũ không được

Bản mới dùng format `RBT4` vì mapping DES key đã được sửa và endpoint trùng không còn bị xóa. Bảng format cũ sẽ không load để tránh sai kết quả hoặc mất coverage. Hãy build lại table.

### Nhập key hợp lệ nhưng miss

Đây là hạn chế của rainbow table. Tăng `NUM_CHAINS`, tạo table lớn hơn, hoặc implement multi-table nếu cần tỷ lệ tìm thấy cao hơn nữa.
