// ============================================================
//  rainbow_table.cpp — Build & Crack Rainbow Table
// ============================================================
#include "rainbow_table.h"
#include "des.h"
#include "config.h"
#include "gpu_builder.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <ctime>
#include <random>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <string>

static std::vector<uint64_t> g_chainStarts;

static std::string tableFileName() {
    return "rainbow_table_k" + std::to_string(KEY_BITS)
         + "_m" + std::to_string(NUM_CHAINS)
         + "_t" + std::to_string(CHAIN_LENGTH)
         + ".bin";
}

// R_i(cipher) → key index, mỗi position i cho kết quả khác nhau
uint64_t R(uint64_t cipher, int position) {
    return (cipher ^ ((uint64_t)position * 0x517CC1B727220A95ULL)) % KEY_SPACE;
}

// Sinh chain: start → [H] → [R0] → [H] → [R1] → ... → end
uint64_t generateChainEnd(uint64_t start) {
    uint64_t cur = start;
    for (int pos = 0; pos < CHAIN_LENGTH; pos++) {
        cur = R(H(cur), pos);
    }
    return cur;
}

uint64_t randomKeyFromBuiltChains() {
    if (g_chainStarts.empty()) return 0;

    static std::mt19937 rng((uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<size_t> chainDist(0, g_chainStarts.size() - 1);
    std::uniform_int_distribution<int> posDist(0, CHAIN_LENGTH - 1);

    uint64_t key = g_chainStarts[chainDist(rng)];
    int targetPos = posDist(rng);
    for (int pos = 0; pos < targetPos; pos++) {
        key = R(H(key), pos);
    }
    return key;
}

std::unordered_map<uint64_t, uint64_t> buildTable() {
    std::cout << "\n" << std::string(65,'=') << "\n";
    std::cout << "  PHASE 1 -- XAY DUNG RAINBOW TABLE\n";
    std::cout << std::string(65,'=') << "\n";
    std::cout << "  Plaintext co dinh : 0x"
              << std::hex << std::uppercase << PLAINTEXT << std::dec << "\n";
    std::cout << "  Key space         : 2^" << KEY_BITS << " = " << KEY_SPACE << " keys\n";
    std::cout << "  So chain (m)      : " << NUM_CHAINS << "\n";
    std::cout << "  Do dai chain (t)  : " << CHAIN_LENGTH << "\n";

    double cov = 1.0 - std::pow(1.0 - (double)CHAIN_LENGTH/KEY_SPACE, NUM_CHAINS);
    std::cout << "  Coverage uoc tinh : ~"
              << std::fixed << std::setprecision(2) << cov*100 << "%\n";

    unsigned numThreads = std::max(1u, std::thread::hardware_concurrency());
    std::cout << "  Threads           : " << numThreads << "\n";
    std::cout << std::string(65,'-') << "\n";

    std::mt19937 rng((uint32_t)time(nullptr));
    std::uniform_int_distribution<uint64_t> dist(0, KEY_SPACE-1);
    std::vector<uint64_t> starts(NUM_CHAINS);
    for (auto& s : starts) s = dist(rng);
    g_chainStarts = starts;

    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::pair<uint64_t,uint64_t>> results(NUM_CHAINS);
    if (generateChainsGpu(starts, results)) {
        std::cout << "  Backend           : CUDA GPU\n";
    } else {
        std::cout << "  Backend           : CPU fallback\n";
        std::atomic<int> done(0);
        std::vector<std::thread> threads;

        auto worker = [&](int from, int to) {
            for (int i = from; i < to; i++) {
                results[i] = { starts[i], generateChainEnd(starts[i]) };
                done.fetch_add(1, std::memory_order_relaxed);
            }
        };

        int chunk = NUM_CHAINS / numThreads;
        for (unsigned t = 0; t < numThreads; t++) {
            int from = t * chunk;
            int to   = (t == numThreads-1) ? NUM_CHAINS : from + chunk;
            threads.emplace_back(worker, from, to);
        }

        int lastPrint = -1;
        while (done.load() < NUM_CHAINS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            int d = done.load();
            if (d - lastPrint >= 400 || d == NUM_CHAINS) {
                double el  = std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
                double pct = (double)d/NUM_CHAINS*100.0;
                int    b   = (int)(pct/5);
                std::string bar(b,'#'); bar += std::string(20-b,'.');
                long long sp = (el>0) ? (long long)(d*CHAIN_LENGTH/el) : 0;
                std::cout << "\r  [" << bar << "] "
                          << std::fixed << std::setprecision(1) << pct << "%  "
                          << d << "/" << NUM_CHAINS << "  " << sp << " ops/s  ("
                          << el << "s)  " << std::flush;
                lastPrint = d;
            }
        }
        for (auto& t : threads) t.join();
        std::cout << "\n";
    }

    std::unordered_map<uint64_t,uint64_t> table;
    table.reserve(NUM_CHAINS);
    int collisions = 0;
    for (auto& [s,e] : results) {
        if (table.count(e)) collisions++;
        else table[e] = s;
    }

    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    std::cout << "\n  [OK] Bang xay xong trong "
              << std::fixed << std::setprecision(2) << elapsed << "s\n";
    std::cout << "  Unique endpoints : " << table.size() << "\n";
    std::cout << "  Collisions       : " << collisions << "\n";
    std::cout << "  Kich thuoc bang  : ~" << table.size()*16/1024 << " KB\n";
    return table;
}

static bool saveTable(const std::unordered_map<uint64_t,uint64_t>& table) {
    std::ofstream out(tableFileName(), std::ios::binary);
    if (!out) return false;

    uint32_t magic = 0x52425431; // RBT1
    uint32_t keyBits = KEY_BITS;
    uint32_t chainLength = CHAIN_LENGTH;
    uint32_t numChains = NUM_CHAINS;
    uint64_t plaintext = PLAINTEXT;
    uint64_t count = table.size();

    out.write((char*)&magic, sizeof(magic));
    out.write((char*)&keyBits, sizeof(keyBits));
    out.write((char*)&chainLength, sizeof(chainLength));
    out.write((char*)&numChains, sizeof(numChains));
    out.write((char*)&plaintext, sizeof(plaintext));
    out.write((char*)&count, sizeof(count));
    for (auto& [end, start] : table) {
        out.write((char*)&end, sizeof(end));
        out.write((char*)&start, sizeof(start));
    }
    return (bool)out;
}

static bool loadTable(std::unordered_map<uint64_t,uint64_t>& table) {
    std::ifstream in(tableFileName(), std::ios::binary);
    if (!in) return false;

    uint32_t magic = 0;
    uint32_t keyBits = 0;
    uint32_t chainLength = 0;
    uint32_t numChains = 0;
    uint64_t plaintext = 0;
    uint64_t count = 0;

    in.read((char*)&magic, sizeof(magic));
    in.read((char*)&keyBits, sizeof(keyBits));
    in.read((char*)&chainLength, sizeof(chainLength));
    in.read((char*)&numChains, sizeof(numChains));
    in.read((char*)&plaintext, sizeof(plaintext));
    in.read((char*)&count, sizeof(count));
    if (!in || magic != 0x52425431 || keyBits != KEY_BITS ||
        chainLength != CHAIN_LENGTH || numChains != NUM_CHAINS ||
        plaintext != PLAINTEXT) {
        return false;
    }

    table.clear();
    table.reserve((size_t)count);
    g_chainStarts.clear();
    g_chainStarts.reserve((size_t)count);
    for (uint64_t i = 0; i < count; i++) {
        uint64_t end = 0;
        uint64_t start = 0;
        in.read((char*)&end, sizeof(end));
        in.read((char*)&start, sizeof(start));
        if (!in) return false;
        table[end] = start;
        g_chainStarts.push_back(start);
    }
    return true;
}

std::unordered_map<uint64_t, uint64_t> loadOrBuildTable() {
    std::unordered_map<uint64_t,uint64_t> table;
    std::cout << "\n  Rainbow table file: " << tableFileName() << "\n";
    if (loadTable(table)) {
        std::cout << "  [OK] Da load bang co san: " << table.size() << " endpoints\n";
        return table;
    }

    std::cout << "  [*] Chua co bang phu hop, se precompute mot lan roi luu lai.\n";
    table = buildTable();
    if (saveTable(table)) {
        std::cout << "  [OK] Da luu bang de lan sau load nhanh.\n";
    } else {
        std::cout << "  [!] Khong luu duoc bang, lan sau se phai build lai.\n";
    }
    return table;
}

std::pair<uint64_t,bool> crack(
    uint64_t targetCipher,
    const std::unordered_map<uint64_t,uint64_t>& table,
    long long& steps)
{
    steps = 0;
    for (int p = CHAIN_LENGTH-1; p >= 0; p--) {
        uint64_t cur = R(targetCipher, p);
        steps++;
        for (int pos = p+1; pos < CHAIN_LENGTH; pos++) {
            cur = R(H(cur), pos);
            steps++;
        }
        auto it = table.find(cur);
        if (it != table.end()) {
            uint64_t k = it->second;
            for (int pos = 0; pos < CHAIN_LENGTH; pos++) {
                uint64_t c = H(k);
                if (c == targetCipher) return {k, true};
                k = R(c, pos);
            }
        }
    }
    return {0, false};
}
