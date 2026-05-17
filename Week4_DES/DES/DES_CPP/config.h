#pragma once
#include <cstdint>

// ============================================================
//  CONFIG — Thay đổi thông số tại đây
// ============================================================

static constexpr int      KEY_BITS     = 32;
static constexpr uint64_t KEY_SPACE    = 1ULL << KEY_BITS;

static constexpr int      CHAIN_LENGTH = 1000;
static constexpr int      NUM_CHAINS   = 8000;

static constexpr uint64_t PLAINTEXT    = 0x1122334455667788ULL;
