# DES Rainbow Table Attack - C++ / CUDA

Project nay minh hoa tan cong DES chosen-plaintext bang rainbow table. Ban hien tai duoc toi uu cho bang lon gan 20 GiB, build bang CUDA neu co GPU NVIDIA, va lookup bang sorted vector de giam RAM hon so voi `unordered_map`.

Day la demo hoc thuat, khong phai cong cu crack production.

## Trang thai hien tai

Config mac dinh trong `config.h`:

```cpp
static constexpr int      KEY_BITS     = 36;
static constexpr uint64_t KEY_SPACE    = 1ULL << KEY_BITS;
static constexpr int      CHAIN_LENGTH = 256;
static constexpr int      NUM_CHAINS   = 1342177280;
static constexpr uint64_t PLAINTEXT    = 0x1122334455667788ULL;
```

Y nghia:

| Thong so | Gia tri |
|---|---:|
| Key space | 2^36 keys |
| Chain length | 256 |
| So chain | 1,342,177,280 |
| File table raw | ~20.00 GiB |
| Kieu table trong RAM | `vector<pair<endpoint,start>>` da sort |
| Lookup endpoint | binary search bang `std::lower_bound` |

Luu y RAM: file table ~20 GiB nhung khi build/sort/load van can RAM lon. Nen dung may co toi thieu 32 GiB RAM, tot hon la 64 GiB+.

## Cau truc file

```text
DES_CPP/
|-- config.h             # Cau hinh KEY_BITS, CHAIN_LENGTH, NUM_CHAINS
|-- des.h / des.cpp      # DES thuan C++
|-- rainbow_table.h      # API rainbow table
|-- rainbow_table.cpp    # Build, save/load, sort, crack
|-- gpu_builder.h        # Interface GPU backend
|-- gpu_builder_stub.cpp # CPU fallback khi build khong CUDA
|-- cuda_rainbow.cu      # CUDA kernel sinh chain
|-- main.cpp             # Menu va demo
|-- build_cpu.ps1        # Build CPU tren Windows
|-- build_cuda.ps1       # Build CUDA tren Windows
|-- README.md
```

## Build

### CPU

```powershell
.\build_cpu.ps1
.\rainbow_attack.exe
```

CPU build dung `gpu_builder_stub.cpp`, nen build table rat cham voi config 20 GiB. CPU build phu hop de test compile/logic, khong phu hop de precompute bang lon.

### CUDA

Can NVIDIA driver + CUDA Toolkit co `nvcc` trong PATH.

```powershell
nvcc --version
.\build_cuda.ps1
.\rainbow_attack_cuda.exe
```

Neu build dung CUDA, luc chay chuong trinh se hien:

```text
(Built-in DES | 36-bit key space | CUDA build)
Backend           : CUDA GPU
```

Neu thay `Backend : CPU fallback`, nghia la chuong trinh khong dung duoc CUDA runtime/GPU va dang roi ve CPU.

## Cach chay

Menu:

```text
1. Crack demo tu dong       (load bang co san neu co)
2. Crack key thu cong       (0 -> KEY_SPACE-1)
3. Precompute/load table    (giong buoc tao bang cua crack.sh)
4. Xem thong tin thuc te ngoai doi
```

Workflow nen dung:

1. Chay `.\rainbow_attack_cuda.exe`.
2. Chon `3` de precompute table mot lan.
3. Cac lan sau chon `1` hoac `2`, chuong trinh se load file `.bin`.

Ten file table tu dong dua tren config:

```text
rainbow_table_k36_m1342177280_t256.bin
```

Neu doi `KEY_BITS`, `NUM_CHAINS`, `CHAIN_LENGTH` hoac `PLAINTEXT`, chuong trinh se tao file table moi.

## Toi uu da ap dung

### 1. Khong dung `unordered_map` cho bang lon

Ban cu luu table bang:

```cpp
unordered_map<endpoint, start>
```

Voi hang ty endpoint, `unordered_map` ton RAM rat lon do bucket/node overhead. Ban moi luu:

```cpp
using RainbowTable = std::vector<std::pair<uint64_t, uint64_t>>;
```

Moi entry van la `(endpoint, start)` 16 bytes. Sau khi build xong, table duoc sort theo endpoint va xoa endpoint trung.

### 2. Lookup bang binary search

Khi crack, code tinh endpoint gia dinh roi tim bang:

```cpp
std::lower_bound(table.begin(), table.end(), endpoint)
```

Thoi gian lookup la `O(log n)`, nhung RAM it hon nhieu va on dinh hon voi table rat lon.

### 3. Build theo batch

Code khong cap phat `starts` va `results` cho toan bo `NUM_CHAINS` cung luc. Moi batch hien tai:

```cpp
BUILD_BATCH_CHAINS = 1 << 20;
```

