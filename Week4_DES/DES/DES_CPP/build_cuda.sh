#!/usr/bin/env bash
set -euo pipefail

profile="${1:-full}"

case "$profile" in
  demo)
    make demo-cuda
    ;;
  1g)
    make cuda-1g
    ;;
  2g)
    make cuda-2g
    ;;
  5g)
    make cuda-5g
    ;;
  10g)
    make cuda-10g
    ;;
  full)
    make cuda
    ;;
  *)
    echo "Usage: $0 [demo|1g|2g|5g|10g|full]" >&2
    exit 1
    ;;
esac
