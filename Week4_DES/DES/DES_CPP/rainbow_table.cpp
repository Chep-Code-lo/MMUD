// ============================================================
//  rainbow_table.cpp — Build & Crack Rainbow Table
// ============================================================
#include "rainbow_table.h"
#include "des.h"
#include "config.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <ctime>
#include <random>
#include <thread>
#include <atomic>
#include <chrono>

// R_i(cipher) → key index, mỗi position i cho kết quả khác nhau
uint32_t R(uint64_t cipher, int position) {
    return (uint32_t)((cipher ^ ((uint64_t)position * 0x517CC1B727220A95ULL)) % KEY_SPACE);
}

// Sinh chain: start → [H] → [R0] → [H] → [R1] → ... → end
uint32_t generateChainEnd(uint32_t start) {
    uint32_t cur = start;
    for (int pos = 0; pos < CHAIN_LENGTH; pos++) {
        cur = R(H(cur), pos);
    }
    return cur;
}

std::unordered_map<uint32_t, uint32_t> buildTable() {
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
    std::uniform_int_distribution<uint32_t> dist(0, (uint32_t)(KEY_SPACE-1));
    std::vector<uint32_t> starts(NUM_CHAINS);
    for (auto& s : starts) s = dist(rng);

    std::vector<std::pair<uint32_t,uint32_t>> results(NUM_CHAINS);
    std::atomic<int> done(0);
    std::vector<std::thread> threads;

    auto worker = [&](int from, int to) {
        for (int i = from; i < to; i++) {
            results[i] = { starts[i], generateChainEnd(starts[i]) };
            done.fetch_add(1, std::memory_order_relaxed);
        }
    };

    auto t0 = std::chrono::steady_clock::now();
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

    std::unordered_map<uint32_t,uint32_t> table;
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
    std::cout << "  Kich thuoc bang  : ~" << table.size()*8/1024 << " KB\n";
    return table;
}

std::pair<uint32_t,bool> crack(
    uint64_t targetCipher,
    const std::unordered_map<uint32_t,uint32_t>& table,
    long long& steps)
{
    steps = 0;
    for (int p = CHAIN_LENGTH-1; p >= 0; p--) {
        uint32_t cur = R(targetCipher, p);
        steps++;
        for (int pos = p+1; pos < CHAIN_LENGTH; pos++) {
            cur = R(H(cur), pos);
            steps++;
        }
        auto it = table.find(cur);
        if (it != table.end()) {
            uint32_t k = it->second;
            for (int pos = 0; pos < CHAIN_LENGTH; pos++) {
                uint64_t c = H(k);
                if (c == targetCipher) return {k, true};
                k = R(c, pos);
            }
        }
    }
    return {0, false};
}
