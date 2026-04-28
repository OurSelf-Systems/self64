#!/usr/bin/env python3
"""
Rewrite a CMake-generated Xcode project's source-file references to use
absolute, symlink-resolved paths.

Why: vm64/src/ is a tree of symlinks into vm/src/. Xcode's atomic save
replaces symlinks with regular files, breaking the mirror. Pointing the
project at realpath()-resolved locations sends writes directly to the
real files.

Usage: fix-xcode-paths.py <build-dir> <source-dir>
  e.g. fix-xcode-paths.py cmake-build-amd64-xcode vm64
"""
import os, re, sys

build_dir, src_dir = sys.argv[1], sys.argv[2]
pbx = os.path.join(build_dir, 'Self.xcodeproj', 'project.pbxproj')
src_root = os.path.realpath(src_dir)

with open(pbx) as f:
    s = f.read()

def repl(m):
    rel = m.group(1)
    real = os.path.realpath(os.path.join(src_root, rel))
    if not os.path.exists(real):
        return m.group(0)
    return 'path = "%s"; sourceTree = "<absolute>";' % real

new = re.sub(r'path = (src/[^;]+); sourceTree = SOURCE_ROOT;', repl, s)
n = s.count('sourceTree = SOURCE_ROOT;') - new.count('sourceTree = SOURCE_ROOT;')

if new != s:
    with open(pbx, 'w') as f:
        f.write(new)
print('fix-xcode-paths: rewrote %d file references in %s' % (n, pbx))
