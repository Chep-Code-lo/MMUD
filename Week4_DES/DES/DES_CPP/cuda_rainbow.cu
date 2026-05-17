#include "config.h"
#include "gpu_builder.h"

#include <cuda_runtime.h>
#include <cstdint>
#include <iostream>
#include <utility>

struct GpuPair {
    uint64_t start;
    uint64_t end;
};

__constant__ int d_IP[64] = {
    58,50,42,34,26,18,10,2, 60,52,44,36,28,20,12,4,
    62,54,46,38,30,22,14,6, 64,56,48,40,32,24,16,8,
    57,49,41,33,25,17, 9,1, 59,51,43,35,27,19,11,3,
    61,53,45,37,29,21,13,5, 63,55,47,39,31,23,15,7
};
__constant__ int d_IP_INV[64] = {
    40,8,48,16,56,24,64,32, 39,7,47,15,55,23,63,31,
    38,6,46,14,54,22,62,30, 37,5,45,13,53,21,61,29,
    36,4,44,12,52,20,60,28, 35,3,43,11,51,19,59,27,
    34,2,42,10,50,18,58,26, 33,1,41, 9,49,17,57,25
};
__constant__ int d_PC1[56] = {
    57,49,41,33,25,17, 9, 1,58,50,42,34,26,18,
    10, 2,59,51,43,35,27,19,11, 3,60,52,44,36,
    63,55,47,39,31,23,15, 7,62,54,46,38,30,22,
    14, 6,61,53,45,37,29,21,13, 5,28,20,12, 4
};
__constant__ int d_PC2[48] = {
    14,17,11,24, 1, 5, 3,28,15, 6,21,10,
    23,19,12, 4,26, 8,16, 7,27,20,13, 2,
    41,52,31,37,47,55,30,40,51,45,33,48,
    44,49,39,56,34,53,46,42,50,36,29,32
};
__constant__ int d_E_TABLE[48] = {
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9,
     8, 9,10,11,12,13,12,13,14,15,16,17,
    16,17,18,19,20,21,20,21,22,23,24,25,
    24,25,26,27,28,29,28,29,30,31,32, 1
};
__constant__ int d_P_TABLE[32] = {
    16, 7,20,21,29,12,28,17, 1,15,23,26, 5,18,31,10,
     2, 8,24,14,32,27, 3, 9,19,13,30, 6,22,11, 4,25
};
__constant__ int d_SHIFTS[16] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};
__constant__ int d_SBOX[8][4][16] = {
    {{14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7},{0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8},{4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0},{15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13}},
    {{15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10},{3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5},{0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15},{13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9}},
    {{10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8},{13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1},{13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7},{1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12}},
    {{7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15},{13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9},{10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4},{3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14}},
    {{2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9},{14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6},{4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14},{11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3}},
    {{12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11},{10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8},{9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6},{4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13}},
    {{4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1},{13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6},{1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2},{6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12}},
    {{13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7},{1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2},{7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8},{2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11}}
};

__device__ __forceinline__ int bit64(uint64_t block, int b) { return (block >> (64 - b)) & 1; }
__device__ __forceinline__ int bit32(uint32_t block, int b) { return (block >> (32 - b)) & 1; }

__device__ uint64_t keyFromIndex(uint64_t idx) {
    return ((idx & 0x00FFFFFFFFFFFFFFULL) << 8) | 0xDDULL;
}

