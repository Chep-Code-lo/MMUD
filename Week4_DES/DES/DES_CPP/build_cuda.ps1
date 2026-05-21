param(
    [ValidateSet("demo", "1g", "2g", "5g", "10g", "full")]
    [string]$Profile = "full"
)

$flags = @("-O3", "-std=c++17", "-DUSE_CUDA")

switch ($Profile) {
    "demo" {
        $flags += @(
            "-DKEY_BITS_VALUE=20",
            "-DCHAIN_LENGTH_VALUE=64",
            "-DNUM_CHAINS_VALUE=65536"
        )
    }
    "1g" {
        $flags += "-DNUM_CHAINS_VALUE=67108864"
    }
    "2g" {
        $flags += "-DNUM_CHAINS_VALUE=134217728"
    }
    "5g" {
        $flags += "-DNUM_CHAINS_VALUE=335544320"
    }
    "10g" {
        $flags += "-DNUM_CHAINS_VALUE=671088640"
    }
}

nvcc @flags -o rainbow_attack_cuda.exe main.cpp des.cpp rainbow_table.cpp cuda_rainbow.cu
