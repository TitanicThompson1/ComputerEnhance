
#!/bin/bash

if [[ -z "$1" ]]; then
    echo "Usage: $0 <filename_filter> [r|d] [arguments...]"
    exit 1
fi

filter="$1"
buildtype="$2"

# Remove filter and optional build type from arguments
shift
if [[ "$buildtype" == "r" || "$buildtype" == "d" ]]; then
    shift
else
    buildtype=""
fi

buildname=""

for file in build/*"$filter"*; do
    [[ -f "$file" && -x "$file" ]] || continue

    if [[ -n "$buildtype" && "$file" != *"_${buildtype}c" ]]; then
        continue
    fi

    buildname="$file"
    break
done

if [[ -z "$buildname" ]]; then
    echo "No matching executable."
    exit 1
fi

echo
echo "============ $buildname ============"
echo

"$buildname" "$@"
