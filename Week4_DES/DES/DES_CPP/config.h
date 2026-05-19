#pragma once
#include <cstdint>

// ============================================================
//  CONFIG — Thay đổi thông số tại đây
// ============================================================

// High-coverage CUDA profile.
// File raw size xap xi: NUM_CHAINS * 16 bytes.
// Cau hinh nay tao file gan 20 GiB, nhung can RAM lon hon nhieu khi load
// vao unordered_map de crack (thuong >50 GiB tuy compiler/stdlib).
static constexpr int      KEY_BITS     = 36;
static constexpr uint64_t KEY_SPACE    = 1ULL << KEY_BITS;

static constexpr int      CHAIN_LENGTH = 256;
static constexpr int      NUM_CHAINS   = 1342177280;

static constexpr uint64_t PLAINTEXT    = 0x1122334455667788ULL;
