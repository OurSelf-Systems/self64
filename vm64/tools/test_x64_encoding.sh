#!/bin/sh
# Cross-check x64_encoding.hh against the system assembler.
# Generates the same instruction battery twice -- once through our encoders,
# once as Intel-syntax assembly fed to the system toolchain -- and diffs the
# bytes.  Sibling of test_a64_encoding.sh.
set -e
cd "$(dirname "$0")"
workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT

c++ -std=c++11 -o "$workdir/gen" test_x64_encoding.cpp
( cd "$workdir" && ./gen )

cc -c -o "$workdir/cases.o" "$workdir/cases.s"

# extract instruction bytes in order from the object file; --insn-width
# keeps every instruction's bytes on its own single line
objdump -d --insn-width=15 "$workdir/cases.o" | \
  awk -F'\t' '$1 ~ /^[ ]*[0-9a-f]+:$/ { gsub(/ /, "", $2); print $2 }' \
  > "$workdir/theirs_raw.txt"

ours_n=$(wc -l < "$workdir/ours.txt"); theirs_n=$(wc -l < "$workdir/theirs_raw.txt")
if [ "$ours_n" != "$theirs_n" ]; then
  echo "case count mismatch: ours=$ours_n system=$theirs_n"; exit 1
fi

paste -d' ' "$workdir/theirs_raw.txt" "$workdir/ours.txt" | \
awk '{
  if ($1 != $2) { printf "MISMATCH: system=%s ours=%s  %s\n", $1, $2, substr($0, index($0,$3)); bad=1 }
  n++
}
END {
  if (bad) { print "FAILED (" n " cases)"; exit 1 }
  print "all " n " encodings match the system assembler"
}'
