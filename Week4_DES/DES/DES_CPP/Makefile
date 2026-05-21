CXX ?= g++
NVCC ?= nvcc

CXXFLAGS ?= -O3 -std=c++17 -pthread -Wall -Wextra -pedantic
NVCCFLAGS ?= -O3 -std=c++17 -DUSE_CUDA

TARGET_CPU := rainbow_attack
TARGET_CUDA := rainbow_attack_cuda

CPU_SOURCES := main.cpp des.cpp rainbow_table.cpp gpu_builder_stub.cpp
CUDA_SOURCES := main.cpp des.cpp rainbow_table.cpp cuda_rainbow.cu

.PHONY: all cpu cuda cuda-1g cuda-2g cuda-5g cuda-10g demo demo-cuda clean run

all: cpu

cpu:
	$(CXX) $(CXXFLAGS) -o $(TARGET_CPU) $(CPU_SOURCES)

cuda:
	$(NVCC) $(NVCCFLAGS) -o $(TARGET_CUDA) $(CUDA_SOURCES)

cuda-1g:
	$(NVCC) $(NVCCFLAGS) \
		-DNUM_CHAINS_VALUE=67108864 \
		-o $(TARGET_CUDA) $(CUDA_SOURCES)

cuda-2g:
	$(NVCC) $(NVCCFLAGS) \
		-DNUM_CHAINS_VALUE=134217728 \
		-o $(TARGET_CUDA) $(CUDA_SOURCES)

cuda-5g:
	$(NVCC) $(NVCCFLAGS) \
		-DNUM_CHAINS_VALUE=335544320 \
		-o $(TARGET_CUDA) $(CUDA_SOURCES)

cuda-10g:
	$(NVCC) $(NVCCFLAGS) \
		-DNUM_CHAINS_VALUE=671088640 \
		-o $(TARGET_CUDA) $(CUDA_SOURCES)

demo:
	$(CXX) $(CXXFLAGS) \
		-DKEY_BITS_VALUE=20 \
		-DCHAIN_LENGTH_VALUE=64 \
		-DNUM_CHAINS_VALUE=65536 \
		-o $(TARGET_CPU) $(CPU_SOURCES)

demo-cuda:
	$(NVCC) $(NVCCFLAGS) \
		-DKEY_BITS_VALUE=20 \
		-DCHAIN_LENGTH_VALUE=64 \
		-DNUM_CHAINS_VALUE=65536 \
		-o $(TARGET_CUDA) $(CUDA_SOURCES)

run: cpu
	./$(TARGET_CPU)

clean:
	rm -f $(TARGET_CPU) $(TARGET_CUDA) *.o
