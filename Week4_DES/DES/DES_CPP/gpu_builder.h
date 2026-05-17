#pragma once
#include <cstdint>
#include <utility>
#include <vector>

bool generateChainsGpu(
    const std::vector<uint64_t>& starts,
    std::vector<std::pair<uint64_t, uint64_t>>& results
);
