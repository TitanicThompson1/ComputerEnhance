#!/bin/bash

gcc decode.c -o decode
nasm $1 -o f1
./decode f1 >> f1.asm
nasm f1.asm -o f2
if cmp -s f1 f2; then 
    echo "✅ SUCCESS: The binary files are identical!"
else
    echo "❌ FAILURE: The binary files do not match."
    cat f1.asm
    echo "================="
    cat $1
fi
rm decode f1 f1.asm f2