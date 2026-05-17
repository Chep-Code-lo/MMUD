// ============================================================
//  main.cpp — Entry point: Menu, Demo, Benchmark
// ============================================================
#include "config.h"
#include "des.h"
#include "rainbow_table.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <random>
#include <ctime>

static void benchmarkSpeed() {
    std::cout << "\n  [*] Do toc do DES...\n";
    const int N = 50000;
    auto t0 = std::chrono::steady_clock::now();
    volatile uint64_t dummy = 0;
    for (int i = 0; i < N; i++) dummy ^= H((uint32_t)i);
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    double speed = N / elapsed;
    std::cout << "  Toc do DES       : "
              << std::fixed << std::setprecision(0) << speed << " ops/giay\n";
    std::cout << "  Uoc tinh bang    : "
              << std::setprecision(1) << (double)NUM_CHAINS*CHAIN_LENGTH/speed << "s\n";
}

static void doAttack(uint32_t targetIdx,
                     const std::unordered_map<uint32_t,uint32_t>& table)
{
    uint64_t targetCipher = H(targetIdx);
    std::cout << "\n" << std::string(65,'=') << "\n";
    std::cout << "  PHASE 2 -- TAN CONG\n" << std::string(65,'=') << "\n";
    std::cout << "  [Victim] Key index  : " << targetIdx
              << "  (0x" << std::hex << std::uppercase << targetIdx << std::dec << ")\n";
    std::cout << "  [Victim] Plaintext  : 0x"
              << std::hex << std::uppercase << PLAINTEXT << std::dec << "\n";
    std::cout << "  [Victim] Ciphertext : 0x"
              << std::hex << std::uppercase << targetCipher << std::dec << "\n";
    std::cout << "  >> Tra bang...\n";

    long long steps = 0;
    auto t0 = std::chrono::steady_clock::now();
    auto [found, ok] = crack(targetCipher, table, steps);
    double ms = std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()*1000.0;

    std::cout << "\n" << std::string(65,'=') << "\n";
    std::cout << "  KET QUA\n" << std::string(65,'=') << "\n";
    if (ok) {
        std::cout << "  [OK] TIM THAY KEY!\n";
        std::cout << "     Key tim duoc : " << found
                  << "  (0x" << std::hex << std::uppercase << found << std::dec << ")\n";
        std::cout << "     Key that     : " << targetIdx
                  << "  (0x" << std::hex << std::uppercase << targetIdx << std::dec << ")\n";
        std::cout << "     Ket qua      : "
                  << ((found==targetIdx) ? "Trung khop hoan toan!"
                                         : "DES Equivalent Key!") << "\n";
    } else {
        std::cout << "  [X] Khong tim thay (table miss)\n";
        std::cout << "      Tang NUM_CHAINS hoac CHAIN_LENGTH trong config.h\n";
    }
    std::cout << "  Thoi gian : " << std::fixed << std::setprecision(2) << ms << " ms\n";
    std::cout << "  So buoc   : " << steps << "\n";
}

static void showRealWorld() {
    std::cout << "\n" << std::string(65,'=') << "\n";
    std::cout << "  THUC TE -- DES Rainbow Table Attack\n";
    std::cout << std::string(65,'=') << "\n";
    struct Row { const char* k; const char* v; };
    Row rows[] = {
        {"Plaintext co dinh", "0x1122334455667788 (Net-NTLMv1)"},
        {"Key space that",    "2^56 = 72,057,594,037,927,936 keys"},
        {"Cong cu",           "crack.sh, ophcrack"},
        {"Phan cung",         "GPU cluster / FPGA"},
        {"Xay bang",          "Nhieu tuan tren GPU"},
        {"Thoi gian crack",   "~25 giay (chi tra bang!)"},
        {"Kich thuoc bang",   "~7 GB (da nen)"},
        {"Website",           "https://crack.sh"},
    };
    for (auto& r : rows)
        std::cout << "  " << std::left << std::setw(22) << r.k << ": " << r.v << "\n";
    std::cout << "\n  -> DES bi khai tu nam 2001, thay bang AES!\n";
    std::cout << std::string(65,'=') << "\n";
}

static void demoAuto(const std::unordered_map<uint32_t,uint32_t>& table) {
    std::mt19937 rng((uint32_t)time(nullptr));
    std::uniform_int_distribution<uint32_t> dist(0,(uint32_t)(KEY_SPACE-1));
    while (true) {
        doAttack(dist(rng), table);
        std::cout << "\n  Chay lai? (y/n): ";
        char c; std::cin >> c;
        if (c!='y' && c!='Y') break;
    }
}

static void demoManual(const std::unordered_map<uint32_t,uint32_t>& table) {
    while (true) {
        std::cout << "\n  Nhap key (0 -> " << (KEY_SPACE-1) << ", vd: 12345 hoac 0xABCD): ";
        std::string raw; std::cin >> raw;
        uint32_t keyIdx = 0;
        try {
            if (raw.size()>2 && raw[0]=='0' && (raw[1]=='x'||raw[1]=='X'))
                keyIdx = (uint32_t)std::stoul(raw,nullptr,16);
            else
                keyIdx = (uint32_t)std::stoul(raw);
        } catch (...) { std::cout << "  Key khong hop le!\n"; continue; }
        doAttack(keyIdx, table);
        std::cout << "\n  Nhap key khac? (y/n): ";
        char c; std::cin >> c;
        if (c!='y' && c!='Y') break;
    }
}

int main() {
    std::cout << "\n" << std::string(65,'=') << "\n";
    std::cout << "  DES Rainbow Table Attack -- C++ Version\n";
    std::cout << "  (Built-in DES | " << KEY_BITS << "-bit key space | Multi-threaded)\n";
    std::cout << std::string(65,'=') << "\n";

    benchmarkSpeed();

    std::cout << "\n  Menu:\n";
    std::cout << "  1. Demo tu dong      (crack key ngau nhien)\n";
    std::cout << "  2. Nhap key thu cong (0 -> " << (KEY_SPACE-1) << ")\n";
    std::cout << "  3. Xem thong tin thuc te ngoai doi\n";
    std::cout << "\n  Chon (1/2/3): ";

    int choice; std::cin >> choice;
    if (choice==3) { showRealWorld(); return 0; }

    auto table = buildTable();
    if (choice==2) demoManual(table);
    else           demoAuto(table);
    return 0;
}
