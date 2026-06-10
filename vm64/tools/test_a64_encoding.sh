#!/bin/sh
# Cross-check a64_encoding.hh against the system assembler.
# Generates the same instruction battery twice -- once through our encoders,
# once as assembly text fed to clang -- and diffs the 32-bit words.
set -e
cd "$(dirname "$0")"
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT

c++ -std=c++11 -o "$workdir/gen" test_a64_encoding.cpp
( cd "$workdir" && ./gen )

cc -c -arch arm64 -o "$workdir/cases.o" "$workdir/cases.s"

# extract instruction words in order from the object file
objdump -d "$workdir/cases.o" | awk '$1 ~ /^[0-9a-f]+:$/ { print $2 }' > "$workdir/theirs_raw.txt"

ours_n=$(wc -l < "$workdir/ours.txt"); theirs_n=$(wc -l < "$workdir/theirs_raw.txt")
if [ "$ours_n" != "$theirs_n" ]; then
  echo "case count mismatch: ours=$ours_n clang=$theirs_n"; exit 1
fi

paste -d' ' "$workdir/theirs_raw.txt" "$workdir/ours.txt" | \
awk '{
  if ($1 != $2) { printf "MISMATCH: clang=%s ours=%s  %s\n", $1, $2, substr($0, index($0,$3)); bad=1 }
  n++
}
END {
  if (bad) { print "FAILED (" n " cases)"; exit 1 }
  print "all " n " encodings match clang"
}'
