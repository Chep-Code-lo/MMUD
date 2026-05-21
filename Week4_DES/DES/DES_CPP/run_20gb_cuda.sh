#!/usr/bin/env bash
set -euo pipefail

make cuda
./rainbow_attack_cuda