Tuc la moi lan sinh 1,048,576 chain, append vao table, roi tiep tuc batch sau.

### 4. CUDA kernel chia chunk

CUDA chain duoc chia thanh cac chunk nho:

```cpp
CUDA_CHAIN_STEP_CHUNK = 256;
```

Voi `CHAIN_LENGTH=256`, moi chain hien tai chay dung 1 kernel chunk. Neu sau nay tang chain length, kernel se chia nho de giam rui ro Windows TDR timeout.

## Vi sao CPU van chay khi build CUDA?

CUDA chi tang toc phan sinh chain trong phase precompute. Cac phan sau van chay tren CPU:

| Phan viec | CPU/GPU |
|---|---|
| Benchmark DES dau chuong trinh | CPU |
| Sinh chain khi co CUDA | GPU |
| Append vector, sort, unique | CPU |
| Save/load file table | CPU / disk I/O |
| Crack/lookup/verify chain | CPU |

Viec CPU van co load la binh thuong. Dieu can xem la dong `Backend`.

## Vi sao nhap key trong vung van co the khong thay?

Rainbow table khong dam bao phu 100% key space. No la time-memory trade-off.

Ly do miss:

- Key do khong nam trong bat ky chain nao cua table.
- Chain collision/merge lam coverage thuc te thap hon cong thuc ly thuyet.
- Reduction function tao endpoint trung.
- Table duoc build voi config/PLAINTEXT khac binary dang chay.

Code da chan truong hop nhap key vuot vung:

```text
0 -> KEY_SPACE-1
```

Nhung neu key hop le ma khong thuoc coverage cua table thi van se bao `Khong tim thay`.

## Cong thuc coverage

Uoc tinh ly thuyet:

```text
coverage ~= 1 - (1 - CHAIN_LENGTH / KEY_SPACE) ^ NUM_CHAINS
```

Cong thuc nay chi la xap xi ly tuong. Coverage thuc te thap hon do collision va merge.

Voi config hien tai:

```text
KEY_SPACE    = 2^36
CHAIN_LENGTH = 256
NUM_CHAINS   = 1342177280
```

Tong so buoc chain ly thuyet rat lon, nhung khong dong nghia 100% vi co trung lap.

## Tang/giam cau hinh

Muon file table lon hon/nho hon, doi `NUM_CHAINS`.

Cong thuc dung de uoc tinh file:

```text
file_size_bytes ~= NUM_CHAINS * 16
```

Vi du:

| File mong muon | NUM_CHAINS gan dung |
|---|---:|
| 5 GiB | 335,544,320 |
| 10 GiB | 671,088,640 |
| 20 GiB | 1,342,177,280 |

Muon search nhanh hon, giam `CHAIN_LENGTH`. Doi lai phai tang `NUM_CHAINS` neu muon giu coverage.

Muon coverage cao hon:

- Tang `NUM_CHAINS` la cach truc tiep nhat.
- Tang `CHAIN_LENGTH` giup coverage nhung lam crack cham hon, vi crack phai thu nhieu vi tri chain.
- Dung nhieu table voi seed/reduction khac se tot hon mot table qua dai, nhung code hien tai chua implement multi-table.

## Gioi han thuc te

Bang gan 20 GiB se ton:

- Nhieu gio hoac lau hon tuy GPU.
- Khoang 20 GiB disk cho file `.bin`.
- Nhieu RAM khi sort/load.
- I/O disk dang ke khi save/load.

Neu may chi co 16 GiB RAM, nen giam `NUM_CHAINS` xuong 5-10 GiB truoc.

## Ghi chu ve DES

`H(keyIdx)` trong project:

```cpp
uint64_t H(uint64_t keyIdx) {
    indexToKey(keyIdx, key);
    KeySchedule ks = buildKeySchedule(key);
    return desEncrypt(PLAINTEXT, ks);
}
```

Plaintext co dinh nen attacker co the precompute table truoc. Day la tinh huong chosen-plaintext/minh hoa tuong tu ly do DES khong con an toan cho he thong hien dai.

## Troubleshooting

### `nvcc` khong nhan

CUDA Toolkit chua cai hoac `nvcc` chua nam trong PATH.

Kiem tra:

```powershell
nvcc --version
nvidia-smi
```

### Build CUDA nhung van CPU fallback

Kiem tra:

- Co NVIDIA GPU khong.
- Driver co chay khong.
- Binary dang chay co phai `rainbow_attack_cuda.exe` khong.
- Output dau chuong trinh co hien `CUDA build` khong.

### Load bang cu khong duoc

Ban moi dung format `RBT2` sorted vector. Bang format cu se khong load de tranh sai ket qua. Hay build lai table.

### Nhap key hop le nhung miss

Day la han che cua rainbow table. Tang `NUM_CHAINS`, tao table lon hon, hoac implement multi-table neu can ty le tim thay cao hon nua.
