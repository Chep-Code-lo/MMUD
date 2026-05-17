#pragma once
#include <cstdint>
#include <unordered_map>

uint64_t R(uint64_t cipher, int position);
uint64_t generateChainEnd(uint64_t start);
uint64_t randomKeyFromBuiltChains();
std::unordered_map<uint64_t, uint64_t> buildTable();
std::unordered_map<uint64_t, uint64_t> loadOrBuildTable();
std::pair<uint64_t, bool> crack(
    uint64_t targetCipher,
    const std::unordered_map<uint64_t, uint64_t>& table,
    long long& steps
);
