#!/usr/bin/env bash
set -euo pipefail

profile="${1:-full}"

case "$profile" in
  demo)
    make demo
    ;;
  full)
    make cpu
    ;;
  *)
    echo "Usage: $0 [demo|full]" >&2
    exit 1
    ;;
esac
