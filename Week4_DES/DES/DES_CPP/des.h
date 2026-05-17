#pragma once
#include <cstdint>
#include <cstddef>

struct KeySchedule {
    uint64_t K[16];
};

void        indexToKey(uint64_t idx, uint8_t out[8]);
KeySchedule buildKeySchedule(uint8_t key[8]);
uint64_t    desEncrypt(uint64_t plaintext, const KeySchedule& ks);
uint64_t    H(uint64_t keyIdx);
