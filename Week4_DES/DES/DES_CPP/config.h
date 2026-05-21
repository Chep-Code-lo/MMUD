#pragma once
#include <cstdint>

// ============================================================
//  CONFIG — Thay đổi thông số tại đây
// ============================================================

// High-coverage CUDA profile by default.
// File raw size xap xi: NUM_CHAINS * 16 bytes.
// Tren Ubuntu co the build cau hinh nho de test nhanh bang:
//   make demo
#ifndef KEY_BITS_VALUE
#define KEY_BITS_VALUE 36
#endif

#ifndef CHAIN_LENGTH_VALUE
#define CHAIN_LENGTH_VALUE 256
#endif

#ifndef NUM_CHAINS_VALUE
#define NUM_CHAINS_VALUE 1342177280
#endif

#ifndef PLAINTEXT_VALUE
#define PLAINTEXT_VALUE 0x1122334455667788ULL
#endif

static constexpr int      KEY_BITS     = KEY_BITS_VALUE;
static constexpr uint64_t KEY_SPACE    = 1ULL << KEY_BITS;

static constexpr int      CHAIN_LENGTH = CHAIN_LENGTH_VALUE;
static constexpr int      NUM_CHAINS   = NUM_CHAINS_VALUE;

static constexpr uint64_t PLAINTEXT    = PLAINTEXT_VALUE;
