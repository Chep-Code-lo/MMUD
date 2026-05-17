nvcc -O3 -std=c++17 -DUSE_CUDA -o rainbow_attack_cuda.exe main.cpp des.cpp rainbow_table.cpp cuda_rainbow.cu
