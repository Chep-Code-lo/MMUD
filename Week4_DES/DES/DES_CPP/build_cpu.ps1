param(
    [ValidateSet("demo", "full")]
    [string]$Profile = "full"
)

$flags = @("-O3", "-std=c++17")

if ($Profile -eq "demo") {
    $flags += @(
        "-DKEY_BITS_VALUE=20",
        "-DCHAIN_LENGTH_VALUE=64",
        "-DNUM_CHAINS_VALUE=65536"
    )
}

g++ @flags -o rainbow_attack.exe main.cpp des.cpp rainbow_table.cpp gpu_builder_stub.cpp
