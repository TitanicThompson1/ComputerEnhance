#!/bin/bash

if ! command -v clang++ &>/dev/null; then
    echo "WARNING: clang++ not found -- CLANG executables will not be built"
fi

filter="$1"

while IFS= read -r -d '' file; do
    filename="${file#./}"

    if [[ -z "$filter" || "$filename" == *"$filter"* ]]; then
        ./build_single.sh "$filename"
    fi
done < <(find . -type f -name '*_main.cpp' -print0)