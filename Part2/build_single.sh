#!/bin/bash

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <source.c>"
    exit 1
fi

if ! command -v clang &>/dev/null; then
    echo "WARNING: clang not found"
    exit 1
fi

mkdir -p build
pushd build > /dev/null || exit 1

filename="../$1"
basename="$(basename "${1%.*}")"

# Debug build
clang -std=c11 -g -Wall "$filename" -o "${basename}_dc" -lm

# Release build (AVX2)
clang -std=c11 -mavx2 -O3 -g -Wall "$filename" -o "${basename}_rc" -lm

popd > /dev/null