__device__ uint64_t encryptDes(uint64_t plain, uint64_t keyVal) {
    uint64_t subkeys[16];
    uint64_t cd = 0;
    for (int i = 0; i < 56; i++) cd = (cd << 1) | bit64(keyVal, d_PC1[i]);
    uint32_t c = (cd >> 28) & 0x0FFFFFFF;
    uint32_t d = cd & 0x0FFFFFFF;
    for (int round = 0; round < 16; round++) {
        int s = d_SHIFTS[round];
        c = ((c << s) | (c >> (28 - s))) & 0x0FFFFFFF;
        d = ((d << s) | (d >> (28 - s))) & 0x0FFFFFFF;
        uint64_t cd56 = ((uint64_t)c << 28) | d;
        uint64_t subkey = 0;
        for (int i = 0; i < 48; i++) subkey = (subkey << 1) | ((cd56 >> (56 - d_PC2[i])) & 1);
        subkeys[round] = subkey;
    }

    uint64_t perm = 0;
    for (int i = 0; i < 64; i++) perm = (perm << 1) | bit64(plain, d_IP[i]);
    uint32_t left = (perm >> 32) & 0xFFFFFFFF;
    uint32_t right = perm & 0xFFFFFFFF;

    for (int round = 0; round < 16; round++) {
        uint64_t er = 0;
        for (int i = 0; i < 48; i++) er = (er << 1) | bit32(right, d_E_TABLE[i]);
        uint64_t xored = er ^ subkeys[round];
        uint32_t sout = 0;
        for (int s = 0; s < 8; s++) {
            int chunk = (xored >> (42 - 6*s)) & 0x3F;
            int row = ((chunk >> 5) & 1) << 1 | (chunk & 1);
            int col = (chunk >> 1) & 0xF;
            sout = (sout << 4) | d_SBOX[s][row][col];
        }
        uint32_t pout = 0;
        for (int i = 0; i < 32; i++) pout = (pout << 1) | bit32(sout, d_P_TABLE[i]);
        uint32_t newRight = left ^ pout;
        left = right;
        right = newRight;
    }

    uint64_t preout = ((uint64_t)right << 32) | left;
    uint64_t cipher = 0;
    for (int i = 0; i < 64; i++) cipher = (cipher << 1) | bit64(preout, d_IP_INV[i]);
    return cipher;
}

__device__ __forceinline__ uint64_t reduceCipher(uint64_t cipher, int position) {
    return (cipher ^ ((uint64_t)position * 0x517CC1B727220A95ULL)) % KEY_SPACE;
}

__global__ void chainKernel(const uint64_t* starts, GpuPair* results, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    uint64_t cur = starts[i];
    for (int pos = 0; pos < CHAIN_LENGTH; pos++) {
        uint64_t cipher = encryptDes(PLAINTEXT, keyFromIndex(cur));
        cur = reduceCipher(cipher, pos);
    }
    results[i].start = starts[i];
    results[i].end = cur;
}

bool generateChainsGpu(
    const std::vector<uint64_t>& starts,
    std::vector<std::pair<uint64_t, uint64_t>>& results)
{
    if (starts.empty()) return true;

    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount <= 0) return false;

    uint64_t* dStarts = nullptr;
    GpuPair* dResults = nullptr;
    size_t n = starts.size();
    cudaError_t err = cudaMalloc(&dStarts, n * sizeof(uint64_t));
    if (err != cudaSuccess) return false;
    err = cudaMalloc(&dResults, n * sizeof(GpuPair));
    if (err != cudaSuccess) {
        cudaFree(dStarts);
        return false;
    }

    cudaMemcpy(dStarts, starts.data(), n * sizeof(uint64_t), cudaMemcpyHostToDevice);
    int block = 128;
    int grid = (int)((n + block - 1) / block);
    chainKernel<<<grid, block>>>(dStarts, dResults, (int)n);
    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        std::cerr << "  [!] CUDA error: " << cudaGetErrorString(err) << "\n";
        cudaFree(dStarts);
        cudaFree(dResults);
        return false;
    }

    std::vector<GpuPair> tmp(n);
    cudaMemcpy(tmp.data(), dResults, n * sizeof(GpuPair), cudaMemcpyDeviceToHost);
    cudaFree(dStarts);
    cudaFree(dResults);

    results.resize(n);
    for (size_t i = 0; i < n; i++) results[i] = {tmp[i].start, tmp[i].end};
    return true;
}
