#!/bin/sh
set -eu

nm_tool=${NM:-nm}
kernel=${1:-build/x86_64/clks_kernel.elf}
output=${2:-build/x86_64/kernel.sym}

mkdir -p "$(dirname "$output")"
{
    printf '%s\n' CLEONOS_KERNEL_SYMBOLS_V2
    "$nm_tool" -n "$kernel" | awk '$2 ~ /^[TtWw]$/ {print "0X" toupper($1) "\t" $3 "\t??:0"}'
} > "$output"
