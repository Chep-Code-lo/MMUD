#pragma once
#include <cstdint>

// ============================================================
//  CONFIG — Thay đổi thông số tại đây
// ============================================================

// Mini crack.sh profile:
// - KEY_BITS < 56 la phien ban thu nho cua DES key space that.
// - Neu co CUDA/GPU kernel, co the tang len 32, 40, 48, roi 56.
static constexpr int      KEY_BITS     = 28;
static constexpr uint64_t KEY_SPACE    = 1ULL << KEY_BITS;

static constexpr int      CHAIN_LENGTH = 15000;
static constexpr int      NUM_CHAINS   = 50000;

static constexpr uint64_t PLAINTEXT    = 0x1122334455667788ULL;
