#pragma once
#include <cstdint>
#include <utility>
#include <vector>

using RainbowTable = std::vector<std::pair<uint64_t, uint64_t>>; // endpoint -> start

uint64_t R(uint64_t cipher, int position);
uint64_t generateChainEnd(uint64_t start);
uint64_t randomKeyFromTable(const RainbowTable& table);
RainbowTable buildTable();
RainbowTable loadOrBuildTable();
std::pair<uint64_t, bool> crack(
    uint64_t targetCipher,
    const RainbowTable& table,
    long long& steps
);
