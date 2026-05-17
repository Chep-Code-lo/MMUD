#pragma once
#include <cstdint>
#include <unordered_map>

uint32_t R(uint64_t cipher, int position);
uint32_t generateChainEnd(uint32_t start);
std::unordered_map<uint32_t, uint32_t> buildTable();
std::pair<uint32_t, bool> crack(
    uint64_t targetCipher,
    const std::unordered_map<uint32_t, uint32_t>& table,
    long long& steps
);
